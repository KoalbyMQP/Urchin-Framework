//
// Tests for UartDiscovery and receive() timeouts, using the fake serial host:
// a "PC" host with several plugged-in ports, only one of which leads to the
// device ("ESP32") that answers to the identity.
//

#include <gtest/gtest.h>

#include <DBOXprotocol.h>
#include <Discovery/UartDiscovery.h>
#include <Dconfig/DconfigUart.h>
#include <Transport-Interface/UART/Fake/FakeSeralPort.h>
#include <Transport-Interface/UART/Fake/FakeSeralHost.h>
#include <Transport-Interface/UART/Fake/FakeSerialLink.h>

#include <chrono>
#include <memory>
#include <thread>

using namespace DBOXProtocol;
using Clock = std::chrono::steady_clock;

static long msSince(Clock::time_point t) {
    return (long)std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - t).count();
}

// ---------------------------------------------------------------- receive()

TEST(DBOX_Receive, TimeoutReturnsFalseWhenNothingArrives) {
    auto link = std::make_shared<FakeSerialLink>("A", "B");
    FakeSerialPort a(link, FakeSeralHost::Side::A);
    ASSERT_TRUE(a.openraw("A", 115200, 10));
    DBOXprotocol proto(&a, "/dev/null");

    DBOX packet;
    auto t0 = Clock::now();
    EXPECT_FALSE(proto.receive(&packet, 150));
    long took = msSince(t0);
    EXPECT_GE(took, 150);
    EXPECT_LT(took, 1000);
}

TEST(DBOX_Receive, RecoversFromFalseStartMarker) {
    auto link = std::make_shared<FakeSerialLink>("A", "B");
    FakeSerialPort a(link, FakeSeralHost::Side::A);
    FakeSerialPort b(link, FakeSeralHost::Side::B);
    ASSERT_TRUE(a.openraw("A", 115200, 10));
    ASSERT_TRUE(b.openraw("B", 115200, 10));
    DBOXprotocol protoA(&a, "/dev/null");
    DBOXprotocol protoB(&b, "/dev/null");

    // A stray 0x07 followed by junk that cannot pass the header CRC...
    const uint8_t junk[] = {0x07, 'j', 'u', 'n', 'k', '!', '!'};
    b.rawwrite(std::span<const uint8_t>(junk, sizeof(junk)), sizeof(junk));
    // ...then a real packet. The old receive() re-checked the junk forever.
    DBOX sent(3, 'M', "move 90");
    ASSERT_TRUE(protoB.send(&sent));

    DBOX got;
    ASSERT_TRUE(protoA.receive(&got, 1000));
    EXPECT_EQ(got, sent);
}

TEST(DBOX_Receive, RecoversFromCorruptedHeader) {
    auto link = std::make_shared<FakeSerialLink>("A", "B");
    FakeSerialPort a(link, FakeSeralHost::Side::A);
    FakeSerialPort b(link, FakeSeralHost::Side::B);
    ASSERT_TRUE(a.openraw("A", 115200, 10));
    ASSERT_TRUE(b.openraw("B", 115200, 10));
    DBOXprotocol protoA(&a, "/dev/null");
    DBOXprotocol protoB(&b, "/dev/null");

    std::vector<uint8_t> bad = static_cast<std::vector<uint8_t>>(DBOXraw(DBOX(3, 'M', "move 90")));
    bad[1] ^= 0x10;   // corrupt the ID byte, so the header CRC fails
    b.rawwrite(bad, bad.size());
    DBOX good(4, 'S', "sensor ok");
    ASSERT_TRUE(protoB.send(&good));

    DBOX got;
    ASSERT_TRUE(protoA.receive(&got, 1000));
    EXPECT_EQ(got, good);
}

// -------------------------------------------------------------- Discovery

// PC host with a "Mouse" port (nothing answers) listed BEFORE the ESP32's
// port, so a pass proves discovery really moves on to the next port.
struct Bench {
    std::shared_ptr<FakeSerialLink> toMouse = std::make_shared<FakeSerialLink>("COM3", "Mouse");
    std::shared_ptr<FakeSerialLink> toEsp   = std::make_shared<FakeSerialLink>("COM5", "UART0");
    FakeSeralHost pc, esp;
    Bench(bool withEsp = true) {
        pc.AddPlug(toMouse, FakeSeralHost::Side::A);
        if (withEsp) pc.AddPlug(toEsp, FakeSeralHost::Side::A);
        esp.AddPlug(toEsp, FakeSeralHost::Side::B);
    }
};

TEST(DBOX_Discovery, FindsTheDevicePortAmongOtherPorts) {
    Bench bench;

    DconfigUart pcCfg("Urchin", "", 115200, 10);
    DconfigUart espCfg("Urchin", "UART0", 115200, 10);
    UartDiscovery pcDisco(true, &pcCfg);
    UartDiscovery espDisco(false, &espCfg);
    FakeSerialPort pcPort(&pcDisco, &bench.pc);
    FakeSerialPort espPort(&espDisco, &bench.esp);
    DBOXprotocol pcProto(&pcPort, "/dev/null");
    DBOXprotocol espProto(&espPort, "/dev/null");
    pcDisco.handshakeTimeoutMs = 200;

    ASSERT_EQ(pcPort.find(), (std::vector<std::string>{"COM3", "COM5"}));

    int espResult = -1;
    std::thread esp([&] { espResult = espDisco.discover(); });   // passive: waits to be found
    int pcResult = pcDisco.discover();                             // active: tries COM3, then COM5
    esp.join();

    EXPECT_EQ(pcResult, 1);
    EXPECT_EQ(espResult, 1);
    EXPECT_EQ(pcCfg.Port, "COM5");      // remembered the port that answered
    EXPECT_TRUE(pcPort.isOpen());       // and left it open for normal traffic

    // Normal D-BOX traffic now flows over the discovered port.
    DBOX cmd(0, 'N', "Spin 200");
    ASSERT_TRUE(pcProto.send(&cmd));
    DBOX got;
    ASSERT_TRUE(espProto.receive(&got, 1000));
    EXPECT_EQ(got, cmd);
}

TEST(DBOX_Discovery, ReturnsNotFoundInsteadOfHanging) {
    Bench bench(/*withEsp=*/false);   // only the Mouse port

    DconfigUart pcCfg("Urchin", "", 115200, 10);
    UartDiscovery pcDisco(true, &pcCfg);
    FakeSerialPort pcPort(&pcDisco, &bench.pc);
    DBOXprotocol pcProto(&pcPort, "/dev/null");
    pcDisco.handshakeTimeoutMs = 100;

    auto t0 = Clock::now();
    EXPECT_EQ(pcDisco.discover(), 0);
    EXPECT_LT(msSince(t0), 2000);   // 3 attempts x 100 ms, plus slack
    EXPECT_EQ(pcCfg.Port, "");
}

TEST(DBOX_Discovery, IgnoresADeviceWithADifferentName) {
    Bench bench;

    DconfigUart pcCfg("Urchin", "", 115200, 10);
    DconfigUart espCfg("SomethingElse", "UART0", 115200, 10);
    UartDiscovery pcDisco(true, &pcCfg);
    UartDiscovery espDisco(false, &espCfg);
    FakeSerialPort pcPort(&pcDisco, &bench.pc);
    FakeSerialPort espPort(&espDisco, &bench.esp);
    DBOXprotocol pcProto(&pcPort, "/dev/null");
    DBOXprotocol espProto(&espPort, "/dev/null");
    pcDisco.handshakeTimeoutMs = 100;

    // A device that answers every hello, but with the wrong name.
    ASSERT_TRUE(espPort.openraw("UART0", 115200, 10));
    std::atomic_bool stop{false};
    std::thread esp([&] {
        while (!stop) {
            DBOX p;
            if (espProto.receive(&p, 50) && p.Stream == 'S') {
                DBOX reply(0, 'S', "SomethingElse");
                espProto.send(&reply);
            }
        }
    });

    EXPECT_EQ(pcDisco.discover(), 0);
    stop = true;
    esp.join();
}

//
// Tests for LinuxSerialPort using a pseudo-terminal (pty): a real kernel
// serial-style device pair, so these exercise the same open/termios/poll/
// read/write code paths as a USB serial adapter, without hardware.
// The "device" side is played by the pty master.
//

#include <gtest/gtest.h>

#include <DBOXprotocol.h>
#include <Discovery/UartDiscovery.h>
#include <Dconfig/DconfigUart.h>
#include <Transport-Interface/UART/Linux/linux-sys-seral.h>

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <fcntl.h>
#include <poll.h>
#include <pty.h>
#include <string>
#include <thread>
#include <unistd.h>
#include <vector>

using namespace DBOXProtocol;
using Clock = std::chrono::steady_clock;

namespace {

struct Pty {
    int master = -1, slave = -1;
    std::string name;
    Pty() {
        char buf[128];
        if (openpty(&master, &slave, buf, nullptr, nullptr) == 0) name = buf;
        // Keep the slave open so the master doesn't see hangups between
        // the port's open/close calls.
    }
    ~Pty() { if (slave >= 0) ::close(slave); if (master >= 0) ::close(master); }

    void write(const std::vector<uint8_t>& b) const { (void)!::write(master, b.data(), b.size()); }

    // Reads whatever arrives within `ms`.
    std::vector<uint8_t> read(int ms) const {
        std::vector<uint8_t> out;
        auto end = Clock::now() + std::chrono::milliseconds(ms);
        while (Clock::now() < end) {
            pollfd p{master, POLLIN, 0};
            if (::poll(&p, 1, 10) > 0) {
                uint8_t b[256];
                ssize_t n = ::read(master, b, sizeof b);
                if (n > 0) out.insert(out.end(), b, b + n);
            }
        }
        return out;
    }
};

std::vector<uint8_t> frame(uint8_t id, char st, const std::string& text) {
    return static_cast<std::vector<uint8_t>>(DBOXraw(DBOX(id, (uint8_t)st, text)));
}

long msSince(Clock::time_point t) {
    return (long)std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - t).count();
}

} // namespace

TEST(LinuxSerial, OpenFailsForMissingDevice) {
    LinuxSerialPort port;
    EXPECT_FALSE(port.openraw("/dev/does-not-exist", 115200, 10));
    EXPECT_FALSE(port.isOpen());
}

TEST(LinuxSerial, OpenFailsForUnsupportedBaud) {
    Pty pty; ASSERT_FALSE(pty.name.empty());
    LinuxSerialPort port;
    EXPECT_FALSE(port.openraw(pty.name.c_str(), 12345, 10));
}

TEST(LinuxSerial, WritesAndReadsBytes) {
    Pty pty; ASSERT_FALSE(pty.name.empty());
    LinuxSerialPort port;
    ASSERT_TRUE(port.openraw(pty.name.c_str(), 115200, 50));

    const std::vector<uint8_t> out = {1, 2, 3, 0x07, 0xFF};
    EXPECT_EQ(port.write(out, out.size()), out.size());
    EXPECT_EQ(pty.read(100), out);

    pty.write({9, 8, 7});
    std::vector<uint8_t> in(3);
    std::size_t got = 0;
    for (int i = 0; i < 10 && got < 3; ++i)
        got += port.read(std::span<uint8_t>(in).subspan(got), 3 - got);
    EXPECT_EQ(in, (std::vector<uint8_t>{9, 8, 7}));
}

TEST(LinuxSerial, ReadGivesUpAfterTimeoutInsteadOfBlocking) {
    Pty pty; ASSERT_FALSE(pty.name.empty());
    LinuxSerialPort port;
    ASSERT_TRUE(port.openraw(pty.name.c_str(), 115200, 80));

    std::vector<uint8_t> buf(4);
    auto t0 = Clock::now();
    EXPECT_EQ(port.read(buf, 4), 0u);
    EXPECT_GE(msSince(t0), 60);
    EXPECT_LT(msSince(t0), 1000);
}

TEST(LinuxSerial, ProtocolRoundTripOverRealDevice) {
    Pty pty; ASSERT_FALSE(pty.name.empty());
    LinuxSerialPort port;
    ASSERT_TRUE(port.openraw(pty.name.c_str(), 115200, 10));
    DBOXprotocol proto(&port, "/dev/null");

    // Send: the bytes on the wire are exactly one D-BOX frame.
    DBOX cmd(0, 'N', "Spin 200");
    ASSERT_TRUE(proto.send(&cmd));
    EXPECT_EQ(pty.read(100), frame(0, 'N', "Spin 200"));

    // Receive: junk + false start + a real reply, like a noisy line.
    std::vector<uint8_t> in = {'b', 'o', 'o', 't', 0x07, 'x', 'y', 'z', 'w', 'q', 'r'};
    auto reply = frame(0, 'E', "OK SPIN 200");
    in.insert(in.end(), reply.begin(), reply.end());
    pty.write(in);

    DBOX got;
    ASSERT_TRUE(proto.receive(&got, 1000));
    EXPECT_EQ(got, DBOX(0, 'E', "OK SPIN 200"));

    // Silence: receive() times out instead of hanging.
    auto t0 = Clock::now();
    EXPECT_FALSE(proto.receive(&got, 200));
    EXPECT_LT(msSince(t0), 1500);
}

// Discovery against a pty "device" that behaves like the ESP32 firmware:
// answers S "Urchin" with S "Urchin".
TEST(LinuxSerial, DiscoveryHandshakeOverRealDevice) {
    Pty pty; ASSERT_FALSE(pty.name.empty());

    std::atomic_bool stop{false};
    std::thread device([&] {
        auto hello = frame(0, 'S', "Urchin");
        std::vector<uint8_t> seen;
        while (!stop) {
            auto b = pty.read(20);
            seen.insert(seen.end(), b.begin(), b.end());
            if (std::search(seen.begin(), seen.end(), hello.begin(), hello.end()) != seen.end()) {
                pty.write(hello);
                seen.clear();
            }
        }
    });

    DconfigUart cfg("Urchin", "", 115200, 10);
    UartDiscovery disco(true, &cfg);
    LinuxSerialPort port(&disco);
    DBOXprotocol proto(&port, "/dev/null");
    disco.handshakeTimeoutMs = 300;

    EXPECT_TRUE(disco.ActiveDiscoverSingle(pty.name, 3));
    stop = true;
    device.join();
}

TEST(LinuxSerial, DiscoverySkipsASilentDevice) {
    Pty pty; ASSERT_FALSE(pty.name.empty());   // nothing answers on this one

    DconfigUart cfg("Urchin", "", 115200, 10);
    UartDiscovery disco(true, &cfg);
    LinuxSerialPort port(&disco);
    DBOXprotocol proto(&port, "/dev/null");
    disco.handshakeTimeoutMs = 100;

    auto t0 = Clock::now();
    EXPECT_FALSE(disco.ActiveDiscoverSingle(pty.name, 3));
    EXPECT_LT(msSince(t0), 2000);
}

TEST(LinuxSerial, FindListsOnlyExistingDevices) {
    LinuxSerialPort port;
    for (const auto& p : port.find()) {
        EXPECT_EQ(p.rfind("/dev/", 0), 0u) << p;
        EXPECT_EQ(::access(p.c_str(), F_OK), 0) << p;
    }
}

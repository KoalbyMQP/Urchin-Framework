//
// Tests for the payload (message) CRC-32 that follows every D-BOX payload.
// Wire format: 0x07 | VPID | Stream | len (2) | header CRC-16 (2) | payload | CRC-32 (4)
//

#include <gtest/gtest.h>

#include <DBOXprotocol.h>
#include <Transport-Interface/UART/Fake/FakeSeralPort.h>
#include <Transport-Interface/UART/Fake/FakeSeralHost.h>
#include <Transport-Interface/UART/Fake/FakeSerialLink.h>

#include <memory>
#include <string>
#include <vector>

using namespace DBOXProtocol;

namespace {
struct Pair {
    std::shared_ptr<FakeSerialLink> link = std::make_shared<FakeSerialLink>("A", "B");
    FakeSerialPort a{link, FakeSeralHost::Side::A};
    FakeSerialPort b{link, FakeSeralHost::Side::B};
    DBOXprotocol protoA{&a, "/dev/null"};
    DBOXprotocol protoB{&b, "/dev/null"};
    Pair() {
        EXPECT_TRUE(a.openraw("A", 115200, 10));
        EXPECT_TRUE(b.openraw("B", 115200, 10));
    }
};

std::vector<uint8_t> wire(const DBOX& p) {
    return static_cast<std::vector<uint8_t>>(DBOXraw(p));
}
}  // namespace

TEST(DBOX_PayloadCRC, MatchesStandardCrc32) {
    // The standard CRC-32 check value (same as Python's zlib.crc32(b"123456789")).
    const std::string check = "123456789";
    std::vector<uint8_t> bytes(check.begin(), check.end());
    EXPECT_EQ(ComputePayloadCRC32(bytes), 0xCBF43926u);
    EXPECT_EQ(ComputePayloadCRC32({}), 0x00000000u);
}

TEST(DBOX_PayloadCRC, AppendedLittleEndianAfterPayload) {
    // 07 | 00 | 'N' | 08 00 | 6A 4A | "Spin 200" | CRC-32 of "Spin 200", little-endian
    std::vector<uint8_t> bytes = wire(DBOX(0, 'N', "Spin 200"));
    ASSERT_EQ(bytes.size(), 7u + 8u + 4u);
    EXPECT_EQ(bytes[5], 0x6A);
    EXPECT_EQ(bytes[6], 0x4A);
    std::vector<uint8_t> payload(bytes.begin() + 7, bytes.begin() + 15);
    uint32_t crc = ComputePayloadCRC32(payload);
    EXPECT_EQ(bytes[15], crc & 0xFF);
    EXPECT_EQ(bytes[16], (crc >> 8) & 0xFF);
    EXPECT_EQ(bytes[17], (crc >> 16) & 0xFF);
    EXPECT_EQ(bytes[18], (crc >> 24) & 0xFF);
}

TEST(DBOX_PayloadCRC, EmptyPayloadStillCarriesCrc) {
    Pair p;
    DBOX sent(5, 'E', "");
    EXPECT_EQ(wire(sent).size(), 7u + 4u);
    ASSERT_TRUE(p.protoB.send(&sent));
    DBOX got;
    ASSERT_TRUE(p.protoA.receive(&got, 1000));
    EXPECT_EQ(got, sent);
}

TEST(DBOX_PayloadCRC, CorruptedPayloadIsDroppedAndNextPacketReceived) {
    Pair p;
    // "Move 90" with one bit flipped -> "Move 98" on the wire. The header CRC
    // still passes; only the CRC-32 can catch this.
    std::vector<uint8_t> bad = wire(DBOX(1, 'N', "Move 90"));
    bad[7 + 6] ^= 0x08;  // '0' (0x30) -> '8' (0x38)
    p.b.rawwrite(bad, bad.size());
    DBOX good(1, 'N', "Move 45");
    ASSERT_TRUE(p.protoB.send(&good));

    DBOX got;
    ASSERT_TRUE(p.protoA.receive(&got, 1000));
    EXPECT_EQ(got, good);
}

TEST(DBOX_PayloadCRC, CorruptedCrcBytesAreDropped) {
    Pair p;
    std::vector<uint8_t> bad = wire(DBOX(2, 'N', "Stop"));
    bad.back() ^= 0x01;
    p.b.rawwrite(bad, bad.size());

    DBOX got;
    EXPECT_FALSE(p.protoA.receive(&got, 200));  // nothing valid arrives
}

TEST(DBOX_PayloadCRC, MissingCrcTimesOut) {
    Pair p;
    // A sender that predates the CRC-32 (no 4 trailing bytes): the receiver
    // waits for them and gives up at the deadline instead of accepting it.
    std::vector<uint8_t> old = wire(DBOX(0, 'N', "Info"));
    old.resize(old.size() - 4);
    p.b.rawwrite(old, old.size());

    DBOX got;
    EXPECT_FALSE(p.protoA.receive(&got, 200));
}

TEST(DBOX_PayloadCRC, ManyPacketsBackToBack) {
    Pair p;
    std::vector<DBOX> sent;
    for (int i = 0; i < 50; ++i) {
        sent.emplace_back(static_cast<uint8_t>(i), 'N', "Spin " + std::to_string(i * 7));
        ASSERT_TRUE(p.protoB.send(&sent.back()));
    }
    for (const DBOX& s : sent) {
        DBOX got;
        ASSERT_TRUE(p.protoA.receive(&got, 1000));
        EXPECT_EQ(got, s);
    }
}

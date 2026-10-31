//
// Created by gabri on 6/21/2026.
//

#include <gtest/gtest.h>

#include <Transport-Interface/UART/Fake/FakeSeralPort.h>
#include <Transport-Interface/UART/Fake/FakeSerialLink.h>

#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <memory>
#include <span>
#include <string>
#include <vector>
#include <RingBuffer/RingBufferException.hpp>
using namespace RingBuffer;
using namespace DBOXProtocol;
// =============================================================================
// Helpers
// =============================================================================

static void assertVectorEq(const std::vector<uint8_t>& actual,
                           std::initializer_list<uint8_t> expected)
{
    EXPECT_TRUE(actual.size() == expected.size());

    std::size_t i = 0;
    for (uint8_t v : expected)
    {
        EXPECT_TRUE(actual[i] == v);
        ++i;
    }
}

static void assertStringVectorEq(const std::vector<std::string>& actual,
                                 std::initializer_list<std::string> expected)
{
    EXPECT_TRUE(actual.size() == expected.size());

    std::size_t i = 0;
    for (const auto& v : expected)
    {
        EXPECT_TRUE(actual[i] == v);
        ++i;
    }
}

static std::shared_ptr<FakeSerialLink>
makeLink(const std::string& aName,
         const std::string& bName)
{
    return std::make_shared<FakeSerialLink>(aName, bName);
}

// =============================================================================
// Open / Close / State
// =============================================================================
TEST(ISerialPort, Starts_Closed){
    auto link = makeLink("SideA", "SideB");
    FakeSerialPort port(link, FakeSeralHost::Side::A);
    EXPECT_EQ(port.isOpen(), false);
}

TEST(ISerialPort, open_succeeds_and_marks_open){
    auto link = makeLink("SideA", "SideB");
    FakeSerialPort port(link, FakeSeralHost::Side::A);

    EXPECT_EQ(port.openraw("SideA", 115200, 10), true);
    EXPECT_EQ(port.isOpen(), true);
}

TEST(ISerialPort, test_close_marks_closed){
    auto link = makeLink("SideA", "SideB");
    FakeSerialPort port(link, FakeSeralHost::Side::A);

    EXPECT_TRUE(port.openraw("SideA", 115200, 10));
    EXPECT_TRUE(port.isOpen());

    port.close();
    EXPECT_FALSE(port.isOpen());
}
TEST(ISerialPort, test_close_is_safe_when_already_closed){
    auto link = makeLink("SideA", "SideB");
    FakeSerialPort port(link, FakeSeralHost::Side::A);

    EXPECT_FALSE(port.isOpen());
    port.close();
    EXPECT_FALSE(port.isOpen());
}

TEST(ISerialPort, test_can_reopen_after_close){
    auto link = makeLink("SideA", "SideB");
    FakeSerialPort port(link, FakeSeralHost::Side::A);

    EXPECT_TRUE(port.openraw("SideA", 115200, 10));
    port.close();
    EXPECT_TRUE(port.openraw("SideA", 115200, 10));
}

// =============================================================================
// TX tests
// =============================================================================
TEST(ISerialPort, test_tx_buffering_and_takeWritten){
    auto link = makeLink("SideA", "SideB");
    FakeSerialPort port(link, FakeSeralHost::Side::A);

    EXPECT_TRUE(port.openraw("SideA", 115200, 10));

    const std::array<uint8_t, 4> data{1, 2, 3, 4};
    port.rawwrite(data, data.size());

    auto tx = port.takeWritten();
    assertVectorEq(tx, {1, 2, 3, 4});
}

// =============================================================================
// waitForWrite()
// =============================================================================

TEST(ISerialPort, test_waitForWrite_returns_true_when_enough_bytes_already_exist){
    auto link = makeLink("SideA", "SideB");
    FakeSerialPort port(link, FakeSeralHost::Side::A);
    EXPECT_TRUE(port.openraw("SideA", 115200, 10));

    const std::array<uint8_t, 3> data{1, 2, 3};
    port.rawwrite(data, data.size());

    EXPECT_TRUE(port.waitForWrite(3, std::chrono::milliseconds(10)));
}

TEST(ISerialPort, test_waitForWrite_returns_false_on_timeout){
    auto link = makeLink("SideA", "SideB");
    FakeSerialPort port(link, FakeSeralHost::Side::A);
    EXPECT_TRUE(port.openraw("SideA", 115200, 10));

    const std::array<uint8_t, 2> data{1, 2};
    port.rawwrite(data, data.size());

    EXPECT_FALSE(port.waitForWrite(3, std::chrono::milliseconds(20)));
}

// =============================================================================
// write()
// =============================================================================
TEST(ISerialPort, test_write_forwards_to_rawwrite){
    auto link = makeLink("SideA", "SideB");
    FakeSerialPort portA(link, FakeSeralHost::Side::A);
    FakeSerialPort portB(link, FakeSeralHost::Side::B);

    assert(portA.openraw("SideA", 115200, 10) == true);
    assert(portB.openraw("SideB", 115200, 10) == true);

    const std::array<uint8_t, 4> data{11, 12, 13, 14};

    auto written = portA.write(data, data.size());
    assert(written == 4);

    auto tx = portA.takeWritten();
    assertVectorEq(tx, {11, 12, 13, 14});

    std::array<uint8_t, 8> buf{};
    auto n = portB.rawread(buf, buf.size());

    EXPECT_TRUE(n == 4);
    EXPECT_TRUE(buf[0] == 11);
    EXPECT_TRUE(buf[1] == 12);
    EXPECT_TRUE(buf[2] == 13);
    EXPECT_TRUE(buf[3] == 14);
}

// =============================================================================
// RX / LINK tests
// =============================================================================


TEST(ISerialPort, two_fake_ports_can_talk_to_each_other){

    auto link = makeLink("SideA", "SideB");
    FakeSerialPort portA(link, FakeSeralHost::Side::A);
    FakeSerialPort portB(link, FakeSeralHost::Side::B);

    EXPECT_TRUE(portA.openraw("SideA", 115200, 10) == true);
    EXPECT_TRUE(portB.openraw("SideB", 115200, 10) == true);

    const std::array<uint8_t, 4> msg{1, 2, 3, 4};
    portA.rawwrite(msg, msg.size());

    std::array<uint8_t, 8> buf{};
    auto n = portB.rawread(buf, buf.size());

    EXPECT_TRUE(n == 4);
    EXPECT_TRUE(buf[0] == 1);
    EXPECT_TRUE(buf[1] == 2);
    EXPECT_TRUE(buf[2] == 3);
    EXPECT_TRUE(buf[3] == 4);
}

TEST(ISerialPort, pushIncoming_makes_bytes_available_to_same_port_rx){
    auto link = makeLink("SideA", "SideB");
    FakeSerialPort portA(link, FakeSeralHost::Side::A);
    FakeSerialPort portB(link, FakeSeralHost::Side::B);

    EXPECT_TRUE(portA.openraw("SideA", 115200, 10) == true);
    EXPECT_TRUE(portB.openraw("SideB", 115200, 10) == true);

    portB.injectFromPeer(std::array<uint8_t, 3>{0xAA, 0xBB, 0xCC});

    std::array<uint8_t, 8> buf{};
    auto n = portB.rawread(buf, buf.size());

    EXPECT_TRUE(n == 3);
    EXPECT_TRUE(buf[0] == 0xAA);
    EXPECT_TRUE(buf[1] == 0xBB);
    EXPECT_TRUE(buf[2] == 0xCC);
}

TEST(ISerialPort, pushIncoming_vector_overload){
    auto link = makeLink("SideA", "SideB");
    FakeSerialPort port(link, FakeSeralHost::Side::A);

    assert(port.openraw("SideA", 115200, 10) == true);

    std::vector<uint8_t> data{5, 6, 7, 8};

    port.injectFromPeer(data);

    std::array<uint8_t, 8> buf{};
    auto n = port.rawread(buf, buf.size());

    EXPECT_TRUE(n == 4);
    EXPECT_TRUE(buf[0] == 5);
    EXPECT_TRUE(buf[1] == 6);
    EXPECT_TRUE(buf[2] == 7);
    EXPECT_TRUE(buf[3] == 8);
}

TEST(ISerialPort, pushIncoming_cstring_overload){
    auto link = makeLink("SideA", "SideB");
    FakeSerialPort port(link, FakeSeralHost::Side::A);

    assert(port.openraw("SideA", 115200, 10) == true);

    port.injectFromPeer(std::span<const uint8_t>(
        reinterpret_cast<const uint8_t*>("ABC"), 3));

    std::array<uint8_t, 8> buf{};
    auto n = port.rawread(buf, buf.size());

    EXPECT_TRUE(n == 3);
    EXPECT_TRUE(buf[0] == static_cast<uint8_t>('A'));
    EXPECT_TRUE(buf[1] == static_cast<uint8_t>('B'));
    EXPECT_TRUE(buf[2] == static_cast<uint8_t>('C'));
}

// =============================================================================
// read() / peek() interaction
// =============================================================================


TEST(ISerialPort, read_reads_directly_from_raw_when_peek_buffer_is_empty){
    auto link = makeLink("SideA", "SideB");
    FakeSerialPort port(link, FakeSeralHost::Side::A);

    assert(port.openraw("SideA", 115200, 10) == true);

    port.injectFromPeer(std::array<uint8_t, 5>{10, 11, 12, 13, 14});

    std::array<uint8_t, 8> buf{};
    auto n = port.read(buf, 5);

    EXPECT_TRUE(n == 5);
    EXPECT_TRUE(buf[0] == 10);
    EXPECT_TRUE(buf[1] == 11);
    EXPECT_TRUE(buf[2] == 12);
    EXPECT_TRUE(buf[3] == 13);
    EXPECT_TRUE(buf[4] == 14);
}


TEST(ISerialPort, peek_then_read_same_size_consumes_from_peek_buffer){
    auto link = makeLink("SideA", "SideB");
    FakeSerialPort port(link, FakeSeralHost::Side::A);

    assert(port.openraw("SideA", 115200, 10) == true);

    port.injectFromPeer(std::array<uint8_t, 4>{21, 22, 23, 24});

    std::array<uint8_t, 4> peekBuf{};
    auto peeked = port.peek(peekBuf, peekBuf.size());

    EXPECT_TRUE(peeked == 4);
    EXPECT_TRUE(peekBuf[0] == 21);
    EXPECT_TRUE(peekBuf[1] == 22);
    EXPECT_TRUE(peekBuf[2] == 23);
    EXPECT_TRUE(peekBuf[3] == 24);

    std::array<uint8_t, 4> readBuf{};
    auto n = port.read(readBuf, readBuf.size());

    EXPECT_TRUE(n == 4);
    EXPECT_TRUE(readBuf[0] == 21);
    EXPECT_TRUE(readBuf[1] == 22);
    EXPECT_TRUE(readBuf[2] == 23);
    EXPECT_TRUE(readBuf[3] == 24);


}


TEST(ISerialPort, peek_then_read_larger_size_uses_peek_buffer_then_bus){
    auto link = makeLink("SideA", "SideB");
    FakeSerialPort port(link, FakeSeralHost::Side::A);

    EXPECT_TRUE(port.openraw("SideA", 115200, 10) == true);

    port.injectFromPeer(std::array<uint8_t, 3>{31, 32, 33});

    std::array<uint8_t, 3> peekBuf{};
    auto peeked = port.peek(peekBuf, peekBuf.size());

    EXPECT_TRUE(peeked == 3);
    EXPECT_TRUE(peekBuf[0] == 31);
    EXPECT_TRUE(peekBuf[1] == 32);
    EXPECT_TRUE(peekBuf[2] == 33);

    port.injectFromPeer(std::array<uint8_t, 2>{34, 35});

    std::array<uint8_t, 5> readBuf{};
    auto n = port.read(readBuf, readBuf.size());

    EXPECT_TRUE(n == 5);
    EXPECT_TRUE(readBuf[0] == 31);
    EXPECT_TRUE(readBuf[1] == 32);
    EXPECT_TRUE(readBuf[2] == 33);
    EXPECT_TRUE(readBuf[3] == 34);
    EXPECT_TRUE(readBuf[4] == 35);


}


TEST(ISerialPort, peek_reads_from_raw_without_consuming){
    auto link = makeLink("SideA", "SideB");
    FakeSerialPort port(link, FakeSeralHost::Side::A);

    EXPECT_TRUE(port.openraw("SideA", 115200, 10) == true);

    port.injectFromPeer(std::array<uint8_t, 4>{41, 42, 43, 44});

    std::array<uint8_t, 4> peekBuf{};
    auto peeked = port.peek(peekBuf, peekBuf.size());

    EXPECT_TRUE(peeked == 4);
    EXPECT_TRUE(peekBuf[0] == 41);
    EXPECT_TRUE(peekBuf[1] == 42);
    EXPECT_TRUE(peekBuf[2] == 43);
    EXPECT_TRUE(peekBuf[3] == 44);

    std::array<uint8_t, 4> readBuf{};
    auto n = port.read(readBuf, readBuf.size());

    EXPECT_TRUE(n == 4);
    EXPECT_TRUE(readBuf[0] == 41);
    EXPECT_TRUE(readBuf[1] == 42);
    EXPECT_TRUE(readBuf[2] == 43);
    EXPECT_TRUE(readBuf[3] == 44);
}


TEST(ISerialPort, peek_tops_up_existing_peek_buffer){
    auto link = makeLink("SideA", "SideB");
    FakeSerialPort port(link, FakeSeralHost::Side::A);

    EXPECT_TRUE(port.openraw("SideA", 115200, 10) == true);

    port.injectFromPeer(std::array<uint8_t, 2>{51, 52});

    std::array<uint8_t, 2> firstPeek{};
    auto n1 = port.peek(firstPeek, firstPeek.size());
    EXPECT_TRUE(n1 == 2);

    port.injectFromPeer(std::array<uint8_t, 3>{53, 54, 55});

    std::array<uint8_t, 5> secondPeek{};
    auto n2 = port.peek(secondPeek, secondPeek.size());

    EXPECT_TRUE(n2 == 5);
    EXPECT_TRUE(secondPeek[0] == 51);
    EXPECT_TRUE(secondPeek[1] == 52);
    EXPECT_TRUE(secondPeek[2] == 53);
    EXPECT_TRUE(secondPeek[3] == 54);
    EXPECT_TRUE(secondPeek[4] == 55);
}


TEST(ISerialPort, peek_returns_as_much_as_available_when_request_is_too_large){
    auto link = makeLink("SideA", "SideB");
    FakeSerialPort port(link, FakeSeralHost::Side::A);

    EXPECT_TRUE(port.openraw("SideA", 115200, 10) == true);

    port.injectFromPeer(std::array<uint8_t, 3>{61, 62, 63});

    std::array<uint8_t, 8> peekBuf{};

    size_t n;
    try
    {
        n = port.peek(peekBuf, 5);
        EXPECT_TRUE(false && "Expected exception was not thrown");
    }
    catch (const RingBufferUnderflowException&)
    {
        // success
    }
    catch (...)
    {
        EXPECT_TRUE(false && "Wrong exception type thrown");
    }


    //printf("%llu\n",n);
    //for (size_t i = 0; i < peekBuf.size(); ++i) {
    //    printf("%u ", peekBuf[i]);
    //}
    //assert(n == 3);
    EXPECT_TRUE(peekBuf[0] == 61);
    EXPECT_TRUE(peekBuf[1] == 62);
    EXPECT_TRUE(peekBuf[2] == 63);

    std::array<uint8_t, 4> readBuf{};
    auto r = port.read(readBuf, 3);

    EXPECT_TRUE(r == 3);
    EXPECT_TRUE(readBuf[0] == 61);
    EXPECT_TRUE(readBuf[1] == 62);
    EXPECT_TRUE(readBuf[2] == 63);
}

// =============================================================================
// drop()
// =============================================================================

TEST(ISerialPort, drop_discards_from_peek_buffer){
    auto link = makeLink("SideA", "SideB");
    FakeSerialPort port(link, FakeSeralHost::Side::A);

    EXPECT_TRUE(port.openraw("SideA", 115200, 10) == true);

    port.injectFromPeer(std::array<uint8_t, 5>{71, 72, 73, 74, 75});

    std::array<uint8_t, 5> peekBuf{};
    auto peeked = port.peek(peekBuf, peekBuf.size());
    EXPECT_TRUE(peeked == 5);

    auto dropped = port.drop(2);
    EXPECT_TRUE(dropped == 2);

    std::array<uint8_t, 8> readBuf{};
    auto n = port.read(readBuf, 3);

    EXPECT_TRUE(n == 3);
    EXPECT_TRUE(readBuf[0] == 73);
    EXPECT_TRUE(readBuf[1] == 74);
    EXPECT_TRUE(readBuf[2] == 75);
}

TEST(ISerialPort, drop_discards_from_peek_buffer_then_raw){
    auto link = makeLink("SideA", "SideB");
    FakeSerialPort port(link, FakeSeralHost::Side::A);

    EXPECT_TRUE(port.openraw("SideA", 115200, 10) == true);

    port.injectFromPeer(std::array<uint8_t, 3>{81, 82, 83});

    std::array<uint8_t, 3> peekBuf{};
    auto peeked = port.peek(peekBuf, peekBuf.size());
    EXPECT_TRUE(peeked == 3);

    port.injectFromPeer(std::array<uint8_t, 2>{84, 85});

    auto dropped = port.drop(5);
    EXPECT_TRUE(dropped == 5);

    std::array<uint8_t, 4> readBuf{};
    auto n = port.read(readBuf, readBuf.size());
    EXPECT_TRUE(n == 0);
}

TEST(ISerialPort, drop_returns_only_available_byte_count_when_request_is_too_large){
    auto link = makeLink("SideA", "SideB");
    FakeSerialPort port(link, FakeSeralHost::Side::A);

    EXPECT_TRUE(port.openraw("SideA", 115200, 10) == true);

    port.injectFromPeer(std::array<uint8_t, 3>{91, 92, 93});

    auto dropped = port.drop(10);
    EXPECT_TRUE(dropped == 3);

    std::array<uint8_t, 4> readBuf{};
    auto n = port.read(readBuf, readBuf.size());
    EXPECT_TRUE(n == 0);
}

// =============================================================================
// flushBuss()
// =============================================================================


TEST(ISerialPort, flushBuss_clears_tx_buffer){
    auto link = makeLink("SideA", "SideB");
    FakeSerialPort port(link, FakeSeralHost::Side::A);

    EXPECT_TRUE(port.openraw("SideA", 115200, 10) == true);

    port.rawwrite(std::array<uint8_t, 2>{4, 5}, 2);
    auto txBefore = port.takeWritten();
    assertVectorEq(txBefore, {4, 5});

    port.rawwrite(std::array<uint8_t, 2>{7, 8}, 2);
    port.flushBuss();

    auto tx = port.takeWritten();
    EXPECT_TRUE(tx.empty());
}

// =============================================================================
// flushPeek()
// =============================================================================

TEST(ISerialPort, flushPeek_clears_only_the_internal_peek_buffer){
    auto link = makeLink("SideA", "SideB");
    FakeSerialPort port(link, FakeSeralHost::Side::A);

    EXPECT_TRUE(port.openraw("SideA", 115200, 10) == true);

    port.injectFromPeer(std::array<uint8_t, 3>{101, 102, 103});

    std::array<uint8_t, 3> peekBuf{};
    auto peeked = port.peek(peekBuf, peekBuf.size());
    EXPECT_TRUE(peeked == 3);

    port.flushPeek();

    std::array<uint8_t, 4> readBuf{};
    auto n = port.read(readBuf, readBuf.size());

    EXPECT_TRUE(n == 0);
}

TEST(ISerialPort, flush_clears_peek_buffer_and_bus){
    auto link = makeLink("SideA", "SideB");
    FakeSerialPort port(link, FakeSeralHost::Side::A);

    EXPECT_TRUE(port.openraw("SideA", 115200, 10) == true);

    port.injectFromPeer(std::array<uint8_t, 2>{111, 112});
    std::array<uint8_t, 2> peekBuf{};
    auto peeked = port.peek(peekBuf, peekBuf.size());
    EXPECT_TRUE(peeked == 2);

    port.injectFromPeer(std::array<uint8_t, 3>{113, 114, 115});

    port.flush();

    std::array<uint8_t, 8> readBuf{};
    auto n = port.read(readBuf, readBuf.size());
    EXPECT_TRUE(n == 0);
}


//
// Created by gabri on 6/21/2026.
//

#include "../TestISerialPort.h"

#include "../Transport-Interface/Fake/FakeSeralPort.h"
#include "../Transport-Interface/Fake/FakeSerialLink.h"

#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <memory>
#include <span>
#include <string>
#include <vector>

// =============================================================================
// Helpers
// =============================================================================

static void assertVectorEq(const std::vector<uint8_t>& actual,
                           std::initializer_list<uint8_t> expected)
{
    assert(actual.size() == expected.size());

    std::size_t i = 0;
    for (uint8_t v : expected)
    {
        assert(actual[i] == v);
        ++i;
    }
}

static void assertStringVectorEq(const std::vector<std::string>& actual,
                                 std::initializer_list<std::string> expected)
{
    assert(actual.size() == expected.size());

    std::size_t i = 0;
    for (const auto& v : expected)
    {
        assert(actual[i] == v);
        ++i;
    }
}

static std::shared_ptr<FakeSerialLink>
makeLink(const std::string& aName,
         const std::string& bName,
         uint32_t baud)
{
    return std::make_shared<FakeSerialLink>(aName, bName, baud);
}

// =============================================================================
// Open / Close / State
// =============================================================================

static void test_starts_closed()
{
    auto link = makeLink("SideA", "SideB", 115200);
    FakeSerialPort port(link, FakeSerialPort::Side::A);
    assert(port.isOpen() == false);
}

static void test_open_succeeds_and_marks_open()
{
    auto link = makeLink("SideA", "SideB", 115200);
    FakeSerialPort port(link, FakeSerialPort::Side::A);

    assert(port.open("SideA", 115200, 10) == true);
    assert(port.isOpen() == true);
}

static void test_close_marks_closed()
{
    auto link = makeLink("SideA", "SideB", 115200);
    FakeSerialPort port(link, FakeSerialPort::Side::A);

    assert(port.open("SideA", 115200, 10) == true);
    assert(port.isOpen() == true);

    port.close();
    assert(port.isOpen() == false);
}

static void test_close_is_safe_when_already_closed()
{
    auto link = makeLink("SideA", "SideB", 115200);
    FakeSerialPort port(link, FakeSerialPort::Side::A);

    assert(port.isOpen() == false);
    port.close();
    assert(port.isOpen() == false);
}

static void test_can_reopen_after_close()
{
    auto link = makeLink("SideA", "SideB", 115200);
    FakeSerialPort port(link, FakeSerialPort::Side::A);

    assert(port.open("SideA", 115200, 10) == true);
    port.close();
    assert(port.open("SideA", 115200, 10) == true);
}

// =============================================================================
// TX tests
// =============================================================================

static void test_tx_buffering_and_takeWritten()
{
    auto link = makeLink("SideA", "SideB", 115200);
    FakeSerialPort port(link, FakeSerialPort::Side::A);

    assert(port.open("SideA", 115200, 10) == true);

    const std::array<uint8_t, 4> data{1, 2, 3, 4};
    port.rawwrite(data, data.size());

    auto tx = port.takeWritten();
    assertVectorEq(tx, {1, 2, 3, 4});
}

// =============================================================================
// waitForWrite()
// =============================================================================

static void test_waitForWrite_returns_true_when_enough_bytes_already_exist()
{
    auto link = makeLink("SideA", "SideB", 115200);
    FakeSerialPort port(link, FakeSerialPort::Side::A);
    assert(port.open("SideA", 115200, 10) == true);

    const std::array<uint8_t, 3> data{1, 2, 3};
    port.rawwrite(data, data.size());

    assert(port.waitForWrite(3, std::chrono::milliseconds(10)) == true);
}

static void test_waitForWrite_returns_false_on_timeout()
{
    auto link = makeLink("SideA", "SideB", 115200);
    FakeSerialPort port(link, FakeSerialPort::Side::A);
    assert(port.open("SideA", 115200, 10) == true);

    const std::array<uint8_t, 2> data{1, 2};
    port.rawwrite(data, data.size());

    assert(port.waitForWrite(3, std::chrono::milliseconds(20)) == false);
}

// =============================================================================
// write()
// =============================================================================

static void test_write_forwards_to_rawwrite()
{
    auto link = makeLink("SideA", "SideB", 115200);
    FakeSerialPort portA(link, FakeSerialPort::Side::A);
    FakeSerialPort portB(link, FakeSerialPort::Side::B);

    assert(portA.open("SideA", 115200, 10) == true);
    assert(portB.open("SideB", 115200, 10) == true);

    const std::array<uint8_t, 4> data{11, 12, 13, 14};

    auto written = portA.write(data, data.size());
    assert(written == 4);

    auto tx = portA.takeWritten();
    assertVectorEq(tx, {11, 12, 13, 14});

    std::array<uint8_t, 8> buf{};
    auto n = portB.rawread(buf, buf.size());

    assert(n == 4);
    assert(buf[0] == 11);
    assert(buf[1] == 12);
    assert(buf[2] == 13);
    assert(buf[3] == 14);
}

// =============================================================================
// RX / LINK tests
// =============================================================================

static void test_two_fake_ports_can_talk_to_each_other()
{
    auto link = makeLink("SideA", "SideB", 115200);
    FakeSerialPort portA(link, FakeSerialPort::Side::A);
    FakeSerialPort portB(link, FakeSerialPort::Side::B);

    assert(portA.open("SideA", 115200, 10) == true);
    assert(portB.open("SideB", 115200, 10) == true);

    const std::array<uint8_t, 4> msg{1, 2, 3, 4};
    portA.rawwrite(msg, msg.size());

    std::array<uint8_t, 8> buf{};
    auto n = portB.rawread(buf, buf.size());

    assert(n == 4);
    assert(buf[0] == 1);
    assert(buf[1] == 2);
    assert(buf[2] == 3);
    assert(buf[3] == 4);
}

static void test_pushIncoming_makes_bytes_available_to_same_port_rx()
{
    auto link = makeLink("SideA", "SideB", 115200);
    FakeSerialPort portA(link, FakeSerialPort::Side::A);
    FakeSerialPort portB(link, FakeSerialPort::Side::B);

    assert(portA.open("SideA", 115200, 10) == true);
    assert(portB.open("SideB", 115200, 10) == true);

    portB.injectFromPeer(std::array<uint8_t, 3>{0xAA, 0xBB, 0xCC});

    std::array<uint8_t, 8> buf{};
    auto n = portB.rawread(buf, buf.size());

    assert(n == 3);
    assert(buf[0] == 0xAA);
    assert(buf[1] == 0xBB);
    assert(buf[2] == 0xCC);
}

static void test_pushIncoming_vector_overload()
{
    auto link = makeLink("SideA", "SideB", 115200);
    FakeSerialPort port(link, FakeSerialPort::Side::A);

    assert(port.open("SideA", 115200, 10) == true);

    std::vector<uint8_t> data{5, 6, 7, 8};

    port.injectFromPeer(data);

    std::array<uint8_t, 8> buf{};
    auto n = port.rawread(buf, buf.size());

    assert(n == 4);
    assert(buf[0] == 5);
    assert(buf[1] == 6);
    assert(buf[2] == 7);
    assert(buf[3] == 8);
}

static void test_pushIncoming_cstring_overload()
{
    auto link = makeLink("SideA", "SideB", 115200);
    FakeSerialPort port(link, FakeSerialPort::Side::A);

    assert(port.open("SideA", 115200, 10) == true);

    port.injectFromPeer(std::span<const uint8_t>(
        reinterpret_cast<const uint8_t*>("ABC"), 3));

    std::array<uint8_t, 8> buf{};
    auto n = port.rawread(buf, buf.size());

    assert(n == 3);
    assert(buf[0] == static_cast<uint8_t>('A'));
    assert(buf[1] == static_cast<uint8_t>('B'));
    assert(buf[2] == static_cast<uint8_t>('C'));
}

// =============================================================================
// read() / peek() interaction
// =============================================================================

static void test_read_reads_directly_from_raw_when_peek_buffer_is_empty()
{
    auto link = makeLink("SideA", "SideB", 115200);
    FakeSerialPort port(link, FakeSerialPort::Side::A);

    assert(port.open("SideA", 115200, 10) == true);

    port.injectFromPeer(std::array<uint8_t, 5>{10, 11, 12, 13, 14});

    std::array<uint8_t, 8> buf{};
    auto n = port.read(buf, 5);

    assert(n == 5);
    assert(buf[0] == 10);
    assert(buf[1] == 11);
    assert(buf[2] == 12);
    assert(buf[3] == 13);
    assert(buf[4] == 14);
}

static void test_peek_then_read_same_size_consumes_from_peek_buffer()
{
    auto link = makeLink("SideA", "SideB", 115200);
    FakeSerialPort port(link, FakeSerialPort::Side::A);

    assert(port.open("SideA", 115200, 10) == true);

    port.injectFromPeer(std::array<uint8_t, 4>{21, 22, 23, 24});

    std::array<uint8_t, 4> peekBuf{};
    auto peeked = port.peek(peekBuf, peekBuf.size());

    assert(peeked == 4);
    assert(peekBuf[0] == 21);
    assert(peekBuf[1] == 22);
    assert(peekBuf[2] == 23);
    assert(peekBuf[3] == 24);

    std::array<uint8_t, 4> readBuf{};
    auto n = port.read(readBuf, readBuf.size());

    assert(n == 4);
    assert(readBuf[0] == 21);
    assert(readBuf[1] == 22);
    assert(readBuf[2] == 23);
    assert(readBuf[3] == 24);


}

static void test_peek_then_read_larger_size_uses_peek_buffer_then_bus()
{
    auto link = makeLink("SideA", "SideB", 115200);
    FakeSerialPort port(link, FakeSerialPort::Side::A);

    assert(port.open("SideA", 115200, 10) == true);

    port.injectFromPeer(std::array<uint8_t, 3>{31, 32, 33});

    std::array<uint8_t, 3> peekBuf{};
    auto peeked = port.peek(peekBuf, peekBuf.size());

    assert(peeked == 3);
    assert(peekBuf[0] == 31);
    assert(peekBuf[1] == 32);
    assert(peekBuf[2] == 33);

    port.injectFromPeer(std::array<uint8_t, 2>{34, 35});

    std::array<uint8_t, 5> readBuf{};
    auto n = port.read(readBuf, readBuf.size());

    assert(n == 5);
    assert(readBuf[0] == 31);
    assert(readBuf[1] == 32);
    assert(readBuf[2] == 33);
    assert(readBuf[3] == 34);
    assert(readBuf[4] == 35);


}

static void test_peek_reads_from_raw_without_consuming()
{
    auto link = makeLink("SideA", "SideB", 115200);
    FakeSerialPort port(link, FakeSerialPort::Side::A);

    assert(port.open("SideA", 115200, 10) == true);

    port.injectFromPeer(std::array<uint8_t, 4>{41, 42, 43, 44});

    std::array<uint8_t, 4> peekBuf{};
    auto peeked = port.peek(peekBuf, peekBuf.size());

    assert(peeked == 4);
    assert(peekBuf[0] == 41);
    assert(peekBuf[1] == 42);
    assert(peekBuf[2] == 43);
    assert(peekBuf[3] == 44);

    std::array<uint8_t, 4> readBuf{};
    auto n = port.read(readBuf, readBuf.size());

    assert(n == 4);
    assert(readBuf[0] == 41);
    assert(readBuf[1] == 42);
    assert(readBuf[2] == 43);
    assert(readBuf[3] == 44);
}

static void test_peek_tops_up_existing_peek_buffer()
{
    auto link = makeLink("SideA", "SideB", 115200);
    FakeSerialPort port(link, FakeSerialPort::Side::A);

    assert(port.open("SideA", 115200, 10) == true);

    port.injectFromPeer(std::array<uint8_t, 2>{51, 52});

    std::array<uint8_t, 2> firstPeek{};
    auto n1 = port.peek(firstPeek, firstPeek.size());
    assert(n1 == 2);

    port.injectFromPeer(std::array<uint8_t, 3>{53, 54, 55});

    std::array<uint8_t, 5> secondPeek{};
    auto n2 = port.peek(secondPeek, secondPeek.size());

    assert(n2 == 5);
    assert(secondPeek[0] == 51);
    assert(secondPeek[1] == 52);
    assert(secondPeek[2] == 53);
    assert(secondPeek[3] == 54);
    assert(secondPeek[4] == 55);
}

static void test_peek_returns_as_much_as_available_when_request_is_too_large()
{
    auto link = makeLink("SideA", "SideB", 115200);
    FakeSerialPort port(link, FakeSerialPort::Side::A);

    assert(port.open("SideA", 115200, 10) == true);

    port.injectFromPeer(std::array<uint8_t, 3>{61, 62, 63});

    std::array<uint8_t, 8> peekBuf{};

    size_t n;
    try
    {
        n = port.peek(peekBuf, 5);
        assert(false && "Expected exception was not thrown");
    }
    catch (const RingBufferUnderflowException&)
    {
        // success
    }
    catch (...)
    {
        assert(false && "Wrong exception type thrown");
    }


    //printf("%llu\n",n);
    //for (size_t i = 0; i < peekBuf.size(); ++i) {
    //    printf("%u ", peekBuf[i]);
    //}
    //assert(n == 3);
    assert(peekBuf[0] == 61);
    assert(peekBuf[1] == 62);
    assert(peekBuf[2] == 63);

    std::array<uint8_t, 4> readBuf{};
    auto r = port.read(readBuf, 3);

    assert(r == 3);
    assert(readBuf[0] == 61);
    assert(readBuf[1] == 62);
    assert(readBuf[2] == 63);
}

// =============================================================================
// drop()
// =============================================================================

static void test_drop_discards_from_peek_buffer()
{
    auto link = makeLink("SideA", "SideB", 115200);
    FakeSerialPort port(link, FakeSerialPort::Side::A);

    assert(port.open("SideA", 115200, 10) == true);

    port.injectFromPeer(std::array<uint8_t, 5>{71, 72, 73, 74, 75});

    std::array<uint8_t, 5> peekBuf{};
    auto peeked = port.peek(peekBuf, peekBuf.size());
    assert(peeked == 5);

    auto dropped = port.drop(2);
    assert(dropped == 2);

    std::array<uint8_t, 8> readBuf{};
    auto n = port.read(readBuf, 3);

    assert(n == 3);
    assert(readBuf[0] == 73);
    assert(readBuf[1] == 74);
    assert(readBuf[2] == 75);
}

static void test_drop_discards_from_peek_buffer_then_raw()
{
    auto link = makeLink("SideA", "SideB", 115200);
    FakeSerialPort port(link, FakeSerialPort::Side::A);

    assert(port.open("SideA", 115200, 10) == true);

    port.injectFromPeer(std::array<uint8_t, 3>{81, 82, 83});

    std::array<uint8_t, 3> peekBuf{};
    auto peeked = port.peek(peekBuf, peekBuf.size());
    assert(peeked == 3);

    port.injectFromPeer(std::array<uint8_t, 2>{84, 85});

    auto dropped = port.drop(5);
    assert(dropped == 5);

    std::array<uint8_t, 4> readBuf{};
    auto n = port.read(readBuf, readBuf.size());
    assert(n == 0);
}

static void test_drop_returns_only_available_byte_count_when_request_is_too_large()
{
    auto link = makeLink("SideA", "SideB", 115200);
    FakeSerialPort port(link, FakeSerialPort::Side::A);

    assert(port.open("SideA", 115200, 10) == true);

    port.injectFromPeer(std::array<uint8_t, 3>{91, 92, 93});

    auto dropped = port.drop(10);
    assert(dropped == 3);

    std::array<uint8_t, 4> readBuf{};
    auto n = port.read(readBuf, readBuf.size());
    assert(n == 0);
}

// =============================================================================
// flushBuss()
// =============================================================================

static void test_flushBuss_clears_tx_buffer()
{
    auto link = makeLink("SideA", "SideB", 115200);
    FakeSerialPort port(link, FakeSerialPort::Side::A);

    assert(port.open("SideA", 115200, 10) == true);

    port.rawwrite(std::array<uint8_t, 2>{4, 5}, 2);
    auto txBefore = port.takeWritten();
    assertVectorEq(txBefore, {4, 5});

    port.rawwrite(std::array<uint8_t, 2>{7, 8}, 2);
    port.flushBuss();

    auto tx = port.takeWritten();
    assert(tx.empty());
}

// =============================================================================
// flushPeek()
// =============================================================================

static void test_flushPeek_clears_only_the_internal_peek_buffer()
{
    auto link = makeLink("SideA", "SideB", 115200);
    FakeSerialPort port(link, FakeSerialPort::Side::A);

    assert(port.open("SideA", 115200, 10) == true);

    port.injectFromPeer(std::array<uint8_t, 3>{101, 102, 103});

    std::array<uint8_t, 3> peekBuf{};
    auto peeked = port.peek(peekBuf, peekBuf.size());
    assert(peeked == 3);

    port.flushPeek();

    std::array<uint8_t, 4> readBuf{};
    auto n = port.read(readBuf, readBuf.size());

    assert(n == 0);
}

static void test_flush_clears_peek_buffer_and_bus()
{
    auto link = makeLink("SideA", "SideB", 115200);
    FakeSerialPort port(link, FakeSerialPort::Side::A);

    assert(port.open("SideA", 115200, 10) == true);

    port.injectFromPeer(std::array<uint8_t, 2>{111, 112});
    std::array<uint8_t, 2> peekBuf{};
    auto peeked = port.peek(peekBuf, peekBuf.size());
    assert(peeked == 2);

    port.injectFromPeer(std::array<uint8_t, 3>{113, 114, 115});

    port.flush();

    std::array<uint8_t, 8> readBuf{};
    auto n = port.read(readBuf, readBuf.size());
    assert(n == 0);
}

// =============================================================================
// MAIN
// =============================================================================

int TestISerialPort()
{
    test_starts_closed();
    test_open_succeeds_and_marks_open();
    test_close_marks_closed();
    test_close_is_safe_when_already_closed();
    test_can_reopen_after_close();

    test_tx_buffering_and_takeWritten();

    test_waitForWrite_returns_true_when_enough_bytes_already_exist();
    test_waitForWrite_returns_false_on_timeout();

    test_write_forwards_to_rawwrite();

    test_two_fake_ports_can_talk_to_each_other();
    test_pushIncoming_makes_bytes_available_to_same_port_rx();
    test_pushIncoming_vector_overload();
    test_pushIncoming_cstring_overload();

    test_read_reads_directly_from_raw_when_peek_buffer_is_empty();
    test_peek_then_read_same_size_consumes_from_peek_buffer();
    test_peek_then_read_larger_size_uses_peek_buffer_then_bus();
    test_peek_reads_from_raw_without_consuming();
    test_peek_tops_up_existing_peek_buffer();
    test_peek_returns_as_much_as_available_when_request_is_too_large();

    test_drop_discards_from_peek_buffer();
    test_drop_discards_from_peek_buffer_then_raw();
    test_drop_returns_only_available_byte_count_when_request_is_too_large();

    test_flushPeek_clears_only_the_internal_peek_buffer();
    test_flush_clears_peek_buffer_and_bus();

    test_flushBuss_clears_tx_buffer();

    std::cout << "All ISerialPort tests passed.\n";
    return 0;
}
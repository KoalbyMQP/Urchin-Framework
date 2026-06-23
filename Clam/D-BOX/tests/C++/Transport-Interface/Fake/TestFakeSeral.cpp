//
// Created by gabri on 6/21/2026.
//




#include "TestFakeSeral.h"

#include "../../../Transport-Interface/Fake/FakeSeralPort.h"
#include "../../../Transport-Interface/Fake/FakeSerialLink.h"

#include <array>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <thread>
#include <vector>
#include <iostream>

using namespace std::chrono_literals;

// =============================================================
// Helpers
// =============================================================


static std::vector<uint8_t> b(std::initializer_list<uint8_t> v)
{
    return std::vector<uint8_t>(v);
}

static std::span<const uint8_t> sp(const std::vector<uint8_t>& v)
{
    return std::span<const uint8_t>(v.data(), v.size());
}

static void assertVectorEq(const std::vector<uint8_t>& actual,
                           std::initializer_list<uint8_t> expected)
{
    assert(actual.size() == expected.size());

    size_t i = 0;
    for (auto v : expected)
        assert(actual[i++] == v);
}

static std::vector<uint8_t> spanToVec(const uint8_t* data, size_t n)
{
    return std::vector<uint8_t>(data, data + n);
}

static std::shared_ptr<FakeSerialLink>
makeLink(const std::string& a, const std::string& b, uint32_t baud)
{
    return std::make_shared<FakeSerialLink>(a, b, baud);
}

// =============================================================
// 1. Lifecycle
// =============================================================

static void test_lifecycle()
{
    auto link = makeLink("A","B",115200);
    FakeSerialPort port(link, FakeSerialPort::Side::A);

    assert(!port.isOpen());

    assert(port.open("A",115200,50));
    assert(port.isOpen());

    port.close();
    assert(!port.isOpen());

    assert(port.open("A",115200,50));
    assert(port.isOpen());
}

// =============================================================
// 2. Basic communication
// =============================================================

static void test_A_to_B_basic()
{
    auto link = makeLink("A","B",115200);

    FakeSerialPort A(link, FakeSerialPort::Side::A);
    FakeSerialPort B(link, FakeSerialPort::Side::B);

    assert(A.open("A",115200,50));
    assert(B.open("B",115200,50));

    auto data = b({1,2,3});
    A.rawwrite(sp(data), data.size());

    uint8_t buf[10]{};
    auto n = B.rawread(buf, 10);

    assert(n == 3);
    assertVectorEq(spanToVec(buf,n), {1,2,3});
}

// =============================================================
// 3. Bidirectional communication
// =============================================================

static void test_bidirectional()
{
    auto link = makeLink("A","B",115200);

    FakeSerialPort A(link, FakeSerialPort::Side::A);
    FakeSerialPort B(link, FakeSerialPort::Side::B);

    assert(A.open("A",115200,50));
    assert(B.open("B",115200,50));

    auto dataA = b({1,2});
    A.rawwrite(sp(dataA), dataA.size());

    auto dataB = b({3,4});
    B.rawwrite(sp(dataB), dataB.size());

    uint8_t aBuf[10]{}, bBuf[10]{};

    auto na = A.rawread(aBuf, 10);
    auto nb = B.rawread(bBuf, 10);

    assertVectorEq(spanToVec(aBuf,na), {3,4});
    assertVectorEq(spanToVec(bBuf,nb), {1,2});
}

// =============================================================
// 4. injectFromPeer
// =============================================================

static void test_inject()
{
    auto link = makeLink("A","B",115200);

    FakeSerialPort A(link, FakeSerialPort::Side::A);
    assert(A.open("A",115200,50));
    auto data = b({9,8,7});
    A.injectFromPeer(sp(data));

    uint8_t buf[10]{};
    auto n = A.rawread(buf, 10);

    assertVectorEq(spanToVec(buf,n), {9,8,7});
}

// =============================================================
// 5. mixed real + injected
// =============================================================

static void test_mixed()
{
    auto link = makeLink("A","B",115200);

    FakeSerialPort A(link, FakeSerialPort::Side::A);
    FakeSerialPort B(link, FakeSerialPort::Side::B);

    assert(A.open("A",115200,50));
    assert(B.open("B",115200,50));

    auto data1 = b({1,2});
    A.rawwrite(sp(data1), data1.size());

    auto data2 = b({9,9});
    A.injectFromPeer(sp(data1));

    auto data3 = b({3,4});
    A.rawwrite(sp(data3), data3.size());

    uint8_t buf[10]{};
    auto n = B.rawread(buf, 10);

    assert(n == 4);
}

// =============================================================
// 6. partial reads
// =============================================================

static void test_partial_reads()
{
    auto link = makeLink("A","B",115200);

    FakeSerialPort A(link, FakeSerialPort::Side::A);
    FakeSerialPort B(link, FakeSerialPort::Side::B);

    assert(A.open("A",115200,50));
    assert(B.open("B",115200,50));

    auto data1 = b({1,2,3,4});
    A.rawwrite(sp(data1), data1.size());

    uint8_t buf[2]{};

    auto n1 = B.rawread(buf, 2);
    auto n2 = B.rawread(buf, 2);

    assert(n1 == 2);
    assert(n2 == 2);
}

// =============================================================
// 7. timeout
// =============================================================

static void test_timeout()
{
    auto link = makeLink("A","B",115200);

    FakeSerialPort A(link, FakeSerialPort::Side::A);
    assert(A.open("A",115200,20));

    uint8_t buf[10]{};

    auto start = std::chrono::steady_clock::now();
    auto n = A.rawread(buf, 10);
    auto end = std::chrono::steady_clock::now();

    assert(n == 0);

    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end-start).count();
    assert(ms >= 20);
}

// =============================================================
// 8. flush
// =============================================================

static void test_flush()
{
    auto link = makeLink("A","B",115200);

    FakeSerialPort A(link, FakeSerialPort::Side::A);
    FakeSerialPort B(link, FakeSerialPort::Side::B);

    assert(A.open("A",115200,50));
    assert(B.open("B",115200,50));

    auto dataA = b({1,2,3});
    A.rawwrite(sp(dataA), dataA.size());
    B.flushBuss();

    uint8_t buf[10]{};
    auto n = B.rawread(buf, 10);

    assert(n == 0);
}

// =============================================================
// 9. concurrency smoke
// =============================================================

static void test_concurrency()
{
    auto link = makeLink("A","B",115200);

    FakeSerialPort A(link, FakeSerialPort::Side::A);
    FakeSerialPort B(link, FakeSerialPort::Side::B);

    assert(A.open("A",115200,100));
    assert(B.open("B",115200,100));

    std::atomic<size_t> bytesWritten{0};
    std::atomic<size_t> bytesRead{0};

    constexpr size_t targetBytes = 300; // 100 * 3 bytes

    // ---------------- writer ----------------
    std::thread writer([&]()
    {
        auto dataA = b({1,2,3});

        for (int i = 0; i < 100; i++)
        {
            A.rawwrite(sp(dataA), dataA.size());
            bytesWritten += dataA.size();
        }
    });

    // ---------------- reader ----------------
    std::thread reader([&]()
    {
        uint8_t buf[10];

        while (bytesRead.load() < targetBytes)
        {
            auto n = B.rawread(buf, sizeof(buf));

            if (n > 0)
                bytesRead += n;
        }
    });

    writer.join();
    reader.join();

    assert(bytesWritten == targetBytes);
    assert(bytesRead == targetBytes);
}

// =============================================================
// MAIN
// =============================================================

int TestFakeSeral()
{
    test_lifecycle();
    test_A_to_B_basic();
    test_bidirectional();
    test_inject();
    test_mixed();
    test_partial_reads();
    test_timeout();
    test_flush();
    test_concurrency();

    std::cout << "All FakeSerialPort tests passed.\n";
    return 0;
}
//
// Created by gabri on 6/21/2026.
//

#include <gtest/gtest.h>

#include <Transport-Interface/UART/Fake/FakeSeralPort.h>
#include <Transport-Interface/UART/Fake/FakeSerialLink.h>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <thread>
#include <vector>

using namespace DBOXProtocol;
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
    return {v.data(), v.size()};
}

static std::vector<uint8_t> spanToVec(const uint8_t* data, size_t n)
{
    return {data, data + n};
}

static std::shared_ptr< DBOXProtocol::FakeSerialLink>
makeLink(const std::string& a, const std::string& b)
{
    return std::make_shared< DBOXProtocol::FakeSerialLink>(a, b);
}

// =============================================================

TEST(FakeSerialPortTest, Lifecycle)
{
    auto link = makeLink("A", "B");
    FakeSerialPort port(link, FakeSeralHost::Side::A);

    EXPECT_FALSE(port.isOpen());

    EXPECT_TRUE(port.openraw("A", 115200, 50));
    EXPECT_TRUE(port.isOpen());

    port.close();
    EXPECT_FALSE(port.isOpen());

    EXPECT_TRUE(port.openraw("A", 115200, 50));
    EXPECT_TRUE(port.isOpen());
}

TEST(FakeSerialPortTest, BasicCommunicationAtoB)
{
    auto link = makeLink("A", "B");

    FakeSerialPort A(link, FakeSeralHost::Side::A);
    FakeSerialPort B(link, FakeSeralHost::Side::B);

    ASSERT_TRUE(A.openraw("A", 115200, 50));
    ASSERT_TRUE(B.openraw("B", 115200, 50));

    auto data = b({1, 2, 3});
    A.rawwrite(sp(data), data.size());

    uint8_t buf[10]{};
    auto n = B.rawread(buf, 10);

    EXPECT_EQ(n, 3u);
    EXPECT_EQ(spanToVec(buf, n), b({1, 2, 3}));
}

TEST(FakeSerialPortTest, BidirectionalCommunication)
{
    auto link = makeLink("A", "B");

    FakeSerialPort A(link, FakeSeralHost::Side::A);
    FakeSerialPort B(link, FakeSeralHost::Side::B);

    ASSERT_TRUE(A.openraw("A", 115200, 50));
    ASSERT_TRUE(B.openraw("B", 115200, 50));

    auto dataA = b({1, 2});
    auto dataB = b({3, 4});

    A.rawwrite(sp(dataA), dataA.size());
    B.rawwrite(sp(dataB), dataB.size());

    uint8_t aBuf[10]{};
    uint8_t bBuf[10]{};

    auto na = A.rawread(aBuf, sizeof(aBuf));
    auto nb = B.rawread(bBuf, sizeof(bBuf));

    EXPECT_EQ(spanToVec(aBuf, na), b({3, 4}));
    EXPECT_EQ(spanToVec(bBuf, nb), b({1, 2}));
}

TEST(FakeSerialPortTest, InjectFromPeer)
{
    auto link = makeLink("A", "B");

    FakeSerialPort A(link, FakeSeralHost::Side::A);

    ASSERT_TRUE(A.openraw("A", 115200, 50));

    auto data = b({9, 8, 7});
    A.injectFromPeer(sp(data));

    uint8_t buf[10]{};

    auto n = A.rawread(buf, 10);

    EXPECT_EQ(spanToVec(buf, n), b({9, 8, 7}));
}

TEST(FakeSerialPortTest, MixedTraffic)
{
    auto link = makeLink("A", "B");

    FakeSerialPort A(link, FakeSeralHost::Side::A);
    FakeSerialPort B(link, FakeSeralHost::Side::B);

    ASSERT_TRUE(A.openraw("A", 115200, 50));
    ASSERT_TRUE(B.openraw("B", 115200, 50));

    A.rawwrite(sp(b({1, 2})), 2);
    A.injectFromPeer(sp(b({9, 9})));
    A.rawwrite(sp(b({3, 4})), 2);

    uint8_t buf[10]{};

    auto n = B.rawread(buf, 10);

    EXPECT_EQ(n, 4u);
}

TEST(FakeSerialPortTest, PartialReads)
{
    auto link = makeLink("A", "B");

    FakeSerialPort A(link, FakeSeralHost::Side::A);
    FakeSerialPort B(link, FakeSeralHost::Side::B);

    ASSERT_TRUE(A.openraw("A", 115200, 50));
    ASSERT_TRUE(B.openraw("B", 115200, 50));

    auto data = b({1, 2, 3, 4});

    A.rawwrite(sp(data), data.size());

    uint8_t buf[2]{};

    auto n1 = B.rawread(buf, 2);
    auto n2 = B.rawread(buf, 2);

    EXPECT_EQ(n1, 2u);
    EXPECT_EQ(n2, 2u);
}

TEST(FakeSerialPortTest, Timeout)
{
    auto link = makeLink("A", "B");

    FakeSerialPort A(link, FakeSeralHost::Side::A);

    ASSERT_TRUE(A.openraw("A", 115200, 20));

    uint8_t buf[10]{};

    auto start = std::chrono::steady_clock::now();
    auto n = A.rawread(buf, 10);
    auto end = std::chrono::steady_clock::now();

    EXPECT_EQ(n, 0u);

    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    EXPECT_GE(ms, 20);
}

TEST(FakeSerialPortTest, FlushBus)
{
    auto link = makeLink("A", "B");

    FakeSerialPort A(link, FakeSeralHost::Side::A);
    FakeSerialPort B(link, FakeSeralHost::Side::B);

    ASSERT_TRUE(A.openraw("A", 115200, 50));
    ASSERT_TRUE(B.openraw("B", 115200, 50));

    A.rawwrite(sp(b({1, 2, 3})), 3);

    B.flushBuss();

    uint8_t buf[10]{};

    auto n = B.rawread(buf, 10);

    EXPECT_EQ(n, 0u);
}

TEST(FakeSerialPortTest, Concurrency)
{
    auto link = makeLink("A", "B");

    FakeSerialPort A(link, FakeSeralHost::Side::A);
    FakeSerialPort B(link, FakeSeralHost::Side::B);

    ASSERT_TRUE(A.openraw("A", 115200, 100));
    ASSERT_TRUE(B.openraw("B", 115200, 100));

    std::atomic<size_t> bytesWritten{0};
    std::atomic<size_t> bytesRead{0};

    constexpr size_t targetBytes = 300;

    std::thread writer([&]
    {
        auto data = b({1, 2, 3});

        for (int i = 0; i < 100; ++i)
        {
            A.rawwrite(sp(data), data.size());
            bytesWritten += data.size();
        }
    });

    std::thread reader([&]
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

    EXPECT_EQ(bytesWritten.load(), targetBytes);
    EXPECT_EQ(bytesRead.load(), targetBytes);
}
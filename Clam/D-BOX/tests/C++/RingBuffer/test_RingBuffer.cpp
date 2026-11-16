//
// Created by gabri on 6/21/2026.
//

#include <gtest/gtest.h>

#include <cassert>
#include <array>
#include <iostream>
#include <RingBuffer/RingBuffer.h>
#include <RingBuffer/RingBufferException.hpp>
#include <RingBuffer/RingBuffer.tpp>

TEST(RingBufferTest, Empty)
{
    RingBuffer::RingBuffer<int, 5> rb;

    EXPECT_TRUE(rb.empty());
    EXPECT_EQ(rb.size(), 0);
    EXPECT_FALSE(rb.full());
}

TEST(RingBufferTest, Push)
{
    RingBuffer::RingBuffer<int, 3> rb;

    std::array<int, 3> data{1, 2, 3};

    size_t written = rb.push(data);

    EXPECT_EQ(written, 3);
    EXPECT_TRUE(rb.full());
}

TEST(RingBufferTest, PopFIFO)
{
    RingBuffer::RingBuffer<int, 5> rb;

    rb.push(std::array<int, 3>{10, 20, 30});

    std::array<int, 3> out{};

    size_t read = rb.pop(out, 3);

    EXPECT_EQ(read, 3);
    EXPECT_EQ(out[0], 10);
    EXPECT_EQ(out[1], 20);
    EXPECT_EQ(out[2], 30);
    EXPECT_TRUE(rb.empty());
}

TEST(RingBufferTest, PeekDoesNotModifyBuffer)
{
    RingBuffer::RingBuffer<int, 5> rb;

    rb.push(std::array<int, 3>{1, 2, 3});

    std::array<int, 3> a{};
    std::array<int, 3> b{};

    rb.peek(a, 3);
    rb.peek(b, 3);

    EXPECT_EQ(a[0], b[0]);
    EXPECT_EQ(a[1], b[1]);
    EXPECT_EQ(a[2], b[2]);

    EXPECT_EQ(rb.size(), 3);
}

TEST(RingBufferTest, PopThrowsWhenSpanTooSmall)
{
    RingBuffer::RingBuffer<int, 5> rb;

    rb.push(std::array<int, 3>{1, 2, 3});

    std::array<int, 2> out{};

    EXPECT_THROW(
        rb.pop(out, 3),
        RingBuffer::RingBufferSpanTooSmallException
    );
}

TEST(RingBufferTest, PopThrowsOnUnderflow)
{
    RingBuffer::RingBuffer<int, 5> rb;

    rb.push(std::array<int, 2>{7, 8});

    std::array<int, 10> out{};

    EXPECT_THROW(
        rb.pop(out, 10),
        RingBuffer::RingBufferUnderflowException
    );
}

TEST(RingBufferTest, Wraparound)
{
    RingBuffer::RingBuffer<int, 4> rb;

    rb.push(std::array<int, 3>{1, 2, 3});

    std::array<int, 2> tmp{};
    rb.pop(tmp, 2);

    rb.push(std::array<int, 3>{4, 5, 6});

    std::array<int, 4> out{};

    size_t read = rb.pop(out, 4);

    EXPECT_EQ(read, 4);
    EXPECT_EQ(out[0], 3);
    EXPECT_EQ(out[1], 4);
    EXPECT_EQ(out[2], 5);
    EXPECT_EQ(out[3], 6);
}
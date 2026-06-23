//
// Created by gabri on 6/21/2026.
//

#include "TestRingBuffer.h"

void test_empty()
{
    RingBuffer<int, 5> rb;

    assert(rb.empty());
    assert(rb.size() == 0);
    assert(!rb.full());
}

void test_push()
{
    RingBuffer<int, 3> rb;

    std::array<int, 3> data{1,2,3};

    size_t written = rb.push(data);

    assert(written == 3);
    assert(rb.full());
}

void test_pop_fifo()
{
    RingBuffer<int, 5> rb;

    rb.push(std::array<int,3>{10,20,30});

    std::array<int,3> out{};

    size_t read = rb.pop(out, 3);

    assert(read == 3);
    assert(out[0] == 10);
    assert(out[1] == 20);
    assert(out[2] == 30);
    assert(rb.empty());
}

void test_peek_no_modify()
{
    RingBuffer<int, 5> rb;

    rb.push(std::array<int,3>{1,2,3});

    std::array<int,3> a{};
    std::array<int,3> b{};

    rb.peek(a, 3);
    rb.peek(b, 3);

    assert(a[0] == b[0]);
    assert(a[1] == b[1]);
    assert(a[2] == b[2]);

    assert(rb.size() == 3);
}

void test_span_too_small()
{
    RingBuffer<int, 5> rb;

    rb.push(std::array<int,3>{1,2,3});

    std::array<int,2> out{};

    bool threw = false;

    try
    {
        rb.pop(out, 3);
    }
    catch (const RingBufferSpanTooSmallException&)
    {
        threw = true;
    }

    assert(threw);
}

void test_underflow()
{
    RingBuffer<int, 5> rb;

    rb.push(std::array<int,2>{7,8});

    std::array<int,10> out{};

    bool threw = false;

    try
    {
        rb.pop(out, 10);
    }
    catch (const RingBufferUnderflowException&)
    {
        threw = true;
    }

    assert(threw);
}

void test_wraparound()
{
    RingBuffer<int, 4> rb;

    rb.push(std::array<int,3>{1,2,3});

    std::array<int,2> tmp{};
    rb.pop(tmp, 2);

    rb.push(std::array<int,3>{4,5,6});

    std::array<int,4> out{};

    size_t read = rb.pop(out, 4);

    assert(read == 4);
    assert(out[0] == 3);
    assert(out[1] == 4);
    assert(out[2] == 5);
    assert(out[3] == 6);
}

int TestRingBuffer()
{
    test_empty();
    test_push();
    test_pop_fifo();
    test_peek_no_modify();
    test_span_too_small();
    test_underflow();
    test_wraparound();

    std::cout << "All RingBuffer tests passed!\n";
    return 0;


}
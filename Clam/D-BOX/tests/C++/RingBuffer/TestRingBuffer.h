//
// Created by gabri on 6/21/2026.
//

#ifndef D_BOX_TESTRINGBUFFER_H
#define D_BOX_TESTRINGBUFFER_H
#include <cassert>
#include <array>
#include <iostream>
#include "../../RingBuffer/RingBuffer.h"
#include "../../RingBuffer/RingBufferException.hpp"
#include "../../RingBuffer/RingBuffer.tpp"


void test_empty();
void test_push();
void test_pop_fifo();
void test_peek_no_modify();
void test_span_too_small();
void test_underflow();
void test_wraparound();
int TestRingBuffer();
#endif //D_BOX_TESTRINGBUFFER_H
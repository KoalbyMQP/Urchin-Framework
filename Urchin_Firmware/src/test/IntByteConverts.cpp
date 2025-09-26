//
// Created by gabri on 9/24/25.
//

#include <unity.h>
#include "Global/GLOBAL.h"
struct TestCase {
    uint32_t input;
    unsigned char expected[4];
};

TestCase tests[] = {
    {2325088403, {0x8A, 0x96, 0x08, 0x93}},
    {0x12345678, {0x12, 0x34, 0x56, 0x78}},
        {1700091990, {0x65, 0x55, 0x58, 0x56}},
    {1, {0x00, 0x00, 0x00, 0x01}},
        { 0,{0x00,0x00,0x00,0x00}}
};


void test_unsignedIntToBytes() {


    for (auto &t : tests) {
        unsigned char buf[4] = {0};
        unsignedIntToBytes(t.input, buf);
        TEST_ASSERT_EQUAL_MEMORY(t.expected, buf, 4);
    }


}


void test_BytesToUnsignedInt() {


    for (auto &t : tests) {
        unsigned char buf[4] = {0};
        TEST_ASSERT_EQUAL_UINT32(t.input,BytesToUnsignedInt(t.expected));
    }


}
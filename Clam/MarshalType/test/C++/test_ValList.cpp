//
// Created by gabri on 6/22/2026.
//
#include <gtest/gtest.h>

// Simple sanity test
TEST(SanityCheck, BasicMath) {
    EXPECT_EQ(1 + 1, 2);
}

// Another test to prove multiple tests are detected
TEST(SanityCheck, BooleanLogic) {
    EXPECT_TRUE(true);
    EXPECT_FALSE(false);
}

// Test that your library can be linked (optional placeholder)
TEST(MarshalType, LibraryExists) {
    // If your library header exists, include it like this:
    // #include "ValList.h"

    // Placeholder assertion to confirm test runs
    SUCCEED();
}
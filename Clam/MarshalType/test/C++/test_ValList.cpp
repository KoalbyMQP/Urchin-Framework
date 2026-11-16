//
// Created by gabri on 6/22/2026.
//
#include <gtest/gtest.h>
#include <ValList.h>




TEST(TestValVariant, IntEquality) {
    MarshalType::ValVariant a(-1);
    MarshalType::ValVariant b(-1);
    MarshalType::ValVariant c(0);

    EXPECT_EQ(a, b);
    EXPECT_FALSE(a == c);
}

TEST(TestValVariant, ZeroEquality) {
    MarshalType::ValVariant a(0);
    MarshalType::ValVariant b(0);

    EXPECT_EQ(a, b);
}

TEST(TestValVariant, PositiveEquality) {
    MarshalType::ValVariant a(123);
    MarshalType::ValVariant b(123);

    EXPECT_EQ(a, b);
}

TEST(TestValVariant, NegativeValues) {
    std::vector<int> values = {-1, -2, -100, -(2LL << 30)};

    for (auto v : values) {
        MarshalType::ValVariant a(v);
        MarshalType::ValVariant b(v);

        EXPECT_EQ(a, b);
    }
}


TEST(TestValVariant, UnsignedAndSignedInEquality) {
    int valA = 30;
    unsigned valB = 30;

    MarshalType::ValVariant a(valA);
    MarshalType::ValVariant b(valB);

    EXPECT_NE(a, b);
}

TEST(TestValVariant, Int32Boundary) {
     MarshalType::ValVariant a((1LL << 31) - 1);
     MarshalType::ValVariant b((1LL << 31) - 1);

    EXPECT_EQ(a, b);
}



TEST(TestValVariant, SignedUnsignedMismatchProbe) {
    MarshalType::ValVariant a(-1);
    MarshalType::ValVariant b(4294967295u);

    std::cout << "\nDEBUG a=" << a
              << " b=" << b << std::endl;

    SUCCEED(); // diagnostic-only test
}

TEST(TestValVariant, EqOperatorWorks) {
    MarshalType::ValVariant a(-1);
    MarshalType::ValVariant b(-1);

    EXPECT_TRUE(a == b);
}

TEST(TestValVariant, NeOperator) {
    MarshalType::ValVariant a(-1);
    MarshalType::ValVariant b(1);

    EXPECT_NE(a, b);
}


TEST(TestValVariant, GetConsistency) {
    MarshalType::ValVariant a(10);
    MarshalType::ValVariant b(10);

    EXPECT_EQ(a.get(), b.get());
}

// -------------------------
// FLOAT TESTS
// -------------------------

TEST(TestValVariant, FloatEquality) {
    MarshalType::ValVariant a(1.0f);
    MarshalType::ValVariant b(1.0f);
    MarshalType::ValVariant c(2.0f);

    EXPECT_EQ(a, b);
    EXPECT_FALSE(a == c);
}

TEST(TestValVariant, FloatEdgeValues) {
    std::vector<float> values = {-1.0f, 0.0f, 3.14f, -2.5f, 1e10f};

    for (float v : values) {
        MarshalType::ValVariant a(v);
        MarshalType::ValVariant b(v);

        EXPECT_EQ(a, b);
    }
}


// -------------------------
// BOOL TESTS
// -------------------------

TEST(TestValVariant, BoolEquality) {
    MarshalType::ValVariant a(true);
    MarshalType::ValVariant b(true);
    MarshalType::ValVariant c(false);

    EXPECT_EQ(a, b);
    EXPECT_FALSE(a == c);
}

TEST(TestValVariant, BoolIntInteraction) {
    MarshalType::ValVariant a(true);
    MarshalType::ValVariant b(false);
    MarshalType::ValVariant c(1);
    MarshalType::ValVariant d(0);

    MarshalType::ValVariant e(1118872535);
    MarshalType::ValVariant f(88.32f);

    std::cout << a << " should be bool\n";
    EXPECT_TRUE(a);

    std::cout << c << " should be int\n";
    EXPECT_EQ(c, 1);

    std::cout << b << " should be bool\n";
    EXPECT_FALSE(b);

    std::cout << d << " should be int\n";
    EXPECT_EQ(d, 0);

    EXPECT_NE(a, b);
    EXPECT_NE(a, c);
    EXPECT_NE(b, d);
    EXPECT_NE(c, d);
    EXPECT_NE(e, f);
}


TEST(ValList, NotEquals_int_float) {
    int i = 1065353216; // bytes: 0x3F800000
    float f = std::bit_cast<float>(i);
    MarshalType::ValVariant val1(i);

    MarshalType::ValVariant val2(f);

    EXPECT_FALSE(val1 == val2);

    MarshalType::ValVariant val3(10);

    MarshalType::ValVariant val4(10.0f);

    EXPECT_FALSE(val1 == val2);


}


TEST(ValList, MarshalAndDemarshal_Bool) {
    std::vector<uint8_t> buffer;
    MarshalType::ValVariant val_pre(true);
    MarshalType::ValVariant val_post;
    val_pre.Marshal(buffer);
    val_post.DeMarshal(buffer);
    EXPECT_TRUE(val_pre == val_post);
}


TEST(ValList, MarshalAndDemarshal_Int) {
    std::vector<uint8_t> buffer;
    int val = 40;
    MarshalType::ValVariant val_pre(val);
    MarshalType::ValVariant val_post;
    val_pre.Marshal(buffer);
    val_post.DeMarshal(buffer);
    EXPECT_TRUE(val_pre == val_post);
}

TEST(ValList, MarshalAndDemarshal_Uint) {
    std::vector<uint8_t> buffer;
    unsigned val = 40;
    MarshalType::ValVariant val_pre(val);
    MarshalType::ValVariant val_post;
    val_pre.Marshal(buffer);
    val_post.DeMarshal(buffer);
    EXPECT_TRUE(val_pre == val_post);
}
TEST(ValList, MarshalAndDemarshal_Float) {
    std::vector<uint8_t> buffer;
    float val = 3.14f;
    MarshalType::ValVariant val_pre(val);
    MarshalType::ValVariant val_post;
    val_pre.Marshal(buffer);
    val_post.DeMarshal(buffer);
    EXPECT_TRUE(val_pre == val_post);
}
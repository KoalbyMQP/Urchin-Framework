//
// Created by gabri on 6/22/2026.
//
#include <gtest/gtest.h>
#include <Item.h>
#include <ValList.h>

TEST(TestItem, Equality) {

    MarshalType::Item a("Arm","moveone",0.45f,30,4);
    MarshalType::Item b("Arm","moveone",0.45f,30,4);

    EXPECT_TRUE(a==b);
}

TEST(TestItem, NonEquality) {

    MarshalType::Item a("Arm","moveone",0.45f,30,true);
    MarshalType::Item b("Arm","moveone",0.45f,30,1);

    EXPECT_FALSE(a==b);

    MarshalType::Item c("Arm","moveone",0.45f,30,false);
    MarshalType::Item d("Arm","moveone",0.45f,30,0);

    EXPECT_FALSE(c==d);
}

TEST(TestItem, MAX_JOINT_LEN) {
    MarshalType::Item a("Uwa1miez1chanee9lishiwahWighie4eivo3Aixai5saif7ung","moveone",0.45f,30,true);

    EXPECT_THROW(
        MarshalType::Item b("Uwa1miez1chanee9lishiwahWighie4eivo3Aixai5saif7unga","moveone",0.45f,30,true),
        std::runtime_error);

}

TEST(TestItem, MAX_VALUES_LEN) {
    std::vector<MarshalType::ValVariant> v1;

    for (int i = 0; i < 255; ++i) {
        v1.push_back(MarshalType::ValVariant(67));
    }

    MarshalType::Item a("Arm","moveone",v1);


    std::vector<MarshalType::ValVariant> v2;

    for (int i = 0; i < 256; ++i) {
        v2.push_back(MarshalType::ValVariant(67));
    }


    EXPECT_THROW(
         MarshalType::Item b("Arm","moveone",v2),
        std::runtime_error);


}

TEST(ValList, MarshalAndDemarshal) {


    std::vector<uint8_t> buffer;
    MarshalType::Item Item_pre("Arm","moveone",0.45f,30,true);
    MarshalType::Item Item_post;
    Item_pre.Marshal(buffer);
    Item_post.DeMarshal(buffer);
    EXPECT_TRUE(Item_pre == Item_post);
}



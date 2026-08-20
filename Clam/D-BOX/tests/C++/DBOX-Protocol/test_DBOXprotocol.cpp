//
// Created by gabri on 7/19/26.
//


#include <gtest/gtest.h>
#include <DBOXprotocol.h>
#include <Transport-Interface/UART/Fake/FakeSeralPort.h>
#include <Transport-Interface/UART/Fake/FakeSerialLink.h>
#include <Transport-Interface/UART/Fake/FakeSeralHost.h>
#include <Discovery/UartDiscovery.h>
#include <Discovery/Discovery.h>
#include <Dconfig/DconfigUart.h>

#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <memory>
#include <span>
#include <string>
#include <vector>
#include <RingBuffer/RingBufferException.hpp>
using namespace RingBuffer;
using namespace DBOXProtocol;

static std::shared_ptr<FakeSerialLink>
makeLink(const std::string& aName,
         const std::string& bName)
{
    return std::make_shared<FakeSerialLink>(aName, bName);
}

static void assertVectorEq(const std::vector<uint8_t>& actual,
                           std::initializer_list<uint8_t> expected)
{
    EXPECT_TRUE(actual.size() == expected.size());

    std::size_t i = 0;
    for (uint8_t v : expected)
    {
        EXPECT_TRUE(actual[i] == v);
        ++i;
    }
}

TEST(DBOX_Protocal, BasicCommunicationAandB) {
    auto link = makeLink("SideA", "SideB");
    FakeSerialPort portA(link, FakeSeralHost::Side::A);
    FakeSerialPort portB(link, FakeSeralHost::Side::B);

    EXPECT_TRUE(portA.openraw("SideA", 115200, 10));
    EXPECT_TRUE(portB.openraw("SideB", 115200, 10));

    //const std::array<uint8_t, 4> data{11, 12, 13, 14};



    DBOXprotocol Primary = DBOXprotocol(&portA,"dumpA");
    DBOXprotocol Secondary = DBOXprotocol(&portB,"dumpB");


    DBOX packet_out_Primary(0, 'R', "Hello world!");
    EXPECT_TRUE(Primary.send(&packet_out_Primary));

    DBOX packet_in_Secondary;
    Secondary.receive(&packet_in_Secondary);

    EXPECT_TRUE(packet_out_Primary == packet_in_Secondary);




    DBOX packet_out_Secondary(0, 'R', "Hello world!");
    EXPECT_TRUE(Primary.send(&packet_out_Secondary));

    DBOX packet_in_Primary;
    Secondary.receive(&packet_in_Primary);

    EXPECT_TRUE(packet_out_Secondary == packet_in_Primary);

}


TEST(DBOX_Protocal, Discovery_Start) {
    auto link1 = makeLink("LinuxCom1", "UART1");
    auto link2 = makeLink("LinuxCom2", "Mouse");

    FakeSeralHost linux = FakeSeralHost();
    linux.AddPlug(link1,FakeSeralHost::Side::A);
    linux.AddPlug(link2,FakeSeralHost::Side::A);

    FakeSeralHost ESP32 = FakeSeralHost();
    ESP32.AddPlug(link1,FakeSeralHost::Side::B);

    DconfigUart configA = DconfigUart("fake","SideA", 115200, 10);
    DconfigUart configB = DconfigUart("fake","SideB", 115200, 10);

    UartDiscovery DiscoA = UartDiscovery(true,&configA);
    UartDiscovery DiscoB = UartDiscovery(false,&configB);

    FakeSerialPort portA(&DiscoA, &linux);
    FakeSerialPort portB(&DiscoB, &ESP32);

    DiscoA.BindPort(&portA);
    DiscoB.BindPort(&portB);

    DBOXprotocol protoA = DBOXprotocol(&portA,"dumpA");
    DBOXprotocol protoB = DBOXprotocol(&portB,"dumpB");
    //protoB.start();
    //protoA.start();


}







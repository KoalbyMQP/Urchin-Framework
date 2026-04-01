//
// Created by gabri on 9/25/25.
//
#include <unity.h>
#include "Ticketing/TicketNum.h"
#include "ESP_PI_Communication/MSGQueue.h"
#include "ESP_PI_Communication/Coms.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

constexpr int MaxTicket=StripSize*sizeof(uint64_t);
typedef struct{
    uint8_t VPID;
    char data[1024];
}MSG;



void Test_FIND() {
    RollINIT(&TicketTape);



    for (int i = 0; i < MaxTicket; ++i) {

    TEST_ASSERT_EQUAL_UINT32(i,FindFree(&TicketTape));
        checkOut(&TicketTape,i);
    }
}

void test_checkOut_strip(void){

    TapeRoll t_Roll;



    RollINIT(&t_Roll);
    RollINIT(&TicketTape);



    unsigned int t_Ticket = 65;
    unsigned int t_Mod = t_Ticket%Width;
    unsigned int t_Chunk = (t_Ticket-t_Mod)/(Width);

    (TicketTape.strip[t_Chunk]) &= ~(1U<<t_Mod);
    checkOut(&t_Roll, t_Ticket);

    for(int i = 0; i < StripSize; i++){//Makes sure whole thing is equal not just single index
        TEST_ASSERT_EQUAL(t_Roll.strip[i],TicketTape.strip[i]);
    }

}


void test_ReturnTicket(void){
    TapeRoll t_Roll;
    RollINIT(&t_Roll);
    RollINIT(&TicketTape);


    unsigned int t_Ticket = 65;
    unsigned int t_Mod = t_Ticket%Width;
    unsigned int t_Chunk = (t_Ticket-t_Mod)/(Width);
    //Width = 64

    ReturnTicket(&t_Roll, t_Ticket);

    (TicketTape.strip[t_Chunk]) &= ~(1U<<t_Mod);

    for(int i = 0; i < StripSize; i++){
        TEST_ASSERT_EQUAL(t_Roll.strip[i],TicketTape.strip[i]);
    }




}

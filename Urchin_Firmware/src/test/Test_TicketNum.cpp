//
// Created by gabri on 9/25/25.
//
#include <unity.h>
#include "Ticketing/TicketNum.h"
constexpr int MaxTicket=StripSize*sizeof(uint64_t);




void Test_FIND() {
    RollINIT(&TicketTape);



    for (int i = 0; i < MaxTicket; ++i) {

    TEST_ASSERT_EQUAL_UINT32(i,FindFree(&TicketTape));
        checkOut(&TicketTape,i);
    }
}





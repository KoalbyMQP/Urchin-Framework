//
// Created by gabri on 9/24/25.
//

#include <unity.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "esp_log.h"
#include <stdint.h>
#include <cstring>

#include "IntByteConverts.h"
#include "Test_Coms.h"
#include "Test_TicketNum.h"




extern "C" void UnitTests() {
    //rolint: ignore
    printf("App main started\n");
    fflush(stdout);
    UNITY_BEGIN();

    RUN_TEST(test_unsignedIntToBytes);

    RUN_TEST(test_BytesToUnsignedInt);

    RUN_TEST(test_PackfToPI_oversize);

   //RUN_TEST(test_PackfToPI_full);
   RUN_TEST(test_PackfToPI_Success);

   RUN_TEST(test_PrintfToPI_BadArgsQueue);

   RUN_TEST(test_PrintfToPI_BadArgsFormat);

   RUN_TEST(test_PrintfToPI_Malicious);

   RUN_TEST(test_ReturnTicket);

   RUN_TEST(test_checkOut_strip);

   RUN_TEST(Test_FIND);

   UNITY_END();
}

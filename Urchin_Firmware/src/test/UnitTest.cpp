
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
#include "Test_TicketNum.h"




extern "C" void UnitTests() {
    printf("App main started\n");
    fflush(stdout);
    UNITY_BEGIN();

    RUN_TEST(test_unsignedIntToBytes);

    RUN_TEST(test_BytesToUnsignedInt);

    RUN_TEST(Test_FIND);
    UNITY_END();
}


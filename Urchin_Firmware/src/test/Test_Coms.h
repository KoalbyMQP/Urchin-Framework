//
// Created by Sam Kim on 9/27/25.
//

#ifndef URCHIN_FIRMWARE_TESTCOMS_H
#define URCHIN_FIRMWARE_TESTCOMS_H

extern "C" {
  #include <unity.h>
  #include "freertos/FreeRTOS.h"
  #include "freertos/queue.h"
}

#include "Global/Errors.h"
#include "Global/GLOBAL.h"
#include <cstdint>
#include <cstring>
#include "ESP_PI_Communication/Coms.h"
#include "ESP_PI_Communication/Shipping.h"

#include <stdio.h>
#include <stdarg.h>

void test_PackfToPI_oversize();

//void test_PackfToPI_full();

void test_PackfToPI_Success();

void test_PrintfToPI_BadArgsQueue();

void test_PrintfToPI_BadArgsFormat();

void test_PrintfToPI_Malicious();
#endif //URCHIN_FIRMWARE_TESTCOMS_H
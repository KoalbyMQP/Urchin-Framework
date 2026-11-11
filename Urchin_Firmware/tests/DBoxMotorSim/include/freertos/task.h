#pragma once
#include "FreeRTOS.h"
void vTaskDelay(TickType_t t);
TickType_t xTaskGetTickCount();
BaseType_t xTaskCreate(TaskFunction_t f, const char*, uint32_t, void* arg, UBaseType_t, TaskHandle_t*);

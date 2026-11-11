// Host stand-in for FreeRTOS: just enough for the DBoxMotor bench build.
#pragma once
#include <cstdint>
#include <cstddef>
typedef uint32_t TickType_t;
typedef int BaseType_t;
typedef unsigned int UBaseType_t;
#define portTICK_PERIOD_MS 1
#define pdMS_TO_TICKS(x) ((TickType_t)(x))
#define portMAX_DELAY 0xFFFFFFFFu
#define pdTRUE 1
#define pdFALSE 0
#define pdPASS 1
struct SimQueue;
typedef SimQueue* QueueHandle_t;
typedef void (*TaskFunction_t)(void*);
typedef void* TaskHandle_t;

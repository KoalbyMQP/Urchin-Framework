//
// Created by gabri on 3/23/2025.
//

#ifndef SETUP_H
#define SETUP_H

#include "driver/gpio.h"
#include "GLOBAL.h"
#include "driver/uart.h"
#include "ESP_PI_Communication/Coms.h"
#include "GLOBAL.h"
#include "ESP_PI_Communication/Shipping.h"


#define AsyncSize 64
#define SequentialSize 64

#ifdef __cplusplus
extern "C" {
#endif


    void SetUpUart();

    void  SetUpPins();

    void esp_core_dump_to_custom(const void* core_dump_data, size_t data_len);

#ifdef __cplusplus
}
#endif

extern uart_config_t uart_config;

#endif //SETUP_H

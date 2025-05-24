//
// Created by gabri on 3/23/2025.
//

#ifndef SETUP_H
#define SETUP_H

#include "driver/gpio.h"
#include "GLOBAL.h"
#include "driver/uart.h"
#include "coms.h"
#include "GLOBAL.h"

#ifdef __cplusplus
extern "C" {
#endif


    void SetUpPins();


#ifdef __cplusplus
}
#endif

extern uart_config_t uart_config;

#endif //SETUP_H

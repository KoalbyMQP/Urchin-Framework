//
// Created by gabri on 12/8/25.
//

#ifndef URCHIN_FIRMWARE_INJECTOR_H
#define URCHIN_FIRMWARE_INJECTOR_H




#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "Coms.h"
#include "Ordering/Ordering.h"
#include "Herkulex/Herkulex.h"
#include "Ticketing/Ticket.h"

#include "Ticketing/Packet.h"
extern SemaphoreHandle_t UART1Mutex;
[[noreturn]] void Spray(void *pvParameters);
#endif //URCHIN_FIRMWARE_INJECTOR_H

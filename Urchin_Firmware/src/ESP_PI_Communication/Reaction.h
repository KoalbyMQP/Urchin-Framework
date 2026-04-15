//
// Created by gabri on 4/13/2026.
//

#ifndef URCHIN_FIRMWARE_REACTION_H
#define URCHIN_FIRMWARE_REACTION_H
#include "Global/Bridge.h"
#include "ESP_PI_Communication/MSGQueue.h"
#include "ESP_PI_Communication/Coms.h"
#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
int SendReaction(const uint8_t VPID, unsigned int Ticket, const char* Joint, const char* CommandName, unsigned int CommandNameSize, unsigned int Code, const char *format, ...);
#endif //URCHIN_FIRMWARE_REACTION_H
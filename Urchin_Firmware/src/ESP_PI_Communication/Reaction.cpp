//
// Created by gabri on 4/13/2026.
//


#include "Reaction.h"
#include "Global//Bridge.h"
#include "ESP_PI_Communication/MSGQueue.h"
#include "ESP_PI_Communication/Coms.h"
#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>
#include "freertos/FreeRTOS.h"

int SendReaction(const uint8_t VPID, unsigned int Ticket, const char* Joint, const char* CommandName, unsigned int CommandNameSize, unsigned int Code, const char *format, ...) {

#pragma pack(push, 1)
    typedef struct {
        unsigned int ticket;
        unsigned int Code;
        char joint[BRIDGEMaxName];
        unsigned char command_len;
        unsigned char values_len;
    } PacketHeader;

    typedef union {
        int Int;
        float Float;
        char Byte;
    } DataValue;

    typedef struct {
        char Type;
        DataValue Data;
    } Data;
#pragma pack(pop)


    int count = 0;

    for (int i = 0; format[i] != '\0'; ) {

        if (format[i] != '%') {
            PrintfToPI(DebugQueue, 0,
                "Error: expected '%%' at position %d\n", i);
            return -1;
        }

        i++; // move to specifier

        if (format[i] == '\0') {
            PrintfToPI(DebugQueue, 0,
                "Error: incomplete format specifier\n");
            return -2;
        }

        switch (format[i]) {
            case 'i':
            case 'f':
            case 'b':
                break;

            default:
                PrintfToPI(DebugQueue, 0,
                    "Error: invalid format specifier '%%%c'\n",
                    format[i]);
                return -3;
        }

        count++;
        i++; // advance past specifier
    }

    // allocate value array
    Data *values = (Data *)pvPortMalloc(sizeof(Data) * count);
    if (!values) {
        PrintfToPI(DebugQueue, 0, "Error: malloc failed for values\n");
        return -99;
    }


    va_list args;
    va_start(args, format);

    int idx = 0;

    for (int i = 0; format[i] != '\0'; ) {

        if (format[i] != '%') {
            PrintfToPI(DebugQueue, 0,
                "Error: expected '%%' at position %d\n", i);
            va_end(args);
            pvPortFree(values);
            return -1;
        }

        i++; // move to specifier

        if (format[i] == '\0') {
            va_end(args);
            pvPortFree(values);
            return -2;
        }

        char spec = format[i];
        i++; // move past specifier

        switch (spec) {

            case 'i': {
                int value = va_arg(args, int);
                values[idx].Type = 'i';
                values[idx].Data.Int = value;
                idx++;
                break;
            }

            case 'f': {
                double value = va_arg(args, double);
                values[idx].Type = 'f';
                values[idx].Data.Float = (float)value;
                idx++;
                break;
            }

            case 'b': {
                int value = va_arg(args, int);
                values[idx].Type = 'b';
                values[idx].Data.Byte = (char)value;
                idx++;
                break;
            }

            default:
                va_end(args);
                pvPortFree(values);
                return -3;
        }
    }

    va_end(args);



    size_t size =
        sizeof(PacketHeader) +
        CommandNameSize +
        (count * sizeof(Data));

    char *buffer = (char *)pvPortMalloc(size);
    if (!buffer) {
        pvPortFree(values);
        return -99;
    }

    char *ptr = buffer;

    // Packet header
    PacketHeader Header = {0};
    Header.ticket = Ticket;
    Header.Code = Code;

    memset(Header.joint, 0, BRIDGEMaxName);
    strncpy(Header.joint, Joint, BRIDGEMaxName - 1);

    Header.command_len = CommandNameSize;
    Header.values_len = count;

    memcpy(ptr, &Header, sizeof(PacketHeader));
    ptr += sizeof(PacketHeader);

    // command name
    memcpy(ptr, CommandName, CommandNameSize);
    ptr += CommandNameSize;

    // values
    memcpy(ptr, values, sizeof(Data) * count);
    ptr += sizeof(Data) * count;

    int error = PackfToPI(RecationQueue, VPID, buffer, size);

    pvPortFree(buffer);
    pvPortFree(values);

    return error;
}
//
// Created by gabri on 4/13/2026.
//


#include "Reaction.h"

//int SendReaction(const uint8_t VPID, unsigned int Ticket, const char* Joint, const char* CommandName, unsigned int CommandNameSize, unsigned int Code, const char *format, ...) {return 0;}



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
            vPortFree(values);
            return -1;
        }

        i++; // move to specifier

        if (format[i] == '\0') {
            va_end(args);
            vPortFree(values);
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
                vPortFree(values);
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
        vPortFree(values);
        return -99;
    }

    char *ptr = buffer;

    // Packet header
    PacketHeader Header;
    memset(&Header, 0, sizeof(Header));
    Header.ticket = Ticket;
    PrintfToPI(DebugQueue, VPID, "Reaction: Setting Ticket to:%d",Ticket);
    Header.Code = Code;
    PrintfToPI(DebugQueue, VPID, "Reaction: Setting code to:%d",Code);

    memset(Header.joint, 0, BRIDGEMaxName);
    strncpy(Header.joint, Joint, BRIDGEMaxName - 1);
    PrintfToPI(DebugQueue, VPID, "Reaction: Setting Joint to:%s",Header.joint);

    Header.command_len = CommandNameSize;
    PrintfToPI(DebugQueue, VPID, "Reaction: Setting Command Len to:%d",CommandNameSize);

    Header.values_len = count;
    PrintfToPI(DebugQueue, VPID, "Reaction: Setting values_len  to:%d",count);

    memcpy(ptr, &Header, sizeof(PacketHeader));
    ptr += sizeof(PacketHeader);

    // command name
    memcpy(ptr, CommandName, CommandNameSize);
    ptr += CommandNameSize;
    PrintfToPI(DebugQueue, VPID, "Reaction: Setting command name to:%s",CommandName);

    // values
    memcpy(ptr, values, sizeof(Data) * count);
    ptr += sizeof(Data) * count;

    int error = PackfToPI(RecationQueue, VPID, buffer, size);

    vPortFree(buffer);
    vPortFree(values);

    return error;
}

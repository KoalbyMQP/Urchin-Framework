#include <sys/cdefs.h>
#include "esp_task_wdt.h"
#include "Ticketing/Ticket.h"
#include "Global/GLOBAL.h"
//
// Created by gabri on 3/24/2025.
// !!! do you best you use only C in this file
//





#include "UnPacker.h"

#include <ESP_PI_Communication/Coms.h>
#include "ESP_PI_Communication/Shipping.h"
#include "Global/Errors.h"
#include "Ticketing/TicketNum.h"
#include "ESP_PI_Communication/MSGQueue.h"



#include "Herkulex/Herkulex.h"

Ticket* Tickets[MaxTickets];




    HerkulexClass Herkulex;


int UnpackerInit() {
    Herkulex.begin(UART_NUM_1,115200,16,17); //115200 is default for DRS-0601

    Herkulex.initialize();
    Herkulex.reboot(7);
    Herkulex.torqueON(7);

    Herkulex.SetIndirect(false);


    return 0;
}


//Basic statements
//-----------------------------------------------------------------


int ReqTicket(const char* buffer){
    unsigned char Platter[4]={0};

    (void)PrintfToPI(DebugQueue,0,"ReqTicket called");

    //Get Ticket
    unsigned int Ticket = FindFree(&TicketTape);

    //send NoFreeTicket if non are found
    if (Ticket == -1) {
        memset(Platter,0,sizeof(int));
        unsignedIntToBytes(URCHIN_ERROR_NoFreeTicket,Platter);
        (void) PackfToPI(ExchangeQueue,0,reinterpret_cast<const char *>(Platter),sizeof(int));

        //Debug Prints
        (void) PrintfToPI(DebugQueue,0,"URCHIN_ERROR_NoFreeTicket :Bytes: %02X %02X %02X %02X", Platter[0], Platter[1], Platter[2], Platter[3]);
        //(void) PrintfToPI(DebugQueue,0,"URCHIN_ERROR_NoFreeTicket:%d",sizeof(int));
        return -1;
    }


    //Check out ticket
    checkOut(&TicketTape,Ticket);


    //send OK
    memset(Platter,0,sizeof(int));
    unsignedIntToBytes(URCHIN_OK,Platter);
    (void) PackfToPI(ExchangeQueue,0,reinterpret_cast<const char *>(Platter),sizeof(int));

    //Debug Prints
        //(void) PrintfToPI(DebugQueue,0,"URCHIN_OK:Bytes: %02X %02X %02X %02X", Platter[0], Platter[1], Platter[2], Platter[3]);
        //(void) PrintfToPI(DebugQueue,0,"URCHIN_OK:%d",sizeof(int));

    //Sent Ticket
    memset(Platter,0,sizeof(unsigned int));
    (void) PrintfToPI(DebugQueue,0,"ReqTicket:Ticket:%d",Ticket);
    unsignedIntToBytes(Ticket,Platter);
    (void) PackfToPI(ExchangeQueue,0,reinterpret_cast<const char *>(Platter),sizeof(unsigned int));

    //Debug Prints
        //(void) PrintfToPI(DebugQueue,0,"Ticket:Bytes: %02X %02X %02X %02X", Platter[0], Platter[1], Platter[2], Platter[3]);
        //(void) PrintfToPI(DebugQueue,0,"Ticket:%d:%d",Ticket,sizeof(int));

    //PrintfToPI(DebugQueue,0,"Ticket:%d\n",Ticket);



    //(void) PrintfToPI(ExchangeQueue,0,"%d",Ticket);
    //(void) PrintfToPI(ExchangeQueue,0,"Tape%u",TicketTape);

  return 0;
}

int FormatTicket(const char* buffer) {
    const char *TicketNumPoint = buffer;
    const char TicketType = *(buffer+sizeof(unsigned int));
    unsigned int TicketNum = BytesToUnsignedInt(reinterpret_cast<const unsigned char *>(TicketNumPoint));


    PrintfToPI(DebugQueue,0,"FormatTicket:%s",buffer);

    PrintfToPI(DebugQueue,0,"FormatTicket:Ticket:%d",TicketNum);
    PrintfToPI(DebugQueue,0,"FormatTicket:TicketType:%c",TicketNumPoint);


    void *alloc = pvPortMalloc(sizeof(Ticket));
    memset(alloc,0,sizeof(Ticket));
    if (alloc == nullptr) {
        //todo throw error
        return -1;
    }else {
        Ticket* TicketPoint= static_cast<Ticket *>(alloc);
        TicketPoint->TicketNum = TicketNum;
        TicketPoint->type = TicketType;
        //Tickets[TicketNum] = TicketPoint;
    }
    return 0;
}



int LoadTicket(const char* buffer) {
    (void) PrintfToPI(ExchangeQueue,0,"LoadTicket called");

    char command[255]={0};

    #pragma pack(push, 1)  // No padding between fields
        typedef struct{
            unsigned int ticket;
            unsigned int motor;
            unsigned int model;
            unsigned char command_len;
            unsigned char values_len;
        }PacketHeader;


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




     PacketHeader *Header = (PacketHeader*) buffer;

    (void)(PrintfToPI)(DebugQueue,0,"LoadTicket:ticket:%d",Header->ticket);
    (void)(PrintfToPI)(DebugQueue,0,"LoadTicket:motor:%d",Header->motor);
    (void)(PrintfToPI)(DebugQueue,0,"LoadTicket:model:%d",Header->model);
    (void)(PrintfToPI)(DebugQueue,0,"LoadTicket:command_len:%d",Header->command_len);
    (void)(PrintfToPI)(DebugQueue,0,"LoadTicket:values_len:%d",Header->values_len);



    strncpy(command,buffer+sizeof(PacketHeader),Header->command_len);


    (void)(PrintfToPI)(DebugQueue,0,"LoadTicket:command:%s",command);

    const char *PostHead = buffer + sizeof(PacketHeader) + Header->command_len;


    Data Variables[Header->values_len];

    for (int i = 0; i < Header->values_len; ++i) {
        //(void)(PrintfToPI)(DebugQueue,0,"LoadTicket:data Run:%d",i);
        Data *CurrentData = (Data*)(PostHead+(sizeof(Data)*i));
        memcpy(&Variables[i],CurrentData,sizeof(Data));

        // (void)(PrintfToPI)(DebugQueue,0,"LoadTicket:data type:%c",CurrentData->Type);
        //
        // if (CurrentData->Type=='I') {
        //     (void)(PrintfToPI)(DebugQueue,0,"LoadTicket:data:%d",CurrentData->Data.Int);
        // }
        //
        // if (CurrentData->Type=='F') {
        //     (void)(PrintfToPI)(DebugQueue,0,"LoadTicket:data:%F",CurrentData->Data.Float);
        // }
        //
        // if (CurrentData->Type=='B') {
        //     (void)(PrintfToPI)(DebugQueue,0,"LoadTicket:data:%u",CurrentData->Data.Byte);
        // }
    }
    Herkulex.moveOne(Header->motor,Variables[0].Data.Int, Variables[1].Data.Int, static_cast<JogLedColor>(Variables[2].Data.Int), static_cast<HerkulexModel>(Header->model));


    Herkulex.moveOne(7,6, 3, LED_RED, MODEL_0601);
    return 0;
}




int PunchTicket(const char* buffer) {
    (void) PrintfToPI(ExchangeQueue,0,"PunchTicket called");







    return 0;
}


int CloseTicket(const char* buffer) {

    (void) PrintfToPI(ExchangeQueue,0,"CloseTicket not added");
return 0;
}



int TicketInfo(const char* buffer) {

    (void) PrintfToPI(ExchangeQueue,0,"TicketInfo not added");
return 0;
}



int GetHealth(const char* buffer) {
    Herkulex.stat(7);
    if (0== strcmp("FreeRam",buffer)) {
        (void) PrintfToPI(ExchangeQueue,0,"%d",xPortGetFreeHeapSize());

    }
    if (0== strcmp("TotalRam",buffer)) {
        (void) PrintfToPI(ExchangeQueue,0,"%d",xPortGetMinimumEverFreeHeapSize());
    }

    if (0== strcmp("CPU",buffer)) {

    }
    if (0== strcmp("AckCheck",buffer)) {

    }


    //(void) PrintfToPI(ExchangeQueue,"GetHealth is being added");
    //DoSomething();
return 0;
}





//-----------------------------------------------------------------------



//Helppers
//------------------------------------------------------------------------



int ProcessRequest(Context Commands[],const uint8_t buffer[]) {
        int i=0;
        int found=0;
        int error=0;

    //(void) PrintfToPI(DebugQueue,0,"ProcessRequest:%s",buffer);
        while (i < NumOfActions && !found){
            if (0==strncmp((char*)buffer,Commands[i].Name,Commands[i].depth)) {
                error=Commands[i].function(SkipFoward((char*)buffer,Commands[i].depth));
                found=1;
            }
            i++;
            }
    if (!found) {return URCHIN_ERROR_CommandNotFound;}
    return error;

}


const char* SkipFoward(const char buffer[],unsigned int distance) {
    return &buffer[distance];
}

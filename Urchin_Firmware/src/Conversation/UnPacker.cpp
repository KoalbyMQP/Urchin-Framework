#include <sys/cdefs.h>
#include "esp_task_wdt.h"
#include "Ticketing/Ticket.h"
#include "Global/GLOBAL.h"
//
// Created by gabri on 3/24/2025.
// !!! do you best you use only C in this file
//




//Local
#include "UnPacker.h"
#include "Global/Bridge.h"
#include "ESP_PI_Communication/Shipping.h"
#include "Global/Errors.h"
#include "Ticketing/TicketNum.h"
#include "ESP_PI_Communication/MSGQueue.h"
#include "Herkulex/Herkulex.h"
#include "Motor/LimitChecks.h"
#include "Ordering/Ordering.h"
#include "Ticketing/Packet.h"
#include "Ticketing/Ticket.h"

Ticket* Tickets[MaxTickets] = {NULL};




HerkulexClass Herkulex;

static AngleSet HerkuleXMotorBounds[]={//has max size see Receving.h
    {0,1023,-166.7f,166.7f}, //MODEL_0101
    {0,1023,-166.7f,166.7f}, //MODEL_0201
    {0,2047,-166.7f,166.6f}, //MODEL_0601
    {0,2047,-166.7f,166.6f} //MODEL_0602
};


int UnpackerInit() {
    Herkulex.begin(UART_NUM_1,115200,16,17); //115200 is default for DRS-0601
    Herkulex.SetIndirect(false);
    Herkulex.initialize();






    return 0;
}


//Basic statements
//-----------------------------------------------------------------

/*
int ReqTicket(unsigned char VPID, const char* buffer){
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
    (void) PrintfToPI(DebugQueue,0,"ReqTicket:TicketS:%.*s", 4, Platter);
    (void) PackfToPI(ExchangeQueue,0,reinterpret_cast<const char *>(Platter),sizeof(unsigned int));

    //Debug Prints
        //(void) PrintfToPI(DebugQueue,0,"Ticket:Bytes: %02X %02X %02X %02X", Platter[0], Platter[1], Platter[2], Platter[3]);
        //(void) PrintfToPI(DebugQueue,0,"Ticket:%d:%d",Ticket,sizeof(int));

    //PrintfToPI(DebugQueue,0,"Ticket:%d\n",Ticket);



    //(void) PrintfToPI(ExchangeQueue,0,"%d",Ticket);
    //(void) PrintfToPI(ExchangeQueue,0,"Tape%u",TicketTape);

  return 0;
}

*/


int FormatTicket(unsigned char VPID, const char* buffer) {
    LED();
    WebConversion Platter;

    PrintfToPI(DebugQueue,VPID,"FormatTicket:%s",buffer);


    const char TicketType = *(buffer);
    volatile unsigned int TicketNum = TicketFindOpen(Tickets);
    volatile unsigned int TicketNum2 = TicketFindOpen(Tickets);
    volatile unsigned int TicketNum3 = TicketFindOpen(Tickets);
    //send NoFreeTicket if non are found
    if (TicketNum == -1) {
        Platter.I=URCHIN_ERROR_NoFreeTicket; //unsigned int
        (void) PackfToPI(ExchangeQueue,VPID,reinterpret_cast<const char *>(Platter.UString),sizeof(int));

        //Debug Prints
        //(void) PrintfToPI(DebugQueue,0,"URCHIN_ERROR_NoFreeTicket :Bytes: %02X %02X %02X %02X", Platter[0], Platter[1], Platter[2], Platter[3]);
        //(void) PrintfToPI(DebugQueue,0,"URCHIN_ERROR_NoFreeTicket:%d",sizeof(int));
        return -1;
    }






    PrintfToPI(DebugQueue,VPID,"FormatTicket:TicketType:%c",TicketType);


    Ticket *TicketPoint = (Ticket*) pvPortMalloc(sizeof(Ticket));
    if (TicketPoint == nullptr) {
        //todo throw error
        return -1;
    }else {
        memset(TicketPoint,0,sizeof(Ticket));

        TicketPoint->TicketNum = TicketNum;
        TicketPoint->VPID = VPID;
        TicketPoint->type = TicketType;

        Tickets[TicketNum] = TicketPoint;
    }

    //Saving the ticket to the list
    Tickets[TicketNum] = TicketPoint;


    Platter.I=URCHIN_OK; //unsigned int
    (void) PackfToPI(ExchangeQueue,VPID,reinterpret_cast<const char *>(Platter.UString),sizeof(int));



    (void) PrintfToPI(DebugQueue,VPID,"FormatTicket:Ticket:%d",TicketNum);

    Platter.UI=TicketNum;

    (void) PrintfToPI(DebugQueue,VPID,"FormatTicket:TicketS:%.*s", 4, Platter.UString);
    (void) PackfToPI(ExchangeQueue,VPID,reinterpret_cast<const char *>(Platter.UString),sizeof(unsigned int));

    return 0;
}



int LoadTicket(unsigned char VPID, const char* buffer) {
    (void) PrintfToPI(DebugQueue,0,"LoadTicket called");


    #pragma pack(push, 1)  // No padding between fields
        typedef struct{
            unsigned int ticket;
            char joint[BRIDGEMaxName];
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

    (void)(PrintfToPI)(DebugQueue,VPID,"LoadTicket:ticket:%d",Header->ticket);
    (void)(PrintfToPI)(DebugQueue,VPID,"LoadTicket:Joint:%s",Header->joint);
    (void)(PrintfToPI)(DebugQueue,VPID,"LoadTicket:command_len:%d",Header->command_len);
    (void)(PrintfToPI)(DebugQueue,VPID,"LoadTicket:values_len:%d",Header->values_len);

    char command[255]={0};
    strncpy(command,buffer+sizeof(PacketHeader),Header->command_len);


    (void)(PrintfToPI)(DebugQueue,VPID,"LoadTicket:command:%s",command);

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
    //

    BridgeMotor* Joint = GetBridge(Header->joint);

    if (Joint == NULL) {// could not find Joint in bridge list
        (void) PrintfToPI(DebugQueue,0,"joint not found");
        return -1;
    }

    //Todo: Add a catch to see if the Joint was found and if not send an error to the Pi that it will pass up to the user.

    (void)(PrintfToPI)(DebugQueue,VPID,"LoadTicket:ServoID:%d",Joint->Num);
    (void)(PrintfToPI)(DebugQueue,VPID,"LoadTicket:Brand:%s",Joint->Brand);
    (void)(PrintfToPI)(DebugQueue,VPID,"LoadTicket:Model:%s",Joint->Model);


    int MotorNum = Joint->Num;
    if (strcmp(Joint->Brand,"HerkuleX")==0) {


        HerkulexModel Model = MODEL_None;

        if (strcmp(Joint->Model,"0101")==0) {
             Model = MODEL_0101;
        }else if (strcmp(Joint->Model,"0201")==0) {
             Model = MODEL_0201;
        }else if (strcmp(Joint->Model,"0601")==0) {
             Model = MODEL_0601;
        }else if (strcmp(Joint->Model,"0602")==0) {
             Model = MODEL_0602;
        }




        //Bridge bounds check
        if (Variables[0].Data.Float < (float)Joint->BoundsMin || Variables[0].Data.Float > (float)Joint->BoundsMax) {
            (void)(PrintfToPI)(DebugQueue, VPID, "ERROR: %f is out of range (%d to %d)",
                                Variables[0].Data.Float, Joint->BoundsMin, Joint->BoundsMax);
            return URCHIN_ERROR_OutOf_Bounds;
        }

        (void)(PrintfToPI)(DebugQueue,VPID,"LoadTicket:Angle:%f",Variables[0].Data.Float);
        int Pos = AngleToPoint(HerkuleXMotorBounds[Model],(double)Variables[0].Data.Float);
        (void)(PrintfToPI)(DebugQueue,VPID,"LoadTicket:pos:%d",Pos);
        (void)(PrintfToPI)(DebugQueue,VPID,"LoadTicket:time:%d",Variables[1].Data.Int);





        Herkulex.torqueON(MotorNum);
        Herkulex.RAMRead(MotorNum,Torque);
        Herkulex.moveOne(MotorNum,Pos, Variables[1].Data.Int*PTime, static_cast<JogLedColor>(Variables[2].Data.Int), Model);


        Packet* Stamp = CreateNode(0/*0=local*/,3/*Herkulex = 2*/,MotorNum,0,reinterpret_cast<char *>(Herkulex.BusPacket),Herkulex.BusPacketLength,VPID,NULL);
       InsertionHead(&Tickets[Header->ticket]->Packets,Stamp);
        if (Tickets[Header->ticket]->Packets==NULL) {
            (void) PrintfToPI(DebugQueue,VPID,"Load: No packets in ticket");
        }
        }

    return 0;
}




int PunchTicket(unsigned char VPID, const char* buffer) {
    WebConversion TicketConversion;
    memcpy(TicketConversion.String,buffer,sizeof(unsigned int));

    unsigned int Ticket = TicketConversion.UI;

    (void) PrintfToPI(DebugQueue,VPID,"PunchTicket called");

    if (Tickets[Ticket] == NULL) {(void) PrintfToPI(DebugQueue,VPID,"PunchTicket Error ticket not real, Looking for %d",Ticket);}
    (void) PrintfToPI(DebugQueue,VPID,"Ticket:%d, Type:%c, VPID:%u",Ticket,Tickets[Ticket]->type,(unsigned int)Tickets[Ticket]->VPID);

    if (Tickets[Ticket]->Packets==NULL) {
        (void) PrintfToPI(DebugQueue,VPID,"No packets in ticket");
    }else{
        PrintPackets(DebugQueue,VPID,Tickets[Ticket]->Packets);
        PunchToQue(Tickets[Ticket]);

    }








    return 0;
}


int CloseTicket(unsigned char VPID, const char* buffer) {

    (void) PrintfToPI(DebugQueue,0,"CloseTicket not added");
return 0;
}



int TicketInfo(unsigned char VPID, const char* buffer) {

    (void) PrintfToPI(DebugQueue,0,"TicketInfo not added");
return 0;
}



int GetHealth(unsigned char VPID, const char* buffer) {
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


int Bridge(unsigned char VPID, const char* buffer) {
    (void) PrintfToPI(DebugQueue,0,"Bridge");

    if (0 == strncmp("Add",buffer,3)) {
        (void) PrintfToPI(DebugQueue,0,"BridgeAdd");
        BridgeMotor *motor = (BridgeMotor*)(buffer+3);
        (void) PrintfToPI(DebugQueue,0,"Bridge:Printing joint");
        (void) PrintfToPI(DebugQueue,0,"Bridge: Num %d", motor->Num);
        (void) PrintfToPI(DebugQueue,0,"Bridge: Brand %s", motor->Brand);
        (void) PrintfToPI(DebugQueue,0,"Bridge: Model %s", motor->Model);
        (void) PrintfToPI(DebugQueue,0,"Bridge: Joint %s", motor->Joint);
        (void) PrintfToPI(DebugQueue,0,"Bridge: Bounds Min %d", motor->BoundsMin);
        (void) PrintfToPI(DebugQueue,0,"Bridge: Bounds Max %d", motor->BoundsMax);
        (void) PrintfToPI(DebugQueue,0,"Bridge: AlignmentAngle %d", motor->AlignmentAngle);


        AddBridge(motor);

        (void) PrintfToPI(DebugQueue,0,"%s",motor->Joint);

    }
    if (0 == strncmp("Rem",buffer,3)) {

    }

    if (0 == strncmp("Read",buffer,4)) {

    }



    //(void) PrintfToPI(ExchangeQueue,"GetHealth is being added");
    //DoSomething();
    return 0;
}




//-----------------------------------------------------------------------

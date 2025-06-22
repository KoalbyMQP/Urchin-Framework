#include <sys/cdefs.h>
#include "esp_task_wdt.h"
//
// Created by gabri on 3/24/2025.
//

#include "UnPacker.h"

#include <coms.h>
#include "shipping.h"
#include "Errors.h"
#include "TicketNum.h"
#include "MSGQueue.h"
#include "GLOBAL.h"

//#include "../disable/Herkulex.h"





//espnow
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_wifi.h"
#include "esp_now.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_timer.h"

#define HX_UART_NUM UART_NUM_2
int64_t rtt=0;
static int64_t send_time_us = 0;
static bool response_received = false;


int copy_until_null(const char *src, char *dest, size_t len) {
    int i = 0;
    while (src[i] != '\0' && i < len - 1) {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
    return i;
}

void shift_string_left(char *str, int n) {
    int len = strlen(str);

    if (n >= len) {
        // If shifting past end, make it an empty string
        str[0] = '\0';
    } else {
        // Move the remainder of the string forward
        memmove(str, str + n, len - n + 1);  // +1 to include the null terminator
    }
}

_Noreturn void receiving(void *pvParameters){

    //Herkulex.beginSerial3(115200); //open serial port 1
    //Herkulex.reboot(7); //reboot
    //Herkulex.initialize();

    RollINIT(&TicketTape);


    //ESPnow setup
    nvs_flash_init();
    esp_netif_init();
    esp_event_loop_create_default();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_start();
    uint8_t receiver_mac[] = {0x78, 0x42, 0x1c, 0x6c, 0xc8, 0x34};
    esp_now_init();
    esp_now_register_recv_cb(on_data_recv);

    esp_now_peer_info_t peer = {
        .channel = 0,
        .ifidx = ESP_IF_WIFI_STA,
        .encrypt = false
    };
    memcpy(peer.peer_addr, receiver_mac, 6);
    esp_now_add_peer(&peer);


    //setup massage buffer
    RecvFrame Frame;
    uint8_t data[COMS_SIZE];



    static Context Basic[]={
            {"ReqTicket",9,ReqTicket},
            {"PunchTicket",11,PunchTicket},
            {"CloseTicket",11,CloseTicket},
            {"TicketInfo",10,TicketInfo},
            {"GetHealth",9,GetHealth},
        {"ESPNOW",6,ESPNOW}
    };





    Context *CurrentConext[PIDNUM];
    CurrentConext[0]=Basic;



    uint8_t MSG[COMS_SIZE];
    while (1){
        vTaskDelay(10 / portTICK_PERIOD_MS);  // Delay for 0.1 second
        //(void) PrintfToPI(ExchangeQueue,"hello");
        //int empty=true;
        esp_task_wdt_reset();
        int len=0;
        //int shift=0;

        len = uart_read_bytes(UART_NUM, (void *)&Frame, sizeof(RecvFrame), 100 / portTICK_PERIOD_MS);
        (void) PrintfToPI(DebugQueue,"scaning\n");






        if (len > 0) {
            data[len] = '\0';
            //Processes data

            (void) strcmp((char *)data,(char *)Frame.MSG);
            //shift = copy_until_null(data, MSG, len);
            //shift_string_left(data,shift);
            ProcessRequest(CurrentConext[0/*Frame.VPID*/],6, MSG);// !!!!Replace The 6 later with dynamic counting of the number of items in the context list



        } else {
            // no message is received so do nothing
        }





    }


}




//Basic statements
//-----------------------------------------------------------------


int ReqTicket(const char* buffer){
(void)PrintfToPI(DebugQueue,"\n\nReqTicket called");
    //PrintTape(&TicketTape);
    //Get Ticket

    unsigned int Ticket = FindFree(&TicketTape);

    //send NoFreeTicket if non are found
    if (Ticket == -1) {
        (void) PrintfToPI(ExchangeQueue,"Code:%d",NoFreeTicket);
        return -1;
    }

    PrintfToPI(DebugQueue,"Ticket:%d\n",Ticket);
    //Check out ticket
    checkOut(&TicketTape,Ticket);
    //send OK with ticket
    (void) PrintfToPI(ExchangeQueue,"Code:%d \nTicket%d ",OK,Ticket);
    (void) PrintfToPI(ExchangeQueue,"Tape%u",TicketTape);

  return 0;
}







int PunchTicket(const char* buffer) {


    (void) PrintfToPI(ExchangeQueue,"PunchTicket not added");
    return 0;
}


int CloseTicket(const char* buffer) {

    (void) PrintfToPI(ExchangeQueue,"CloseTicket not added");
return 0;
}



int TicketInfo(const char* buffer) {

    (void) PrintfToPI(ExchangeQueue,"TicketInfo not added");
return 0;
}



int GetHealth(const char* buffer) {
    if (0== strcmp("FreeRam",buffer)) {
        (void) PrintfToPI(ExchangeQueue,"%d",xPortGetFreeHeapSize());

    }
    if (0== strcmp("TotalRam",buffer)) {
        (void) PrintfToPI(ExchangeQueue,"%d",xPortGetMinimumEverFreeHeapSize());
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

int ESPNOW(const char* buffer) {

    PrintfToPI(DebugQueue,"nESPNOW called\n");
    PrintfToPI(DebugQueue,"\nESPNOW Buffer size s= |%s|\n",buffer);
    int PAYLOAD_SIZE = atoi(buffer);
    PrintfToPI(DebugQueue,"\nESPNOW Buffer size d= |%d|\n",PAYLOAD_SIZE);


    uint8_t payload[PAYLOAD_SIZE];


    memset(payload, 0x42, PAYLOAD_SIZE);

    response_received = false;


        uint8_t receiver_mac[] = {0x78, 0x42, 0x1c, 0x6c, 0xc8, 0x34};



        send_time_us = esp_timer_get_time();
        esp_now_send(receiver_mac, payload, PAYLOAD_SIZE);

        // Wait for response (timeout after 200ms)
        int wait_ms = 0;
        while (!response_received && wait_ms < 200) {
            vTaskDelay(pdMS_TO_TICKS(10));
            wait_ms += 10;
        }

        if (!response_received) {
            PrintfToPI(ExchangeQueue,"No echo received (timeout)");
        }else {
            //float delay = (float)(esp_timer_get_time() - send_time_us) / 1000.0f;
            PrintfToPI(ExchangeQueue,"\nESPNOW RTT = |%d|\n",rtt);
        }
        vTaskDelay(pdMS_TO_TICKS(1000));  // Wait 1 second before next send




    return 0;
}

void on_data_recv(const esp_now_recv_info_t *recv_info, const uint8_t *data, int len) {
    rtt = esp_timer_get_time() - send_time_us;
    PrintfToPI(ExchangeQueue, "Received echo. RTT: %lld us", rtt);
    response_received = true;
}

// sender_main.c





// Replace with receiver MAC address
uint8_t receiver_mac[] = {0x24, 0x6F, 0x28, 0xAB, 0xCD, 0xEF};





void sender_task(void *pvParameter) {

}









//PrintfToPI(DebugQueue,"Set Led Green");
//Herkulex.setLed(7,0x02); //set the led to green
//PrintfToPI(DebugQueue,"Status:");
//PrintfToPI(DebugQueue,"",Herkulex.stat(n)); //verify error code
//Herkulex.end();





//Helppers
//------------------------------------------------------------------------

int ProcessRequest(Context Commands[],size_t ComandNum,const uint8_t buffer[]) {
        int i=0;
        int found=0;
        int error=0;

    (void) PrintfToPI(DebugQueue,"ProcessRequest:%s",buffer);
        while (i < ComandNum && !found){
            if (0==strncmp((char*)buffer,Commands[i].Name,Commands[i].depth)) {
                error=Commands[i].function(SkipFoward((char*)buffer,Commands[i].depth));
                found=1;
            }
            i++;
            }
    if (!found) {return CommandNotFound;}
    return error;

}


const char* SkipFoward(const char buffer[],unsigned int distance) {
    return &buffer[distance];
}


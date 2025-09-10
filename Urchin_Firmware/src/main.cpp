// Lib headers
#include <cstring>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "driver/uart.h"


// Local headers
#include "Global/GLOBAL.h"
#include "Global/setup.h"
#include "ESP_PI_Communication/Coms.h"
#include "ESP_PI_Communication/Shipping.h"
#include "Ticketing/Ticket.h"
#include "Conversation/UnPacker.h"
#include "ESP_PI_Communication/MSGQueue.h"
#include "ESP_PI_Communication/Receving.h"

uart_config_t uart_config;



extern "C" [[noreturn]] void app_main() {
    esp_log_level_set("*", ESP_LOG_NONE);
    //sets up the seral comuncation
    SetUpUart();

    SetUpPins();



    // QueueHandle_t AsyncQueue = xQueueCreate(AsyncSize, sizeof(Ticket));
    // QueueHandle_t SequentialQueue = xQueueCreate(SequentialSize, sizeof(Ticket));
    // Ticket *interrupt;

    //printf("MSG_ITEM_SIZE:%d\n",MSG_ITEM_SIZE);
    //printf("MSG_QUEUE_LENGTH:%d\n",MSG_QUEUE_LENGTH);

    ExchangeQueue= xQueueCreate(MSG_QUEUE_LENGTH, MSG_ITEM_SIZE);

    RecationQueue = xQueueCreate(MSG_QUEUE_LENGTH, MSG_ITEM_SIZE);

    DebugQueue = xQueueCreate(MSG_QUEUE_LENGTH, MSG_ITEM_SIZE);

    if (!ExchangeQueue || !RecationQueue || !DebugQueue) {
        //rolint: ignore
        printf("One or more queues failed to allocate!\n");
        //abort();
    }

    TaskHandle_t xHandleShip = nullptr;
    //Shipping(nullptr);
    BaseType_t ShipxReturned = xTaskCreate(Shipping, "Shipping", 8048, nullptr, 3, &xHandleShip);
    if( ShipxReturned == pdPASS ){
        //printf("Shipping Task creation worked!\n");
    }else {
        //printf("Shipping Task creation failed!\n");
    }




    TaskHandle_t xHandleRecv = nullptr;
    BaseType_t RecvxReturned = xTaskCreate(receiving, "Receiving", 8048, nullptr, 3, &xHandleRecv);
    if( RecvxReturned == pdPASS ){
        //printf("Receiving Task creation worked!\n");
    }else {
        //printf("Receiving Task creation failed!\n");
    }


    //Shipping(nullptr);
int nothing=0;
    while(1) {
        vTaskDelay(100 / portTICK_PERIOD_MS);
        nothing=1;
        nothing=nothing;
    }

}











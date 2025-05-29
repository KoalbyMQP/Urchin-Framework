#include <cstring>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "driver/uart.h"

#include "GLOBAL.h"
#include "setup.h"
#include "coms.h"
#include "shipping.h"
#include "Ticket.h"
#include "UnPacker.h"

uart_config_t uart_config;



extern "C" [[noreturn]] void app_main() {
    //sets up the seral comuncation
    SetUpUart();



    QueueHandle_t AsyncQueue = xQueueCreate(AsyncSize, sizeof(Ticket));
    QueueHandle_t SequentialQueue = xQueueCreate(SequentialSize, sizeof(Ticket));


    TaskHandle_t xHandleShip = nullptr;
    //Shipping(nullptr);
    BaseType_t xReturned = xTaskCreate(Shipping, "Shipping", 8048, nullptr, 3, &xHandleShip);
    if( xReturned == pdPASS ){
        printf("Shipping Task creation worked!\n");
    }else {
        printf("Shipping Task creation failed!\n");
    }



    TaskHandle_t xHandleRec = nullptr;
    xReturned = xTaskCreate(receiving, "Receiving", 8048, nullptr, 3, &xHandleRec);
    if( xReturned == pdPASS ){
        printf("Receiving Task creation worked!\n");
    }else {
        printf("Receiving Task creation failed!\n");
    }


    while (true) {


        vTaskDelay(100 / portTICK_PERIOD_MS);  // Delay for 0.1 second
    }

}







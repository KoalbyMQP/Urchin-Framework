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

uart_config_t uart_config;



extern "C" [[noreturn]] void app_main() {
    //sets up the seral comuncation
    SetUpUart();



    QueueHandle_t AsyncQueue = xQueueCreate(AsyncSize, sizeof(Ticket));
    QueueHandle_t SequentialQueue = xQueueCreate(SequentialSize, sizeof(Ticket));


    TaskHandle_t xHandle = nullptr;
    Shipping(nullptr);
    BaseType_t xReturned = xTaskCreate(Shipping, "Shipping", 8048, nullptr, 3, &xHandle);
    if( xReturned == pdPASS ){
        printf("Shipping Task creation worked!\n");
    }else {
        printf("Shipping Task creation failed!\n");
    }

    while (true) {



    }

}







#include <cstring>
//#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_sleep.h"
#include "esp_log.h"
#include "driver/uart.h"

#include "GLOBAL.h"
#include "setup.h"
#include "coms.h"
#include "shipping.h"


uart_config_t uart_config;



extern "C" [[noreturn]] void app_main() {
    int i=0;
    SetUpPins();
    esp_sleep_enable_ext0_wakeup(static_cast<gpio_num_t>(SLAP_PIN), 1);

    //set up our message Que
    QueueHandle_t MSGQueue = xQueueCreate(MSG_QUEUE_LENGTH, MSG_ITEM_SIZE);

    gpio_set_direction(static_cast<gpio_num_t>(LED_Working), GPIO_MODE_OUTPUT);
    gpio_set_direction(static_cast<gpio_num_t>(LED_BUT), GPIO_MODE_OUTPUT);
    gpio_set_direction(static_cast<gpio_num_t>(ButtonPin), GPIO_MODE_INPUT);
    gpio_set_direction(static_cast<gpio_num_t>(SLAP_PIN), GPIO_MODE_INPUT);


    TaskHandle_t xHandle = nullptr;
    Shipping(nullptr);
    BaseType_t xReturned = xTaskCreate(Shipping, "Shipping", 8048, MSGQueue, 3, &xHandle);
    if( xReturned == pdPASS ){
        //printf("Shipping Task creation worked!\n");
    }else {
        //printf("Shipping Task creation failed!\n");
    }

    while (true) {

            PrintfToPI(MSGQueue,"Hello, ESP-IDF World!%d\n",i);
            //printf("print error %d",PrintfToPI(MSGQueue,"shipping workingeeeeeeeeeeeeeeeeeee"));



        i++;
        gpio_set_level(static_cast<gpio_num_t>(LED_Working), 1);
        gpio_set_level(static_cast<gpio_num_t>(LED_BUT), 0);
        vTaskDelay(1000 / portTICK_PERIOD_MS);


        //printf("Flip flop\n");
        PrintfToPI(MSGQueue,"Flip flop\n");

        gpio_set_level(static_cast<gpio_num_t>(LED_Working), 0);
        gpio_set_level(static_cast<gpio_num_t>(LED_BUT), 1);
        vTaskDelay(1000 / portTICK_PERIOD_MS);

    }

}







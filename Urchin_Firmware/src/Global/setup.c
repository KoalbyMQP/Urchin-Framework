//
// Created by gabri on 3/23/2025.
//

#include "setup.h"
#include "GLOBAL.h"
#include "MSGQueue.h"
#include "Shipping.h"
//#include "esp_core_dump.h"


void SetUpUart() {


    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .rx_flow_ctrl_thresh = 122,  // Common safe value
        .source_clk = UART_SCLK_APB  // Ensure flags are set
    };
    uart_param_config(UART_NUM, &uart_config);

    // Install UART driver
    uart_driver_install(UART_NUM,  sizeof(Box)*12, sizeof(Box)*2, 0, NULL, 0);

}



void SetUpPins() {
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << CherpPin),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
};
    gpio_config(&io_conf);


}
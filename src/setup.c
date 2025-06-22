//
// Created by gabri on 3/23/2025.
//

#include "setup.h"
#include "unpacker.h"
#include "shipping.h"




void SetUpUart() {


    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .rx_flow_ctrl_thresh = 122,  // Common safe value
        .flags = { .allow_pd = 0, .backup_before_sleep = 0 }  // Ensure flags are set
    };
    uart_param_config(UART_NUM, &uart_config);

    // Install UART driver
    uart_driver_install(UART_NUM, sizeof(RecvFrame)*2, sizeof(uint8_t)+sizeof(char)+sizeof(char)*1024, 0, NULL, 0);

}
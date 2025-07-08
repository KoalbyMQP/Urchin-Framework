//
// Created by gabri on 3/23/2025.
//

#include "setup.h"

#include "MSGQueue.h"
#include "shipping.h"
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
    uart_driver_install(UART_NUM,  sizeof(Box), sizeof(Box), 0, NULL, 0);

}


// void esp_core_dump_to_custom(const void* core_dump_data, size_t data_len) {
  ///  printf("ESP core dump:!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
    // const uint8_t* data = (const uint8_t*) core_dump_data;
    // size_t offset = 0;
    //
    // while (offset < data_len) {
    //     size_t chunk_size = COMS_SIZE;
    //     if (offset + chunk_size > data_len) {
    //         chunk_size = data_len - offset;
    //     }
    //
    //     SendMessage(0, 'F', data + offset, chunk_size);
    //     offset += chunk_size;
    // }

 //}
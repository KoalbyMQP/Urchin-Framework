// Host stand-in for ESP-IDF's UART driver. UART0 = a pty (the USB link to
// the host), UART1 = a simulated Herkulex bus (sim.cpp).
#pragma once
#include "../esp_err.h"
#include "../freertos/FreeRTOS.h"
#include <cstddef>
typedef int uart_port_t;
#define UART_NUM_0 0
#define UART_NUM_1 1
#define UART_PIN_NO_CHANGE (-1)
typedef enum { UART_DATA_8_BITS = 3 } uart_word_length_t;
typedef enum { UART_PARITY_DISABLE = 0 } uart_parity_t;
typedef enum { UART_STOP_BITS_1 = 1 } uart_stop_bits_t;
typedef enum { UART_HW_FLOWCTRL_DISABLE = 0 } uart_hw_flowcontrol_t;
typedef enum { UART_SCLK_APB = 0, UART_SCLK_DEFAULT = 0 } uart_sclk_t;
typedef struct {
    int baud_rate;
    uart_word_length_t data_bits;
    uart_parity_t parity;
    uart_stop_bits_t stop_bits;
    uart_hw_flowcontrol_t flow_ctrl;
    uint8_t rx_flow_ctrl_thresh;
    uint8_t rx_glitch_filt_thresh;
    uart_sclk_t source_clk;
    struct { uint32_t allow_pd : 1; uint32_t backup_before_sleep : 1; } flags;
} uart_config_t;
esp_err_t uart_driver_install(uart_port_t, int, int, int, void*, int);
esp_err_t uart_driver_delete(uart_port_t);
esp_err_t uart_param_config(uart_port_t, const uart_config_t*);
esp_err_t uart_set_pin(uart_port_t, int, int, int, int);
int uart_write_bytes(uart_port_t, const void* src, size_t size);
int uart_read_bytes(uart_port_t, void* buf, uint32_t length, TickType_t wait);
esp_err_t uart_flush(uart_port_t);
esp_err_t uart_flush_input(uart_port_t);
esp_err_t uart_wait_tx_done(uart_port_t, TickType_t);
esp_err_t uart_set_baudrate(uart_port_t, uint32_t baudrate);

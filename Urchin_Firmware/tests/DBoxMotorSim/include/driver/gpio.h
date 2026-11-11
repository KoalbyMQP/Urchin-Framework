#pragma once
typedef int gpio_num_t;
typedef enum { GPIO_PULLUP_ONLY, GPIO_FLOATING } gpio_pull_mode_t;
inline int gpio_set_pull_mode(gpio_num_t, gpio_pull_mode_t) { return 0; }

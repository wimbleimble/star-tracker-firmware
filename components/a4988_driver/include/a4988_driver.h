#ifndef A4988_DRIVER_H
#define A4988_DRIVER_H
#include "driver/gpio.h"

typedef struct a4988_driver_config {
    gpio_num_t dir_gpio;
    gpio_num_t step_gpio;
    gpio_num_t ms1_gpio;
    gpio_num_t ms2_gpio;
    gpio_num_t ms3_gpio;
    gpio_num_t sleep_gpio;
    gpio_num_t reset_gpio;
    gpio_num_t enable_gpio;

} a4988_driver_config_t;

void a4988_driver_init(const a4988_driver_config_t* config);
void a4988_rotate_continuous(double omega);
void a4988_stop();
void a4988_set_direction(uint32_t direction);

#endif

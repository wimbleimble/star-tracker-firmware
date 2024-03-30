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

typedef enum step_mode {
    STEP_MODE_FULL,
    STEP_MODE_HALF,
    STEP_MODE_QUARTER,
    STEP_MODE_EIGHTH,
    STEP_MODE_SIXTEENTH,
    STEP_MODE_MAX
} step_mode_t;

#define STEP_MODE_TO_MUL(SM) ((uint8_t)1 << SM)

void a4988_driver_init(const a4988_driver_config_t* config);
void a4988_rotate_continuous(double omega);
void a4988_set_step_mode(step_mode_t mode);
step_mode_t a4988_get_step_mode();
void a4988_stop();

#endif

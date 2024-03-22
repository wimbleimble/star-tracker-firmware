#ifndef PIN_DEFINITIONS_H
#define PIN_DEFINITIONS_H

// Stepper Motor Controller
#define A4988_EN_PIN GPIO_NUM_23
#define A4988_RST_PIN GPIO_NUM_18
#define A4988_SLP_PIN GPIO_NUM_5
#define A4988_MS1_PIN GPIO_NUM_22
#define A4988_MS2_PIN GPIO_NUM_21
#define A4988_MS3_PIN GPIO_NUM_19
#define A4988_STEP_PIN GPIO_NUM_17
#define A4988_DIR_PIN GPIO_NUM_16

// Auxiliary GPIO Connector
#define IO_PIN_2 GPIO_NUM_25
#define IO_PIN_3 GPIO_NUM_33
#define IO_PIN_4 GPIO_NUM_26
#define IO_PIN_5 GPIO_NUM_32
#define IO_PIN_6 GPIO_NUM_27
#define IO_PIN_7 GPIO_NUM_35

// WIFI Config
#define SSID "Star Tracker"
#define PASSWORD "fuckfuck"
#define CHANNEL 11
#define HOSTNAME "star-tracker"
#define FRIENDLY_NAME "Star Tracker"
#define MAX_CONNECTIONS 2

#endif

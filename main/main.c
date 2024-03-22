#include "esp_log.h"
#include "network.h"
#include "nvs_flash.h"
#include "web_server.h"
#include "a4988_driver.h"
#include "star_tracker_config.h"

static const char* TAG = "Main";

void stepper_init()
{
    const a4988_driver_config_t a4988_config = {
        .dir_gpio = A4988_DIR_PIN,
        .step_gpio = A4988_STEP_PIN,
        .enable_gpio = A4988_EN_PIN,
        .sleep_gpio = A4988_SLP_PIN,
        .reset_gpio = A4988_RST_PIN,
        .ms1_gpio = A4988_MS1_PIN,
        .ms2_gpio = A4988_MS2_PIN,
        .ms3_gpio = A4988_MS3_PIN,
    };
    a4988_driver_init(&a4988_config);
}

void app_main(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES
        || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "Initialising stepper motor driver...");
    stepper_init();

    ESP_LOGI(TAG, "Initialising WIFI access point...");
    wifi_init();

    ESP_LOGI(TAG, "Initialising mDNS service...");
    mdns_service_init();

    ESP_LOGI(TAG, "Launching web server...");
    web_server_init();
}

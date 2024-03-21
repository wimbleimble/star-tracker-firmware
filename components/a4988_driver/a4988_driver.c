#include "a4988_driver.h"

#include "driver/rmt_tx.h"
#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define RMT_RESOLUTION (1 * 1000 * 1000)
#define PULSE_LENGTH_US ((uint16_t)2)
#define PI ((double)3.1415926)

static a4988_driver_config_t driver_config;
static rmt_channel_handle_t tx_chan = NULL;
static const rmt_transmit_config_t tx_config = {
    .loop_count = -1, .flags.eot_level = 0, .flags.queue_nonblocking = false
};
static rmt_encoder_handle_t copy_encoder_handle;
const char* TAG = "a4988_driver";

void a4988_driver_init(const a4988_driver_config_t* config)
{
    driver_config = *config;
    uint64_t gpio_bit_mask = (1ULL << config->dir_gpio)
        | (1ULL << config->ms1_gpio) | (1ULL << config->ms2_gpio)
        | (1ULL << config->ms3_gpio) | (1ULL << config->sleep_gpio)
        | (1ULL << config->enable_gpio) | (1ULL << config->reset_gpio);
    ESP_LOGI(TAG, "gpio_bit_mask %" PRIu64, gpio_bit_mask);

    gpio_config_t gpio_cfg = { .intr_type = GPIO_INTR_DISABLE,
                               .mode = GPIO_MODE_OUTPUT,
                               .pull_up_en = 0,
                               .pull_down_en = 0,
                               .pin_bit_mask = gpio_bit_mask };

    ESP_ERROR_CHECK(gpio_config(&gpio_cfg));

    ESP_ERROR_CHECK(gpio_set_level(config->dir_gpio, 0));
    ESP_ERROR_CHECK(gpio_set_level(config->sleep_gpio, 0));
    ESP_ERROR_CHECK(gpio_set_level(config->reset_gpio, 1));
    ESP_ERROR_CHECK(gpio_set_level(config->enable_gpio, 1));
    ESP_ERROR_CHECK(gpio_set_level(config->ms1_gpio, 0));
    ESP_ERROR_CHECK(gpio_set_level(config->ms2_gpio, 0));
    ESP_ERROR_CHECK(gpio_set_level(config->ms3_gpio, 0));

    rmt_tx_channel_config_t tx_chan_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .gpio_num = config->step_gpio,
        .mem_block_symbols = 64,
        .resolution_hz = RMT_RESOLUTION, // 1MHz, 1us per tick
        .trans_queue_depth = 4,
        .flags.invert_out = false,
        .flags.with_dma = false,
    };
    ESP_ERROR_CHECK(rmt_new_tx_channel(&tx_chan_config, &tx_chan));
    const rmt_copy_encoder_config_t encoder_config = {};
    ESP_ERROR_CHECK(
        rmt_new_copy_encoder(&encoder_config, &copy_encoder_handle));
}
void a4988_rotate_continuous(double omega)
{
    const uint16_t t1 = PULSE_LENGTH_US * (RMT_RESOLUTION / (1 * 1000 * 1000));
    static rmt_symbol_word_t block[3] = {
        { .level0 = 1, .level1 = 1, .duration0 = t1/2 , .duration1 = t1/2},
        { .level0 = 0, .level1 = 0, },
        { .level0 = 0, .level1 = 0, }
    };
    const uint32_t t2 =
        ((uint32_t)(((double)RMT_RESOLUTION * 2.0 * PI) / (omega * 400.0)) - (uint32_t)t1);

    block[1].duration0 = (uint16_t)(t2/4);
    block[1].duration1 = (uint16_t)(t2/4);
    block[2].duration0 = (uint16_t)(t2/4);
    block[2].duration1 = (uint16_t)(t2/4);
    ESP_ERROR_CHECK(gpio_set_level(driver_config.sleep_gpio, 1));
    ESP_ERROR_CHECK(gpio_set_level(driver_config.enable_gpio, 0));
    vTaskDelay(1 / portTICK_PERIOD_MS);

    ESP_LOGI(TAG, "t1: %d, t2: %" PRIu32 " t2/2: %" PRIu16, t1, t2, block[1].duration0);

    ESP_ERROR_CHECK(rmt_enable(tx_chan));
    ESP_ERROR_CHECK(rmt_transmit(
        tx_chan,
        copy_encoder_handle,
        &block,
        3 * sizeof(rmt_symbol_word_t),
        &tx_config));
}

void a4988_stop()
{
    ESP_ERROR_CHECK(rmt_disable(tx_chan));
    ESP_ERROR_CHECK(gpio_set_level(driver_config.enable_gpio, 1));
    ESP_ERROR_CHECK(gpio_set_level(driver_config.sleep_gpio, 0));
}
void a4988_set_direction(uint32_t direction) {}

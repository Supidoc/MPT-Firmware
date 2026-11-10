#include <stdio.h>
#include "spi2.h"
#include "display.h"
#include "esp_timer.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lvgl.h"
#include "input.h"
#include "i2c.h"
#include "bq27441.h"

static bq27441_handle_t s_fuel_gauge;

void app_main(void)
{
    ESP_ERROR_CHECK(spi2_init());
    ESP_ERROR_CHECK(input_init());
    ESP_ERROR_CHECK(lcd_init());

#if CONFIG_DEBUG_LED_ON == 1
    gpio_config_t gpio_cfg = {
        .pin_bit_mask = (1ULL << CONFIG_DEBUG_LED_GPIO),
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };

    gpio_config(&gpio_cfg);

    while (1) {
        // Do work

        vTaskDelay(pdMS_TO_TICKS(1000)); // Wait 1 second
        gpio_set_level(CONFIG_DEBUG_LED_GPIO, 1); // Set GPIO 2 high

        vTaskDelay(pdMS_TO_TICKS(1000)); // Wait 1 second
        gpio_set_level(CONFIG_DEBUG_LED_GPIO, 0); // Set GPIO 2 low

    }
#endif

}
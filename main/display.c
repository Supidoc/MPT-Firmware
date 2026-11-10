/************************************************************
 * @file    display.c
 * @brief   Filedescription
 * @author  dg
 * @date    01 Oct 2026
 ************************************************************/

/********************
 *     Includes    *
 ********************/
#include "display.h"
#include "driver/spi_master.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "esp_timer.h"
#include "esp_check.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "gui.h"

/************************************
 *     Private Macros / Defines    *
 ************************************/

/**
* @brief Size of the partial buffer for LVGL
* This macro defines the size of the partial buffer used by LVGL for rendering the display.
* The times 2 is for RGB565 format, which uses 2 bytes per pixel. LVGL Recommends using a buffer size of at least 1/10th of the screen size for optimal performance.
*/
#define LCD_PARTIAL_BUFFER_SIZE (CONFIG_LCD_WIDTH * CONFIG_LCD_HEIGHT * 2 / CONFIG_LCD_PARTIAL_BUFFER_DIVIDER)

/**
 * @brief LCD display rotation
 * This macro defines the rotation of the LCD display based on the configuration settings.
 * It can be set to 0, 90, 180, or 270 degrees.
 */
#if defined(CONFIG_LCD_ROTATION_NONE)
#define LCD_DISPLAY_ROTATION LV_DISPLAY_ROTATION_0
#elif defined(CONFIG_LCD_ROTATION_90)
#define LCD_DISPLAY_ROTATION LV_DISPLAY_ROTATION_90
#elif defined(CONFIG_LCD_ROTATION_180)
#define LCD_DISPLAY_ROTATION LV_DISPLAY_ROTATION_180
#elif defined(CONFIG_LCD_ROTATION_270)
#define LCD_DISPLAY_ROTATION LV_DISPLAY_ROTATION_270
#else
#error "No LCD rotation selected"
#endif

/**
 * @brief Tag for logging
 * This tag is used for logging messages related to the display module.
 */
#define TAG "display"

/***************************
 *     Private Typedefs     *
 ***************************/

/*****************************************
 *     Private Function Declarations     *
 *****************************************/

void     lcd_send_cmd(lv_display_t *disp, const uint8_t *cmd, size_t cmd_size, const uint8_t *param, size_t param_size);
void     lcd_send_color(lv_display_t *disp, const uint8_t *cmd, size_t cmd_size, uint8_t *param, size_t param_size);
uint32_t lcd_tick_get_cb(void);
void     lcd_task(void *arg);
static esp_err_t   lcd_lvgl_init(void);
static esp_err_t   lcd_hardware_init(void);

/****************************
 *     Public Variables     *
 ****************************/

/*****************************
 *     Private Variables     *
 *****************************/

/**
 * @brief Handle for the SPI device representing the LCD
 */
static spi_device_handle_t s_lcd_spi_device;

/**
 * @brief Pointer to the LVGL display structure
 */
static lv_display_t       *s_display_handle;

/**
 * @brief Mutex to prevent a second transaction to the lcd from being initiated while one is in progress
 */
static SemaphoreHandle_t s_lcd_transaction_mutex;

/**
 * @brief Task handle for the LCD task
 * This is the handle for the FreeRTOS task that handles the LCD operations.
 */
static TaskHandle_t s_lcd_task_handle                            = NULL;

/**
 * @brief LVGL buffer 1
 * This is the first buffer used by LVGL for rendering the display.
 */
static uint8_t s_lvgl_buffer_1[LCD_PARTIAL_BUFFER_SIZE] __attribute__((aligned(4)));

#ifdef CONFIG_LCD_DOUBLE_BUFFERING
/**
 * @brief LVGL buffer 2
 * This is the second buffer used by LVGL for rendering the display. This buffer is only used when double buffering is enabled.
 */
static uint8_t s_lvgl_buffer_2[LCD_PARTIAL_BUFFER_SIZE] __attribute__((aligned(4)));
#endif

/*******************************************
 *     Public Function Implementations     *
 *******************************************/

esp_err_t lcd_init(void)
{
    BaseType_t result = xTaskCreate(
                            lcd_task,
                            "lcd_task",
                            CONFIG_LCD_TASK_STACK_SIZE,
                            NULL,
                            CONFIG_LCD_TASK_PRIORITY,
                            &s_lcd_task_handle
                        );

    return result == pdPASS ? ESP_OK : ESP_ERR_NO_MEM;

    return ESP_OK;
}

bool lcd_get_display(lv_display_t **pDisp)
{
    if (pDisp == NULL) {
        return false;
    }

    if (s_display_handle == NULL) {
        return false;
    }

    *pDisp = s_display_handle;

    return true;
}

/********************************************
 *     Private Function Implementations     *
 ********************************************/

/**
 * @brief Send a command to the LCD
 * This function sends a command to the LCD and waits for the transaction to complete.
 * @param disp Pointer to the display
 * @param cmd Pointer to the command.
 * @param cmd_size Size of the command
 * @param param Pointer to the parameter.
 * @param param_size Size of the parameter
 */
void lcd_send_cmd(lv_display_t *disp, const uint8_t *cmd, size_t cmd_size, const uint8_t *param, size_t param_size)
{
    if (cmd == NULL || cmd_size != 1 || (param == NULL && param_size != 0)) {
        return;
    }

    if (xSemaphoreTake(s_lcd_transaction_mutex, portMAX_DELAY) != pdTRUE) {

        while (1)
            ;
    }

    esp_err_t ret = spi_device_acquire_bus(s_lcd_spi_device, portMAX_DELAY);
    if (ret != ESP_OK) {
        xSemaphoreGive(s_lcd_transaction_mutex);
        return;
    }

    spi_transaction_ext_t command_transaction = {
        .base = {
            .flags = SPI_TRANS_VARIABLE_CMD | (param_size > 0 ? SPI_TRANS_CS_KEEP_ACTIVE : 0),
        },
        .command_bits = 8,
    };
    command_transaction.base.cmd = cmd[0] & 0xFF;

    gpio_set_level(CONFIG_LCD_DCX_GPIO, 0);
    ret = spi_device_transmit(s_lcd_spi_device, &command_transaction.base);

    if (ret == ESP_OK && param_size > 0) {
        spi_transaction_ext_t data_transaction = {
            .base = {
                .flags    = SPI_TRANS_VARIABLE_CMD,
                .length   = param_size * 8,
                .tx_buffer = param,
            },
            .command_bits = 0,
        };
        gpio_set_level(CONFIG_LCD_DCX_GPIO, 1);
        ret = spi_device_transmit(s_lcd_spi_device, &data_transaction.base);
    }

    spi_device_release_bus(s_lcd_spi_device);
    xSemaphoreGive(s_lcd_transaction_mutex);
}

/**
 * @brief Send color data to the LCD
 * This function sends color data to the LCD and queues the transaction for background processing.
 * @param disp Pointer to the display
 * @param cmd Pointer to the command
 * @param cmd_size Size of the command
 * @param param Pointer to the parameter
 * @param param_size Size of the parameter
 */
void lcd_send_color(lv_display_t *disp, const uint8_t *cmd, size_t cmd_size, uint8_t *param, size_t param_size)
{
    if (cmd == NULL || cmd_size != 1 || (param == NULL && param_size != 0)) {
        return;
    }

    if (xSemaphoreTake(s_lcd_transaction_mutex, portMAX_DELAY) != pdTRUE) {

        while (1)
            ;
    }

    size_t bytes_sent = 0;
    esp_err_t ret = spi_device_acquire_bus(s_lcd_spi_device, portMAX_DELAY);
    if (ret != ESP_OK) {
        xSemaphoreGive(s_lcd_transaction_mutex);
        return;
    }

    spi_transaction_ext_t command_transaction = {
        .base = {
            .flags = SPI_TRANS_VARIABLE_CMD | SPI_TRANS_CS_KEEP_ACTIVE,
        },
        .command_bits = 8,
    };
    command_transaction.base.cmd = cmd[0] & 0xFF;

    gpio_set_level(CONFIG_LCD_DCX_GPIO, 0);
    ret = spi_device_transmit(s_lcd_spi_device, &command_transaction.base);
    if (ret != ESP_OK) {
        spi_device_release_bus(s_lcd_spi_device);
        xSemaphoreGive(s_lcd_transaction_mutex);
        lv_display_flush_ready(disp);
        return;
    }

    gpio_set_level(CONFIG_LCD_DCX_GPIO, 1);
    while (bytes_sent < param_size) {
        size_t chunk_size = param_size - bytes_sent;
        if (chunk_size > CONFIG_SPI2_MAX_TRANSFER_SIZE) {
            chunk_size = CONFIG_SPI2_MAX_TRANSFER_SIZE;
        }

        spi_transaction_ext_t transaction = {
            .base = {
                .flags    = SPI_TRANS_VARIABLE_CMD
                | (bytes_sent + chunk_size < param_size ? SPI_TRANS_CS_KEEP_ACTIVE : 0),
                .length   = chunk_size * 8,
                .tx_buffer = param + bytes_sent,
            },
            .command_bits = 0,
        };

        ret = spi_device_transmit(s_lcd_spi_device, &transaction.base);
        if (ret != ESP_OK) {
            spi_device_release_bus(s_lcd_spi_device);
            xSemaphoreGive(s_lcd_transaction_mutex);
            lv_display_flush_ready(disp);
            return;
        }

        bytes_sent += chunk_size;
    }

    spi_device_release_bus(s_lcd_spi_device);
    lv_display_flush_ready(disp);
    xSemaphoreGive(s_lcd_transaction_mutex);
}

/**
 * @brief Wrapper function for esp_timer_get_time() / 1000
 * @return The current time in milliseconds.
 */
uint32_t lcd_tick_get_cb(void)
{
    return esp_timer_get_time() / 1000;
}

/**
 * @brief Task function for handling LVGL timers
 * This function runs in a FreeRTOS task and handles LVGL timers. It calls the lv_timer_handler() function to process any pending timers and then sleeps for the appropriate amount of time until the next timer is ready.
 * @param arg Pointer to the task argument (not used).
 */
void lcd_task(void *arg)
{
    esp_err_t err = lcd_hardware_init();

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "LCD hardware initialization failed: %s",
                 esp_err_to_name(err));
        vTaskDelete(NULL);
        return;
    }

    err = lcd_lvgl_init();

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "LVGL initialization failed: %s",
                 esp_err_to_name(err));
        vTaskDelete(NULL);
        return;
    }

    gui_init(); // Initialize the GUI after LVGL is initialized

    gpio_set_level(CONFIG_LCD_BACKLIGHT_GPIO, 1); // Turn on backlight after initialization

    while (true) {
        uint32_t time_until_next = lv_timer_handler();

        if (time_until_next == LV_NO_TIMER_READY) {
            time_until_next = LV_DEF_REFR_PERIOD;
        }

        lv_sleep_ms(time_until_next);
    }
}

static esp_err_t lcd_lvgl_init(void)
{
    lv_init();
    lv_tick_set_cb(lcd_tick_get_cb);

    s_display_handle = lv_nv3007_create(
                           CONFIG_LCD_HEIGHT,
                           CONFIG_LCD_WIDTH,
                           LV_LCD_FLAG_NONE,
                           lcd_send_cmd,
                           lcd_send_color
                       );

    if (s_display_handle == NULL) {
        return ESP_ERR_NO_MEM;
    }

    lv_nv3007_set_gap(s_display_handle, 0, 14);
    lv_display_set_rotation(s_display_handle, LCD_DISPLAY_ROTATION);
    lv_display_set_color_format(
        s_display_handle,
        LV_COLOR_FORMAT_RGB565_SWAPPED
    );

#ifdef CONFIG_LCD_DOUBLE_BUFFERING
    lv_display_set_buffers(
        s_display_handle,
        s_lvgl_buffer_1,
        s_lvgl_buffer_2,
        LCD_PARTIAL_BUFFER_SIZE,
        LV_DISPLAY_RENDER_MODE_PARTIAL
    );
#else
    lv_display_set_buffers(
        s_display_handle,
        s_lvgl_buffer_1,
        NULL,
        LCD_PARTIAL_BUFFER_SIZE,
        LV_DISPLAY_RENDER_MODE_PARTIAL
    );
#endif

    return ESP_OK;
}

static esp_err_t lcd_hardware_init(void)
{
    s_lcd_transaction_mutex = xSemaphoreCreateMutex();
    if (s_lcd_transaction_mutex == NULL) {
        ESP_LOGE(TAG, "Failed to create mutex for LCD transaction");
        return ESP_ERR_NO_MEM;
    }

    spi_device_interface_config_t display_spi_devcfg = {
        .command_bits     = 8,
        .address_bits     = 0,
        .dummy_bits       = 0,
        .mode             = 0,
        .clock_source     = SPI_CLK_SRC_DEFAULT,
        .duty_cycle_pos   = 128,
        .cs_ena_pretrans  = 0,
        .cs_ena_posttrans = 0,
        .clock_speed_hz   = CONFIG_LCD_SPI_CLOCK_SPEED_HZ,
        .input_delay_ns   = 0,
        .sample_point     = SPI_SAMPLING_POINT_PHASE_0,
        .spics_io_num     = CONFIG_SPI2_CS_GPIO,
        .queue_size       = 7,
        .post_cb          = NULL,
    };

    ESP_RETURN_ON_ERROR(spi_bus_add_device(SPI2_HOST, &display_spi_devcfg, &s_lcd_spi_device), TAG, "Failed to add SPI device");

    gpio_config_t rst_gpio_cfg = {
        .pin_bit_mask = (1ULL << CONFIG_LCD_RST_GPIO),
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };

    ESP_RETURN_ON_ERROR(gpio_config(&rst_gpio_cfg), TAG, "Failed to configure LCD RST");

    gpio_config_t dcx_gpio_cfg = {
        .pin_bit_mask = (1ULL << CONFIG_LCD_DCX_GPIO),
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };

    ESP_RETURN_ON_ERROR(gpio_config(&dcx_gpio_cfg), TAG, "Failed to configure LCD DCX");

    gpio_config_t bl_gpio_cfg = {
        .pin_bit_mask = (1ULL << CONFIG_LCD_BACKLIGHT_GPIO),
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };

    ESP_RETURN_ON_ERROR(gpio_config(&bl_gpio_cfg), TAG, "Failed to configure LCD BACKLIGHT");

    gpio_set_level(CONFIG_LCD_BACKLIGHT_GPIO, 0); // Turn off backlight for initialization

    gpio_set_level(CONFIG_LCD_RST_GPIO, 1);
    vTaskDelay(pdMS_TO_TICKS(10));
    gpio_set_level(CONFIG_LCD_RST_GPIO, 0);
    vTaskDelay(pdMS_TO_TICKS(10));
    gpio_set_level(CONFIG_LCD_RST_GPIO, 1);
    vTaskDelay(pdMS_TO_TICKS(10));

    return ESP_OK;
}
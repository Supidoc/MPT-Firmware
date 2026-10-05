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

/************************************
 *     Private Macros / Defines    *
 ************************************/

/***************************
 *     Private Typedefs     *
 ***************************/

/*****************************************
 *     Private Function Declarations     *
 *****************************************/

void     lcd_post_transaction_cb(spi_transaction_t *trans);
void     lcd_send_cmd(lv_display_t *disp, const uint8_t *cmd, size_t cmd_size, const uint8_t *param, size_t param_size);
void     lcd_send_color(lv_display_t *disp, const uint8_t *cmd, size_t cmd_size, uint8_t *param, size_t param_size);
uint32_t lcd_tick_get_cb(void);
void     lcd_task(void *arg);

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
 * @brief SPI transaction for current LCD operation
 */
static spi_transaction_t s_lcd_transaction;

/**
 * @brief Mutex to prevent a second transaction to the lcd from being initiated while one is in progress
 */
static SemaphoreHandle_t s_lcd_transaction_mutex;

/**
 * @brief Device configuration for NV3007
 * This structure contains the configuration parameters for the NV3007 LCD controller.
 */
static spi_device_interface_config_t s_display_spi_devcfg = {
    .command_bits     = 8,
    .address_bits     = 24,
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
    .post_cb          = lcd_post_transaction_cb,
};

/**
 * @brief Task handle for the LCD task
 * This is the handle for the FreeRTOS task that handles the LCD operations.
 */
static TaskHandle_t s_lcd_task_handle                            = NULL;

/**
 * @brief LVGL buffer 1
 * This is the first buffer used by LVGL for rendering the display.
 */
static uint8_t s_lvgl_buffer_1[CONFIG_LCD_BUFFER_SIZE];

#ifdef CONFIG_LCD_DOUBLE_BUFFERING
/**
 * @brief LVGL buffer 2
 * This is the second buffer used by LVGL for rendering the display. This buffer is only used when double buffering is enabled.
 */
static uint8_t s_lvgl_buffer_2[CONFIG_LCD_BUFFER_SIZE];
#endif

/*******************************************
 *     Public Function Implementations     *
 *******************************************/

esp_err_t lcd_init(void)
{

    s_lcd_transaction_mutex = xSemaphoreCreateMutex();
    if (s_lcd_transaction_mutex == NULL) {
        return ESP_ERR_NO_MEM;
    }

    esp_err_t ret = spi_bus_add_device(SPI2_HOST, &s_display_spi_devcfg, &s_lcd_spi_device);
    if (ret != ESP_OK) {
        return ret;
    }

    lv_init();

    lv_tick_set_cb(lcd_tick_get_cb);

    BaseType_t retRtos = xTaskCreate(lcd_task, "lcd_task", CONFIG_LCD_TASK_STACK_SIZE, NULL, CONFIG_LCD_TASK_PRIORITY, &s_lcd_task_handle);
    if (retRtos != pdPASS) {
        return ESP_ERR_NOT_FINISHED;
    }

    s_display_handle = lv_nv3007_create(142, 428, LV_LCD_FLAG_NONE, lcd_send_cmd, lcd_send_color);
    lv_nv3007_set_gap(s_display_handle, 0, 14);
    lv_display_set_rotation(s_display_handle, LV_DISPLAY_ROTATION_270);
    lv_display_set_color_format(s_display_handle, LV_COLOR_FORMAT_RGB565_SWAPPED);

#ifdef CONFIG_LCD_DOUBLE_BUFFERING
    lv_display_set_buffers(s_display_handle, s_lvgl_buffer_1, s_lvgl_buffer_2, CONFIG_LCD_BUFFER_SIZE, LV_DISP_RENDER_MODE_PARTIAL);
#else
    lv_display_set_buffers(s_display_handle, s_lvgl_buffer_1, NULL, CONFIG_LCD_BUFFER_SIZE, LV_DISP_RENDER_MODE_PARTIAL);
#endif

    return ESP_OK;
}

void lcd_get_display(lv_display_t **display)
{
    if (display == NULL) {
        return;
    }
    *display = s_display_handle;
}

/********************************************
 *     Private Function Implementations     *
 ********************************************/

/**
 * @brief Callback function for post-transaction handling
 * This function is called after a SPI transaction is completed. It signals to the LVGL library that the flush operation is complete, allowing it to proceed with the next rendering task.
 * @param trans Pointer to the SPI transaction that has just completed.
 */
void lcd_post_transaction_cb(spi_transaction_t *trans)
{
    if (trans == &s_lcd_transaction) {
        // Signal to LVGL that the flush operation is complete
        lv_display_flush_ready(s_display_handle);
    }
}

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
    if (xSemaphoreTake(s_lcd_transaction_mutex, portMAX_DELAY) != pdTRUE) {

        while (1)
            ;
    }

    s_lcd_transaction.cmd       = 0x02;                              // OP Code for Write Command
    s_lcd_transaction.addr      = 0x000000 | ((cmd[0] & 0xFF) << 8); // Adress  is 0x00XX00 where XX is the command byte
    s_lcd_transaction.length    = param_size * 8;                    // Length of the parameter data in bits
    s_lcd_transaction.tx_buffer = (void *)param;                     // Pointer to the parameter data

    esp_err_t ret             = spi_device_polling_transmit(s_lcd_spi_device, &s_lcd_transaction);
    if (ret != ESP_OK) {
        while (1)
            ;
    }

    if (xSemaphoreGive(s_lcd_transaction_mutex) != pdTRUE) {
        while (1)
            ;
    }
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
    if (xSemaphoreTake(s_lcd_transaction_mutex, portMAX_DELAY) != pdTRUE) {

        while (1)
            ;
    }

    s_lcd_transaction.cmd       = 0x02;                              // OP Code for Write Command
    s_lcd_transaction.addr      = 0x000000 | ((cmd[0] & 0xFF) << 8); // Adress  is 0x00XX00 where XX is the command byte
    s_lcd_transaction.length    = param_size * 8;                    // Length of the parameter data in bits
    s_lcd_transaction.tx_buffer = (void *)param;                     // Pointer to the parameter data

    esp_err_t ret             = spi_device_queue_trans(s_lcd_spi_device, &s_lcd_transaction, portMAX_DELAY);
    if (ret != ESP_OK) {
        while (1)
            ;
    }

    if (xSemaphoreGive(s_lcd_transaction_mutex) != pdTRUE) {
        while (1)
            ;
    }
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
    while (1) {
        uint32_t time_till_next = lv_timer_handler();

        /*If there is nothing to do now, check again a little bit later.*/
        if (time_till_next == LV_NO_TIMER_READY) {
            time_till_next = LV_DEF_REFR_PERIOD; /*33 ms by default in lv_conf.h*/
        }
        lv_sleep_ms(time_till_next); /*Sleep the thread*/
    }
}
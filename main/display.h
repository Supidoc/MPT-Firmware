/************************************************************
 * @file    display.h
 * @brief   Filedescription
 * @author  dg
 * @date    01 Oct 2026
 ************************************************************/

#pragma once

/********************
 *     Includes    *
 ********************/
#include "lvgl.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/***********************************
 *     Public Macros / Defines     *
 ***********************************/

/***************************
 *     Public Typedefs     *
 ***************************/

/****************************
 *     Public Variables     *
 ****************************/

/**************************************
 *     Public Function Prototypes    *
 **************************************/

/**
 * @brief Initialize the LCD display
 * This function initializes the LCD display, adds the SPI device to the bus, and configures the LVGL library for rendering. It also creates a FreeRTOS task to handle LVGL timers.
 * @return ESP_OK on success, or an error code on failure.
 */
esp_err_t lcd_init(void);

/**
 * @brief Get the LVGL display handle
 * This function retrieves the handle to the LVGL display structure. It is used to access the display for rendering operations.
 * @param pDisp Pointer to a pointer that will receive the display handle.
 * @return true if the display handle was successfully retrieved, false if the pointer is NULL or the display handle is not initialized.
 */
bool lcd_get_display(lv_display_t **pDisp);

#ifdef __cplusplus
}
#endif
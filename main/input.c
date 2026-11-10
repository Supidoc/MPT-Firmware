/************************************************************
 * @file    input.c
 * @brief   Filedescription
 * @author  dg
 * @date    09 Nov 2026
 ************************************************************/
#ifndef LVGL_LIVE_PREVIEW

/********************
 *     Includes    *
 ********************/
#include "input.h"
#include "driver/gpio.h"
#include "esp_check.h"

/************************************
 *     Private Macros / Defines    *
 ************************************/

/**
 * @brief Tag for logging
 */
#define TAG "input"

/***************************
 *     Private Typedefs     *
 ***************************/

/*****************************************
 *     Private Function Declarations     *
 *****************************************/

/****************************
 *     Public Variables     *
 ****************************/

/*****************************
 *     Private Variables     *
 *****************************/

/*******************************************
 *     Public Function Implementations     *
 *******************************************/

esp_err_t input_init(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << CONFIG_INPUT_BUTTON_UP_GPIO)
        | (1ULL << CONFIG_INPUT_BUTTON_DOWN_GPIO)
        | (1ULL << CONFIG_INPUT_BUTTON_LEFT_GPIO)
        | (1ULL << CONFIG_INPUT_BUTTON_RIGHT_GPIO)
        | (1ULL << CONFIG_INPUT_BUTTON_ENTER_GPIO)
        | (1ULL << CONFIG_INPUT_BUTTON_ESCAPE_GPIO)
        | (1ULL << CONFIG_INPUT_BUTTON_PREV_GPIO)
        | (1ULL << CONFIG_INPUT_BUTTON_NEXT_GPIO),
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };

    ESP_RETURN_ON_ERROR(gpio_config(&io_conf), TAG, "Failed to configure input pins");
    return ESP_OK;
}

void input_get_button_states(bool *up, bool *down, bool *left, bool *right, bool *enter, bool *escape, bool *prev, bool *next)
{
    if (up) {
        *up = gpio_get_level(CONFIG_INPUT_BUTTON_UP_GPIO) == 0;
    }
    if (down) {
        *down = gpio_get_level(CONFIG_INPUT_BUTTON_DOWN_GPIO) == 0;
    }
    if (left) {
        *left = gpio_get_level(CONFIG_INPUT_BUTTON_LEFT_GPIO) == 0;
    }
    if (right) {
        *right = gpio_get_level(CONFIG_INPUT_BUTTON_RIGHT_GPIO) == 0;
    }
    if (enter) {
        *enter = gpio_get_level(CONFIG_INPUT_BUTTON_ENTER_GPIO) == 0;
    }
    if (escape) {
        *escape = gpio_get_level(CONFIG_INPUT_BUTTON_ESCAPE_GPIO) == 0;
    }
    if (prev) {
        *prev = gpio_get_level(CONFIG_INPUT_BUTTON_PREV_GPIO) == 0;
    }
    if (next) {
        *next = gpio_get_level(CONFIG_INPUT_BUTTON_NEXT_GPIO) == 0;
    }
}

/********************************************
 *     Private Function Implementations     *
 ********************************************/
#endif
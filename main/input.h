/************************************************************
 * @file    input.h
 * @brief   Filedescription
 * @author  dg
 * @date    09 Nov 2026
 ************************************************************/

#pragma once

/********************
 *     Includes    *
 ********************/
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
esp_err_t input_init(void);

void input_get_button_states(bool *up, bool *down, bool *left, bool *right, bool *enter, bool *escape, bool *prev, bool *next);

#ifdef __cplusplus
}
#endif
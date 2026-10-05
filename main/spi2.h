/************************************************************
 * @file    spi2.h
 * @brief   Filedescription
 * @author  dg
 * @date    01 Oct 2026
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

esp_err_t spi2_init(void);

#ifdef __cplusplus
}
#endif
/************************************************************
 * @file    spi2.c
 * @brief   Filedescription
 * @author  dg
 * @date    01 Oct 2026
 ************************************************************/

/********************
 *     Includes    *
 ********************/
#include "spi2.h"
#include "sdkconfig.h"
#include "driver/spi_common.h"
#include "driver/spi_master.h"

/************************************
 *     Private Macros / Defines    *
 ************************************/

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

esp_err_t spi2_init(void)
{

    spi_bus_config_t buscfg = {
        .data0_io_num = CONFIG_SPI2_DATA0_GPIO,
        .data1_io_num = CONFIG_SPI2_DATA1_GPIO,
        .data2_io_num = CONFIG_SPI2_DATA2_GPIO,
        .data3_io_num = CONFIG_SPI2_DATA3_GPIO,
        .sclk_io_num = CONFIG_SPI2_SCLK_GPIO
    };

    esp_err_t ret = spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO);

    if (ret != ESP_OK) {
        return ret;
    }

    return ESP_OK;
}

/********************************************
 *     Private Function Implementations     *
 ********************************************/

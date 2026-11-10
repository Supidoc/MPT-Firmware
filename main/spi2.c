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
#define LCD_SPI_MAX_TRANSFER_SIZE 4092U

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
        .mosi_io_num = CONFIG_SPI2_MOSI_GPIO,
        .miso_io_num = -1,
        .sclk_io_num = CONFIG_SPI2_SCLK_GPIO,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = CONFIG_SPI2_MAX_TRANSFER_SIZE,
        .flags = SPICOMMON_BUSFLAG_MASTER |  SPICOMMON_BUSFLAG_SCLK | SPICOMMON_BUSFLAG_MOSI,
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

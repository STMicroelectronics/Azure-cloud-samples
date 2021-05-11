/**
  ******************************************************************************
  * @file    es_wifi_io.h
  * @author  MCD Application Team
  * @brief   This file contains the functions prototypes for es_wifi IO operations.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2017 STMicroelectronics International N.V. 
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */

#ifndef WIFI_IO_H
#define WIFI_IO_H

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32l4xx_hal.h"

/* Exported constants --------------------------------------------------------*/

#define WIFI_SPI_INSTANCE           SPI1
#define WIFI_SPI_CLK_ENABLE()       __SPI1_CLK_ENABLE()
#define WIFI_SPI_IRQn               SPI1_IRQn
#define WIFI_SPI_IRQHandler         SPI1_IRQHandler

   
// SPI Configuration
#define WIFI_SPI_MODE               SPI_MODE_MASTER
#define WIFI_SPI_DIRECTION          SPI_DIRECTION_2LINES
#define WIFI_SPI_DATASIZE           SPI_DATASIZE_16BIT
#define WIFI_SPI_CLKPOLARITY        SPI_POLARITY_LOW
#define WIFI_SPI_CLKPHASE           SPI_PHASE_1EDGE
#define WIFI_SPI_NSS                SPI_NSS_SOFT
#define WIFI_SPI_FIRSTBIT           SPI_FIRSTBIT_MSB
#define WIFI_SPI_TIMODE             SPI_TIMODE_DISABLED
#define WIFI_SPI_CRCPOLYNOMIAL      0
#define WIFI_SPI_BAUDRATEPRESCALER  SPI_BAUDRATEPRESCALER_8
#define WIFI_SPI_CRCCALCULATION     SPI_CRCCALCULATION_DISABLED

//  Reset Pin:
#define WIFI_SPI_RESET_PIN          GPIO_PIN_6
#define WIFI_SPI_RESET_PORT         GPIOC
#define WIFI_SPI_RESET_CLK_ENABLE() __GPIOC_CLK_ENABLE();
   
//  Data Ready Pin: 
#define WIFI_DATA_READY_PIN           GPIO_PIN_11
#define WIFI_DATA_READY_PORT          GPIOE
#define WIFI_DATA_READY_CLK_ENABLE()  __GPIOE_CLK_ENABLE()
   
// SCLK:
#define WIFI_SPI_SCLK_PIN           GPIO_PIN_2
#define WIFI_SPI_SCLK_PORT          GPIOG
#define WIFI_SPI_SCLK_CLK_ENABLE()  __GPIOG_CLK_ENABLE()

// MISO (Master Input Slave Output):
#define WIFI_SPI_MISO_PIN           GPIO_PIN_3
#define WIFI_SPI_MISO_PORT          GPIOG
#define WIFI_SPI_MISO_CLK_ENABLE()  __GPIOG_CLK_ENABLE()

// MOSI (Master Output Slave Input):
#define WIFI_SPI_MOSI_PIN           GPIO_PIN_4
#define WIFI_SPI_MOSI_PORT          GPIOG
#define WIFI_SPI_MOSI_CLK_ENABLE()  __GPIOG_CLK_ENABLE()

// NSS/CSN/CS: PA.1
#define WIFI_SPI_CS_PIN             GPIO_PIN_3
#define WIFI_SPI_CS_PORT            GPIOF
#define WIFI_SPI_CS_CLK_ENABLE()    __GPIOF_CLK_ENABLE()

// BOOT
#define WIFI_BOOT_PIN             GPIO_PIN_12
#define WIFI_BOOT_PORT            GPIOF
#define WIFI_BOOT_CLK_ENABLE()    __GPIOF_CLK_ENABLE()

/* Exported macro ------------------------------------------------------------*/
#define WIFI_RESET_MODULE()                do{\
                                            HAL_GPIO_WritePin(WIFI_SPI_RESET_PORT, WIFI_SPI_RESET_PIN, GPIO_PIN_RESET);\
                                            HAL_Delay(10);\
                                            HAL_GPIO_WritePin(WIFI_SPI_RESET_PORT, WIFI_SPI_RESET_PIN, GPIO_PIN_SET);\
                                            HAL_Delay(500);\
                                             }while(0);


#define WIFI_ENABLE_NSS()                  do{ \
                                             HAL_GPIO_WritePin( WIFI_SPI_CS_PORT, WIFI_SPI_CS_PIN, GPIO_PIN_RESET );\
                                             }while(0);

#define WIFI_DISABLE_NSS()                 do{ \
                                             HAL_GPIO_WritePin( WIFI_SPI_CS_PORT, WIFI_SPI_CS_PIN, GPIO_PIN_SET );\
                                             }while(0);

#define WIFI_IS_CMDDATA_READY()            (HAL_GPIO_ReadPin(WIFI_DATA_READY_PORT, WIFI_DATA_READY_PIN) == GPIO_PIN_SET)

/* Exported functions ------------------------------------------------------- */ 
void    SPI_WIFI_MspInit(SPI_HandleTypeDef* hspi);
int8_t  SPI_WIFI_DeInit(void);
int8_t  SPI_WIFI_Init(uint16_t mode);
int8_t  SPI_WIFI_ResetModule(void);
int16_t SPI_WIFI_ReceiveData(uint8_t *pData, uint16_t len, uint32_t timeout);
int16_t SPI_WIFI_SendData( uint8_t *pData, uint16_t len, uint32_t timeout);
void    SPI_WIFI_Delay(uint32_t Delay);
void    SPI_WIFI_ISR(void);

#ifdef __cplusplus
}
#endif

#endif /* WIFI_IO_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

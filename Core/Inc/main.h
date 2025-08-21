/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.h
 * @brief          : Header for main.c file.
 *                   This file contains the common defines of the application.
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2025 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C"
{
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"

    /* Private includes ----------------------------------------------------------*/
    /* USER CODE BEGIN Includes */

    /* USER CODE END Includes */

    /* Exported types ------------------------------------------------------------*/
    /* USER CODE BEGIN ET */
// U-Boot头部定义
#define UBOOT_MAGIC 0x27051956
#define UBOOT_HEADER_SIZE 256
    // U-Boot头部结构体
    typedef struct image_header
    {
        uint32_t ih_magic;   /* Image Header Magic Number  */
        uint32_t ih_hcrc;    /* Image Header CRC Checksum  */
        uint32_t ih_time;    /* Image Creation Timestamp   */
        uint32_t ih_size;    /* Image Data Size            */
        uint32_t ih_load;    /* Data Load Address          */
        uint32_t ih_ep;      /* Entry Point Address        */
        uint32_t ih_dcrc;    /* Image Data CRC Checksum    */
        uint8_t ih_os;       /* Operating System           */
        uint8_t ih_arch;     /* CPU architecture           */
        uint8_t ih_type;     /* Image Type                 */
        uint8_t ih_comp;     /* Compression Type           */
        uint8_t ih_name[32]; /* Image Name                 */
    } image_header_t;
    /* USER CODE END ET */

    /* Exported constants --------------------------------------------------------*/
    /* USER CODE BEGIN EC */

    /* USER CODE END EC */

    /* Exported macro ------------------------------------------------------------*/
    /* USER CODE BEGIN EM */

    /* USER CODE END EM */

    /* Exported functions prototypes ---------------------------------------------*/
    void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define LED_Pin GPIO_PIN_2
#define LED_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */
#define APP_ADDRESS 0x08040000U // APP 起始地址
#define UART_TIMEOUT 1000       // 上位机等待时间(ms)
    /* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

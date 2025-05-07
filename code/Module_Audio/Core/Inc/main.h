/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.h
 * @brief          : Header for main.c file.
 *                   This file contains the common defines of the application.
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2024 STMicroelectronics.
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
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32g0xx_hal.h"

#include "stm32g0xx_ll_i2c.h"
#include "stm32g0xx_ll_iwdg.h"
#include "stm32g0xx_ll_tim.h"
#include "stm32g0xx_ll_bus.h"
#include "stm32g0xx_ll_cortex.h"
#include "stm32g0xx_ll_rcc.h"
#include "stm32g0xx_ll_system.h"
#include "stm32g0xx_ll_utils.h"
#include "stm32g0xx_ll_pwr.h"
#include "stm32g0xx_ll_gpio.h"
#include "stm32g0xx_ll_dma.h"

#include "stm32g0xx_ll_exti.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

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
#define LIN_MIC_PU_EN_Pin       GPIO_PIN_7
#define LIN_MIC_PU_EN_GPIO_Port GPIOB
#define HP_DET_Pin              GPIO_PIN_1
#define HP_DET_GPIO_Port        GPIOA
#define HP_DET_EXTI_IRQn        EXTI0_1_IRQn
#define HP_MODE_SET_Pin         GPIO_PIN_2
#define HP_MODE_SET_GPIO_Port   GPIOA

/* USER CODE BEGIN Private defines */

#define LIN_MIC_PC_EN_ADDR (0x00)
#define HP_MODE_ADDR       (0x10)
#define HP_DET_ADDR        (0x20)
#define RGB_LIGHT_ADDR     (0x30)
#define RGB1_STATUS_ADDR   (0x40)
#define RGB2_STATUS_ADDR   (0x43)
#define RGB3_STATUS_ADDR   (0x46)
#define FLASH_WRITE_BACK   (0xF0)
#define IAP_UPDATE_ADDR    (0xFD)
#define SW_VER_REG_ADDR    (0xFE)
#define I2C_ADDR_REG_ADDR  (0xFF)

#define FIRMWARE_VERSION    (1)
#define I2C1_ADDR_BASE      (0x33)
#define RGB_REG_LIGHT_BASE  (10)
#define APPLICATION_ADDRESS (0x08001800)

typedef enum {
    RGB_LED_1 = 0x00,  // RGB LED 1
    RGB_LED_2 = 0x01,  // RGB LED 2
    RGB_LED_3 = 0x02   // RGB LED 3
} rgb_led_t;           // RGB LED type

typedef enum {
    HEADPHONE_UNPLUG    = 0x00,  // Headphone unplugged
    HEADPHONE_INSERTION = 0x01   // Headphone inserted
} headphone_detection_t;         // Headphone insertion status

typedef enum {
    MICROPHONE_OFF = 0x00,  // Microphone off
    MICROPHONE_ON  = 0x01   // Microphone on
} mic_t;                    // Microphone status

typedef enum {
    NATIONAL_STANDARD = 0x00,  // Chinese national standard
    AMERICA_STANDARD  = 0x01   // American standard
} headphone_mode_t;            // Headphone mode type

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

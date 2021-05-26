/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2021 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
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
#include "stm32f7xx_hal.h"

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
#define LED_YELLOW_Pin GPIO_PIN_2
#define LED_YELLOW_GPIO_Port GPIOE
#define LED_RED_Pin GPIO_PIN_3
#define LED_RED_GPIO_Port GPIOE
#define Current_Meas_Pin GPIO_PIN_0
#define Current_Meas_GPIO_Port GPIOA
#define DRB_USART1_DIR_Pin GPIO_PIN_3
#define DRB_USART1_DIR_GPIO_Port GPIOA
#define IN01_Pin GPIO_PIN_6
#define IN01_GPIO_Port GPIOA
#define IN02_Pin GPIO_PIN_8
#define IN02_GPIO_Port GPIOA
#define DRB_USART1_TX_Pin GPIO_PIN_9
#define DRB_USART1_TX_GPIO_Port GPIOA
#define DRB_USART1_RX_Pin GPIO_PIN_10
#define DRB_USART1_RX_GPIO_Port GPIOA
#define IN03_Pin GPIO_PIN_11
#define IN03_GPIO_Port GPIOA
/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

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
#include "eeprom.h"
/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */
extern struct ee_storage_s eemqtttopic;
extern struct ee_storage_s eemqtthost;
extern struct ee_storage_s eeblindmovingtime;
extern struct ee_storage_s eecurrentthreshold;
extern uint32_t blindmovingtime[];
extern uint16_t currentthreshold;
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
void setReset(void);
/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define MQTT_REQ_MAX_IN_FLIGHT 70
#define LWIP_NUM_NETIF_CLIENT_DATA 1
#define MQTT_OUTPUT_RINGBUF_SIZE 1024
#define LWIP_DEBUG 1
#define LED_YELLOW_Pin GPIO_PIN_2
#define LED_YELLOW_GPIO_Port GPIOE
#define LED_RED_Pin GPIO_PIN_3
#define LED_RED_GPIO_Port GPIOE
#define Current_Meas_Pin GPIO_PIN_0
#define Current_Meas_GPIO_Port GPIOA
#define DRB_USART1_DIR_Pin GPIO_PIN_3
#define DRB_USART1_DIR_GPIO_Port GPIOA
#define IN01_Pin GPIO_PIN_7
#define IN01_GPIO_Port GPIOE
#define IN02_Pin GPIO_PIN_8
#define IN02_GPIO_Port GPIOE
#define IN03_Pin GPIO_PIN_9
#define IN03_GPIO_Port GPIOE
#define IN04_Pin GPIO_PIN_10
#define IN04_GPIO_Port GPIOE
#define IN05_Pin GPIO_PIN_11
#define IN05_GPIO_Port GPIOE
#define IN06_Pin GPIO_PIN_12
#define IN06_GPIO_Port GPIOE
#define IN07_Pin GPIO_PIN_13
#define IN07_GPIO_Port GPIOE
#define IN08_Pin GPIO_PIN_14
#define IN08_GPIO_Port GPIOE
#define IN09_Pin GPIO_PIN_15
#define IN09_GPIO_Port GPIOE
#define IN10_Pin GPIO_PIN_10
#define IN10_GPIO_Port GPIOB
#define IN11_Pin GPIO_PIN_12
#define IN11_GPIO_Port GPIOD
#define IN12_Pin GPIO_PIN_13
#define IN12_GPIO_Port GPIOD
#define IN13_Pin GPIO_PIN_14
#define IN13_GPIO_Port GPIOD
#define IN14_Pin GPIO_PIN_15
#define IN14_GPIO_Port GPIOD
#define IN15_Pin GPIO_PIN_6
#define IN15_GPIO_Port GPIOC
#define IN16_Pin GPIO_PIN_7
#define IN16_GPIO_Port GPIOC
#define IN17_Pin GPIO_PIN_8
#define IN17_GPIO_Port GPIOC
#define IN18_Pin GPIO_PIN_9
#define IN18_GPIO_Port GPIOC
#define DRB_USART1_TX_Pin GPIO_PIN_9
#define DRB_USART1_TX_GPIO_Port GPIOA
#define DRB_USART1_RX_Pin GPIO_PIN_10
#define DRB_USART1_RX_GPIO_Port GPIOA
#define OUT01DOWN_Pin GPIO_PIN_11
#define OUT01DOWN_GPIO_Port GPIOC
#define OUT01UP_Pin GPIO_PIN_12
#define OUT01UP_GPIO_Port GPIOC
#define OUT02DOWN_Pin GPIO_PIN_0
#define OUT02DOWN_GPIO_Port GPIOD
#define OUT02UP_Pin GPIO_PIN_1
#define OUT02UP_GPIO_Port GPIOD
#define OUT03DOWN_Pin GPIO_PIN_2
#define OUT03DOWN_GPIO_Port GPIOD
#define OUT03UP_Pin GPIO_PIN_3
#define OUT03UP_GPIO_Port GPIOD
#define OUT04DOWN_Pin GPIO_PIN_4
#define OUT04DOWN_GPIO_Port GPIOD
#define OUT04UP_Pin GPIO_PIN_5
#define OUT04UP_GPIO_Port GPIOD
#define OUT05DOWN_Pin GPIO_PIN_6
#define OUT05DOWN_GPIO_Port GPIOD
#define OUT05UP_Pin GPIO_PIN_7
#define OUT05UP_GPIO_Port GPIOD
#define OUT06DOWN_Pin GPIO_PIN_4
#define OUT06DOWN_GPIO_Port GPIOB
#define OUT06UP_Pin GPIO_PIN_5
#define OUT06UP_GPIO_Port GPIOB
#define OUT07DOWN_Pin GPIO_PIN_6
#define OUT07DOWN_GPIO_Port GPIOB
#define OUT07UP_Pin GPIO_PIN_7
#define OUT07UP_GPIO_Port GPIOB
#define OUT08DOWN_Pin GPIO_PIN_8
#define OUT08DOWN_GPIO_Port GPIOB
#define OUT08UP_Pin GPIO_PIN_9
#define OUT08UP_GPIO_Port GPIOB
/* USER CODE BEGIN Private defines */

#define ADDR_FLASH_SECTOR_0     ((uint32_t)0x08000000) /* Base address of Sector 0, 32 Kbytes */
#define ADDR_FLASH_SECTOR_1     ((uint32_t)0x08008000) /* Base address of Sector 1, 32 Kbytes */
#define ADDR_FLASH_SECTOR_2     ((uint32_t)0x08010000) /* Base address of Sector 2, 32 Kbytes */
#define ADDR_FLASH_SECTOR_3     ((uint32_t)0x08018000) /* Base address of Sector 3, 32 Kbytes */
#define ADDR_FLASH_SECTOR_4     ((uint32_t)0x08020000) /* Base address of Sector 4, 128 Kbytes */
#define ADDR_FLASH_SECTOR_5     ((uint32_t)0x08040000) /* Base address of Sector 5, 256 Kbytes */
#define ADDR_FLASH_SECTOR_6     ((uint32_t)0x08080000) /* Base address of Sector 6, 256 Kbytes */
#define ADDR_FLASH_SECTOR_7     ((uint32_t)0x080C0000) /* Base address of Sector 7, 256 Kbytes */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

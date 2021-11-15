/**
  ******************************************************************************
  * File Name          : main.hpp
  * Description        : This file contains the common defines of the application
  ******************************************************************************
  * This notice applies to any and all portions of this file
  * that are not between comment pairs USER CODE BEGIN and
  * USER CODE END. Other portions of this file, whether 
  * inserted by the user or by software development tools
  * are owned by their respective copyright owners.
  *
  * Copyright (c) 2017 STMicroelectronics International N.V. 
  * All rights reserved.
  *
  * Redistribution and use in source and binary forms, with or without 
  * modification, are permitted, provided that the following conditions are met:
  *
  * 1. Redistribution of source code must retain the above copyright notice, 
  *    this list of conditions and the following disclaimer.
  * 2. Redistributions in binary form must reproduce the above copyright notice,
  *    this list of conditions and the following disclaimer in the documentation
  *    and/or other materials provided with the distribution.
  * 3. Neither the name of STMicroelectronics nor the names of other 
  *    contributors to this software may be used to endorse or promote products 
  *    derived from this software without specific written permission.
  * 4. This software, including modifications and/or derivative works of this 
  *    software, must execute solely and exclusively on microcontroller or
  *    microprocessor devices manufactured by or for STMicroelectronics.
  * 5. Redistribution and use of this software other than as permitted under 
  *    this license is void and will automatically terminate your rights under 
  *    this license. 
  *
  * THIS SOFTWARE IS PROVIDED BY STMICROELECTRONICS AND CONTRIBUTORS "AS IS" 
  * AND ANY EXPRESS, IMPLIED OR STATUTORY WARRANTIES, INCLUDING, BUT NOT 
  * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A 
  * PARTICULAR PURPOSE AND NON-INFRINGEMENT OF THIRD PARTY INTELLECTUAL PROPERTY
  * RIGHTS ARE DISCLAIMED TO THE FULLEST EXTENT PERMITTED BY LAW. IN NO EVENT 
  * SHALL STMICROELECTRONICS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
  * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
  * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, 
  * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF 
  * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING 
  * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
  * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H
  /* Includes ------------------------------------------------------------------*/

/* Includes ------------------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private define ------------------------------------------------------------*/

#define BM77_RST_Pin GPIO_PIN_13
#define BM77_RST_GPIO_Port GPIOC
#define BM77_BT1_Pin GPIO_PIN_0
#define BM77_BT1_GPIO_Port GPIOC
#define CHECK_VDD_Pin GPIO_PIN_1
#define CHECK_VDD_GPIO_Port GPIOC
#define BM77_E2PR_Pin GPIO_PIN_2
#define BM77_E2PR_GPIO_Port GPIOC
#define SPI_CS_Pin GPIO_PIN_4
#define SPI_CS_GPIO_Port GPIOA
#define BB0906_RST_Pin GPIO_PIN_0
#define BB0906_RST_GPIO_Port GPIOB
#define OUT_WATCHDOG_Pin GPIO_PIN_1
#define OUT_WATCHDOG_GPIO_Port GPIOB
#define GSM_POWER_Pin GPIO_PIN_2
#define GSM_POWER_GPIO_Port GPIOB
#define LED_RUN_Pin GPIO_PIN_10
#define LED_RUN_GPIO_Port GPIOB
#define LED_PASS_Pin GPIO_PIN_11
#define LED_PASS_GPIO_Port GPIOB
#define SPI2_CS_Pin GPIO_PIN_12
#define SPI2_CS_GPIO_Port GPIOB
#define IRQI_Pin GPIO_PIN_6
#define IRQI_GPIO_Port GPIOC
#define IRQO_Pin GPIO_PIN_7
#define IRQO_GPIO_Port GPIOC
#define BEEP_Pin GPIO_PIN_8
#define BEEP_GPIO_Port GPIOC
#define KEY_CONFIG_Pin GPIO_PIN_9
#define KEY_CONFIG_GPIO_Port GPIOC
#define KEY_DOOR_Pin GPIO_PIN_8
#define KEY_DOOR_GPIO_Port GPIOA
#define KEY_DOOR_EXTI_IRQn EXTI9_5_IRQn
#define KEY_SYS_Pin GPIO_PIN_11
#define KEY_SYS_GPIO_Port GPIOA
#define KEY_SYS_EXTI_IRQn EXTI15_10_IRQn
#define RELAY_Pin GPIO_PIN_12
#define RELAY_GPIO_Port GPIOA
#define ETH_INT_Pin GPIO_PIN_15
#define ETH_INT_GPIO_Port GPIOA
#define ETH_INT_EXTI_IRQn EXTI15_10_IRQn
#define ETN_RST_Pin GPIO_PIN_10
#define ETN_RST_GPIO_Port GPIOC
#define RS485_EN_Pin GPIO_PIN_11
#define RS485_EN_GPIO_Port GPIOC
#define SPI3_CS_Pin GPIO_PIN_6
#define SPI3_CS_GPIO_Port GPIOB
#define TH_INT_Pin GPIO_PIN_7
#define TH_INT_GPIO_Port GPIOB
#define TH_INT_EXTI_IRQn EXTI9_5_IRQn
#define I2C1_SCL_Pin GPIO_PIN_8
#define I2C1_SCL_GPIO_Port GPIOB
#define I2C1_SDA_Pin GPIO_PIN_9
#define I2C1_SDA_GPIO_Port GPIOB
#define NET_CHOOSE_PIN1_Pin     GPIO_PIN_3
#define NET_CHOOSE_PIN1_Port    GPIOC
#define NET_CHOOSE_PIN2_Pin     GPIO_PIN_13
#define NET_CHOOSE_PIN2_Port    GPIOC

/* ########################## Assert Selection ############################## */
/**
  * @brief Uncomment the line below to expanse the "assert_param" macro in the 
  *        HAL drivers code
  */
/* #define USE_FULL_ASSERT    1U */

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
 extern "C" {
#endif
void _Error_Handler(char *, int);

#define Error_Handler() _Error_Handler(__FILE__, __LINE__)
#ifdef __cplusplus
}
#endif

/**
  * @}
  */ 

/**
  * @}
*/ 

#endif /* __MAIN_H */
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

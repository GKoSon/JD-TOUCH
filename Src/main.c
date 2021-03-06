/**
  ******************************************************************************
  * File Name          : main.c
  * Description        : Main program body
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

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "stm32l4xx_hal.h"
#include "cmsis_os.h"
#include "adc.h"
#include "iwdg.h"
#include "rtc.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* USER CODE BEGIN Includes */
#include "bsp.h"
#include "components_ins.h"
#include "config.h"
#include <cm_backtrace.h>

/* USER CODE END Includes */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
/* Private variables ---------------------------------------------------------*/
#define TESTOTA  0
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void MX_FREERTOS_Init(void);
static void MX_NVIC_Init(void);

/* USER CODE BEGIN PFP */
/* Private function prototypes -----------------------------------------------*/
/******************************测试1 交换顺序************************/
uint16_t  exchangeBytes(uint16_t value)
{

    uint16_t tmp_value = 0;
    uint8_t *index_1, *index_2;

    index_1 = (uint8_t *)&tmp_value;
    index_2 = (uint8_t *)&value;

    *index_1     = *(index_2+1);
    *(index_1+1) = *index_2;

    return tmp_value;

}

void ExchangeBytes(void)
{
    #define uint16_t_CHANGE_BIT_MODE(x)     (((x>>8)&0x00ff)| ((x << 8)&0xff00))

    uint16_t flag = 0XABCD;

    flag = exchangeBytes( flag);
       
    printf("exchangeBytes:0X%04X uint16_t_CHANGE_BIT_MODE:0X%04X\r\n",flag,uint16_t_CHANGE_BIT_MODE(flag));   
}
/******************************测试2 什么叫做CRC8************************/
/*
1---0和任何数疑惑都等于任何数
2---A和A疑惑等于0[A可以是string]

*/

void test_crc8(void)
{
	uint8_t A=0XAB,B=0XAB;

	printf("A^B=%02X\r\n",A^B);

	char AA[21]={"110101001001003102001"};
	char BB[21]={"110101001001003102001"};
	uint8_t AAACRC=mycrc8((uint8_t*)&AA,21);
	uint8_t BBBCRC=mycrc8((uint8_t*)&BB,21);
	printf("AAACRC=%c\r\n",AAACRC);
	printf("BBBCRC=%c\r\n",BBBCRC);
	printf("AAACRC^BBBCRC=%d\r\n",AAACRC^BBBCRC);
}


void test_my(void)
{
	char A[6]={"123456"};
	memcpy_down(A,A,strlen(A));
	log_arry(DEBUG,"A" ,(uint8_t *)A , 6);

	char B[3]={0xAB,0XCD,0XEF};
	memcpy_up(A,B,3);
	printf("[%s]\r\n",A);


	int C=654321;//-->0X65 0X43 0X21
	sprintf(A,"%d",C);
	printf("[%.6s]\r\n",A);
	memcpy_down(A,A,strlen(A));
	log_arry(DEBUG,"A" ,(uint8_t *)A , 6);
}


/* USER CODE END PFP */

/* USER CODE BEGIN 0 */
extern char*  CreatJsCustNo( void );
extern int create_js(void);
/* USER CODE END 0 */
 int main(void)
{
    /* USER CODE BEGIN 1 */
    cm_backtrace_init("seed", HARDWARE_VERSION, SOFTWARE_VERSION);
    /* USER CODE END 1 */
    /* MCU Configuration----------------------------------------------------------*/
    /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
    HAL_Init();

    /* USER CODE BEGIN Init */
    /* USER CODE END Init */

    /* Configure the system clock */
    
    SystemClock_Config();

    /* USER CODE BEGIN SysInit */

    /* USER CODE END SysInit */

    /* Initialize all configured peripherals */

    MX_GPIO_Init();
    MX_RTC_Init();
    MX_SPI1_Init();
    MX_UART4_Init();
    MX_UART5_Init();
    MX_USART1_UART_Init();
    MX_USART2_UART_Init();
    MX_USART3_UART_Init();
    MX_SPI3_Init();
    MX_ADC1_Init();
    MX_ADC2_Init();
    if(!TESTOTA)MX_IWDG_Init();
    MX_TIM7_Init();
    MX_TIM4_Init();
    /* Initialize interrupts */
    if(!TESTOTA)MX_NVIC_Init();
create_js();
    /* USER CODE BEGIN 2 */
    serial_console_init();
    //ExchangeBytes();
    //test_crc8();
    //test_my();
    /* USER CODE END 2 */
#if TESTOTA
while(1)
{
//printf("I AM FREE CODE \r\n");
sys_delay(300);
}
#endif
    /* Call init function for freertos objects (in freertos.c) */
    MX_FREERTOS_Init();

    /* Start scheduler */
    osKernelStart();

    /* We should never get here as control is now taken by the scheduler */

    /* Infinite loop */
    /* USER CODE BEGIN WHILE */
    while (1)
    {
        /* USER CODE END WHILE */

        /* USER CODE BEGIN 3 */

    }
    /* USER CODE END 3 */

}

/** System Clock Configuration
*/
void SystemClock_Config(void)
{

    RCC_OscInitTypeDef RCC_OscInitStruct;
    RCC_ClkInitTypeDef RCC_ClkInitStruct;
    RCC_PeriphCLKInitTypeDef PeriphClkInit;

    /**Configure LSE Drive Capability
    */
    __HAL_RCC_LSEDRIVE_CONFIG(RCC_LSEDRIVE_LOW);

    /**Initializes the CPU, AHB and APB busses clocks
    */

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSI|RCC_OSCILLATORTYPE_HSE
                                       |RCC_OSCILLATORTYPE_LSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.LSEState = RCC_LSE_ON;
    RCC_OscInitStruct.LSIState = RCC_LSI_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM = 1;
    RCC_OscInitStruct.PLL.PLLN = 20;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV7;
    RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
    RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        _Error_Handler(__FILE__, __LINE__);
    }

    /**Initializes the CPU, AHB and APB busses clocks
    */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                                  |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3) != HAL_OK)
    {
        _Error_Handler(__FILE__, __LINE__);
    }

    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_RTC|RCC_PERIPHCLK_USART1
                                         |RCC_PERIPHCLK_USART2|RCC_PERIPHCLK_USART3
                                         |RCC_PERIPHCLK_UART4|RCC_PERIPHCLK_UART5
                                         |RCC_PERIPHCLK_ADC;
    PeriphClkInit.Usart1ClockSelection = RCC_USART1CLKSOURCE_PCLK2;
    PeriphClkInit.Usart2ClockSelection = RCC_USART2CLKSOURCE_PCLK1;
    PeriphClkInit.Usart3ClockSelection = RCC_USART3CLKSOURCE_PCLK1;
    PeriphClkInit.Uart4ClockSelection = RCC_UART4CLKSOURCE_PCLK1;
    PeriphClkInit.Uart5ClockSelection = RCC_UART5CLKSOURCE_PCLK1;
    PeriphClkInit.AdcClockSelection = RCC_ADCCLKSOURCE_PLLSAI1;
    PeriphClkInit.RTCClockSelection = RCC_RTCCLKSOURCE_LSE;
    PeriphClkInit.PLLSAI1.PLLSAI1Source = RCC_PLLSOURCE_HSE;
    PeriphClkInit.PLLSAI1.PLLSAI1M = 1;
    PeriphClkInit.PLLSAI1.PLLSAI1N = 16;
    PeriphClkInit.PLLSAI1.PLLSAI1P = RCC_PLLP_DIV7;
    PeriphClkInit.PLLSAI1.PLLSAI1Q = RCC_PLLQ_DIV2;
    PeriphClkInit.PLLSAI1.PLLSAI1R = RCC_PLLR_DIV2;
    PeriphClkInit.PLLSAI1.PLLSAI1ClockOut = RCC_PLLSAI1_ADC1CLK;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
    {
        _Error_Handler(__FILE__, __LINE__);
    }

    /**Configure the main internal regulator output voltage
    */
    if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
    {
        _Error_Handler(__FILE__, __LINE__);
    }

    /**Configure the Systick interrupt time
    */
    HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq()/1000);

    /**Configure the Systick
    */
    HAL_SYSTICK_CLKSourceConfig(SYSTICK_CLKSOURCE_HCLK);

    /* SysTick_IRQn interrupt configuration */
    HAL_NVIC_SetPriority(SysTick_IRQn, 15, 0);
}




void MX_NVIC_SetIRQ( uint8_t status )
{
    if(status)
    {

        HAL_NVIC_EnableIRQ(EXTI0_IRQn);
        
        HAL_NVIC_EnableIRQ(EXTI1_IRQn);
        
        HAL_NVIC_EnableIRQ(EXTI2_IRQn);
        
        HAL_NVIC_EnableIRQ(EXTI3_IRQn);
        
        HAL_NVIC_EnableIRQ(EXTI4_IRQn);
        
        HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);

        HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

        HAL_NVIC_EnableIRQ(RTC_Alarm_IRQn);

        HAL_NVIC_EnableIRQ(TIM3_IRQn);
        
        SysTick_Start();
    }
    else
    {
        HAL_NVIC_DisableIRQ(EXTI0_IRQn);
        
        HAL_NVIC_DisableIRQ(EXTI1_IRQn);
        
        HAL_NVIC_DisableIRQ(EXTI2_IRQn);
        
        HAL_NVIC_DisableIRQ(EXTI3_IRQn);
        
        HAL_NVIC_DisableIRQ(EXTI4_IRQn);
        
        HAL_NVIC_DisableIRQ(EXTI9_5_IRQn);
        
        HAL_NVIC_DisableIRQ(EXTI15_10_IRQn);
        
        HAL_NVIC_DisableIRQ(RTC_Alarm_IRQn);
        
        HAL_NVIC_DisableIRQ(TIM3_IRQn);
        
        SysTick_Stop();
    }
}


/** NVIC Configuration
*/
static void MX_NVIC_Init(void)
{
    /* EXTI9_5_IRQn interrupt configuration */
    HAL_NVIC_SetPriority(EXTI9_5_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);
    /* USART2_IRQn interrupt configuration */
    HAL_NVIC_SetPriority(USART2_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(USART2_IRQn);
    /* USART3_IRQn interrupt configuration */
    HAL_NVIC_SetPriority(USART3_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(USART3_IRQn);
    /* UART4_IRQn interrupt configuration */
    //HAL_NVIC_SetPriority(UART4_IRQn, 5, 0);
    //HAL_NVIC_EnableIRQ(UART4_IRQn);
    /* UART5_IRQn interrupt configuration */
    //HAL_NVIC_SetPriority(UART5_IRQn, 5, 0);
    //HAL_NVIC_EnableIRQ(UART5_IRQn);
    /* EXTI15_10_IRQn interrupt configuration */
    HAL_NVIC_SetPriority(EXTI15_10_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
    /* RTC_Alarm_IRQn interrupt configuration */
    HAL_NVIC_SetPriority(RTC_Alarm_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(RTC_Alarm_IRQn);
    /* USART1_IRQn interrupt configuration */
    HAL_NVIC_SetPriority(USART1_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(USART1_IRQn);
}



/* USER CODE BEGIN 4 */
void vApplicationStackOverflowHook(TaskHandle_t pxTask, char *pcTaskName)
{

    //(void)pcTaskName;
    //(void)pxTask;


    log_err("任务堆栈溢出 TASK = %s \n", pcTaskName);
    /* Run time stack overflow checking is performed if
    configCHECK_FOR_STACK_OVERFLOW is defined to 1 or 2.  This hook
    function is called if a stack overflow is detected. */
    taskDISABLE_INTERRUPTS();
    for (;; );
}
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @param  None
  * @retval None
  */
void _Error_Handler(char * file, int line)
{
    /* USER CODE BEGIN Error_Handler_Debug */
    /* User can add his own implementation to report the HAL error return state */
    while(1)
    {
    }
    /* USER CODE END Error_Handler_Debug */
}

#ifdef USE_FULL_ASSERT

/**
   * @brief Reports the name of the source file and the source line number
   * where the assert_param error has occurred.
   * @param file: pointer to the source file name
   * @param line: assert_param error line source number
   * @retval None
   */
void assert_failed(uint8_t* file, uint32_t line)
{
    /* USER CODE BEGIN 6 */
    /* User can add his own implementation to report the file name and line number,
      ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
    /* USER CODE END 6 */

}

#endif

/**
  * @}
  */

/**
  * @}
*/

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

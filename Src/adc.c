/**
  ******************************************************************************
  * File Name          : ADC.c
  * Description        : This file provides code for the configuration
  *                      of the ADC instances.
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
#include "adc.h"

#include "gpio.h"

/* USER CODE BEGIN 0 */
/* Variable used to get converted value */
__IO uint16_t uhADCxConvertedValue = 0;
__IO uint16_t uhADC2xConvertedValue = 0;
/* USER CODE END 0 */

ADC_HandleTypeDef hadc1;
ADC_HandleTypeDef hadc2;

/* ADC1 init function */
void MX_ADC1_Init(void)
{

  ADC_ChannelConfTypeDef sConfig;

    /**Common config 
    */
  hadc1.Instance = ADC1;
  hadc1.Init.ClockPrescaler        = ADC_CLOCK_ASYNC_DIV1;          /* Asynchronous clock mode, input ADC clock not divided */
  hadc1.Init.Resolution            = ADC_RESOLUTION_12B;             /* 12-bit resolution for converted data */
  hadc1.Init.DataAlign             = ADC_DATAALIGN_RIGHT;           /* Right-alignment for converted data */
  hadc1.Init.ScanConvMode          = DISABLE;                       /* Sequencer disabled (ADC conversion on only 1 channel: channel set on rank 1) */
  hadc1.Init.EOCSelection          = ADC_EOC_SINGLE_CONV;           /* EOC flag picked-up to indicate conversion end */
  hadc1.Init.LowPowerAutoWait      = DISABLE;                       /* Auto-delayed conversion feature disabled */
  hadc1.Init.ContinuousConvMode    = DISABLE;                       /* Continuous mode disabled to have only 1 conversion at each conversion trig */
  hadc1.Init.NbrOfConversion       = 1;                             /* Parameter discarded because sequencer is disabled */
  hadc1.Init.DiscontinuousConvMode = DISABLE;                       /* Parameter discarded because sequencer is disabled */
  hadc1.Init.NbrOfDiscConversion   = 1;                             /* Parameter discarded because sequencer is disabled */
  hadc1.Init.ExternalTrigConv      = ADC_SOFTWARE_START;            /* Software start to trig the 1st conversion manually, without external event */
  hadc1.Init.ExternalTrigConvEdge  = ADC_EXTERNALTRIGCONVEDGE_NONE; /* Parameter discarded because software trigger chosen */
  hadc1.Init.DMAContinuousRequests = DISABLE;                       /* DMA one-shot mode selected (not applied to this example) */
  hadc1.Init.Overrun               = ADC_OVR_DATA_OVERWRITTEN;      /* DR register is overwritten with the last conversion result in case of overrun */
  hadc1.Init.OversamplingMode      = DISABLE;                       /* No oversampling */
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

    /**Configure Regular Channel 
    */
  sConfig.Channel = ADC_CHANNEL_2;
  sConfig.Rank = 1;
  sConfig.SamplingTime = LL_ADC_SAMPLINGTIME_247CYCLES_5;
  sConfig.SingleDiff = ADC_SINGLE_ENDED;
  sConfig.OffsetNumber = ADC_OFFSET_NONE;
  sConfig.Offset = 0;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

}

void HAL_ADC_MspInit(ADC_HandleTypeDef* adcHandle)
{

	GPIO_InitTypeDef GPIO_InitStruct;
	if(adcHandle->Instance==ADC1)
	{
		/* USER CODE BEGIN ADC1_MspInit 0 */
		__HAL_RCC_ADC_CONFIG(RCC_ADCCLKSOURCE_SYSCLK);

		__HAL_RCC_GPIOC_CLK_ENABLE();

		/* USER CODE END ADC1_MspInit 0 */
		/* ADC1 clock enable */
		__HAL_RCC_ADC_CLK_ENABLE();

		/**ADC1 GPIO Configuration    
		PC1     ------> ADC1_IN2 
		*/
		GPIO_InitStruct.Pin = CHECK_VDD_Pin;
		GPIO_InitStruct.Mode = GPIO_MODE_ANALOG_ADC_CONTROL;
		GPIO_InitStruct.Pull = GPIO_NOPULL;
		HAL_GPIO_Init(CHECK_VDD_GPIO_Port, &GPIO_InitStruct);

		/* USER CODE BEGIN ADC1_MspInit 1 */
		/* Run the ADC calibration in single-ended mode */
		if (HAL_ADCEx_Calibration_Start(&hadc1, ADC_SINGLE_ENDED) != HAL_OK)
		{
			/* Calibration Error */
			Error_Handler();
		}
		/* USER CODE END ADC1_MspInit 1 */
	}

	if(adcHandle->Instance==ADC2)
	{
		/* USER CODE BEGIN ADC1_MspInit 0 */
		__HAL_RCC_ADC_CONFIG(RCC_ADCCLKSOURCE_SYSCLK);

		__HAL_RCC_GPIOC_CLK_ENABLE();

		/* USER CODE END ADC1_MspInit 0 */
		/* ADC1 clock enable */
		__HAL_RCC_ADC_CLK_ENABLE();

		/**ADC1 GPIO Configuration    
		PC0     ------> ADC2_IN1 
		*/
		GPIO_InitStruct.Pin = GPIO_PIN_0;
		GPIO_InitStruct.Mode = GPIO_MODE_ANALOG_ADC_CONTROL;
		GPIO_InitStruct.Pull = GPIO_NOPULL;
		HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

		/* USER CODE BEGIN ADC1_MspInit 1 */
		/* Run the ADC calibration in single-ended mode */
		if (HAL_ADCEx_Calibration_Start(&hadc2, ADC_SINGLE_ENDED) != HAL_OK)
		{
			/* Calibration Error */
			Error_Handler();
		}
		/* USER CODE END ADC1_MspInit 1 */
	}
}

/* ADC2 init function */
void MX_ADC2_Init(void)
{

  ADC_ChannelConfTypeDef sConfig;

    /**Common config 
    */
  hadc2.Instance = ADC2;
  hadc2.Init.ClockPrescaler        = ADC_CLOCK_ASYNC_DIV1;          /* Asynchronous clock mode, input ADC clock not divided */
  hadc2.Init.Resolution            = ADC_RESOLUTION_12B;             /* 12-bit resolution for converted data */
  hadc2.Init.DataAlign             = ADC_DATAALIGN_RIGHT;           /* Right-alignment for converted data */
  hadc2.Init.ScanConvMode          = DISABLE;                       /* Sequencer disabled (ADC conversion on only 1 channel: channel set on rank 1) */
  hadc2.Init.EOCSelection          = ADC_EOC_SINGLE_CONV;           /* EOC flag picked-up to indicate conversion end */
  hadc2.Init.LowPowerAutoWait      = DISABLE;                       /* Auto-delayed conversion feature disabled */
  hadc2.Init.ContinuousConvMode    = DISABLE;                       /* Continuous mode disabled to have only 1 conversion at each conversion trig */
  hadc2.Init.NbrOfConversion       = 1;                             /* Parameter discarded because sequencer is disabled */
  hadc2.Init.DiscontinuousConvMode = DISABLE;                       /* Parameter discarded because sequencer is disabled */
  hadc2.Init.NbrOfDiscConversion   = 1;                             /* Parameter discarded because sequencer is disabled */
  hadc2.Init.ExternalTrigConv      = ADC_SOFTWARE_START;            /* Software start to trig the 1st conversion manually, without external event */
  hadc2.Init.ExternalTrigConvEdge  = ADC_EXTERNALTRIGCONVEDGE_NONE; /* Parameter discarded because software trigger chosen */
  hadc2.Init.DMAContinuousRequests = DISABLE;                       /* DMA one-shot mode selected (not applied to this example) */
  hadc2.Init.Overrun               = ADC_OVR_DATA_OVERWRITTEN;      /* DR register is overwritten with the last conversion result in case of overrun */
  hadc2.Init.OversamplingMode      = DISABLE;                       /* No oversampling */
  if (HAL_ADC_Init(&hadc2) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

    /**Configure Regular Channel 
    */
  sConfig.Channel = ADC_CHANNEL_1;
  sConfig.Rank = 1;
  sConfig.SamplingTime = LL_ADC_SAMPLINGTIME_247CYCLES_5;
  sConfig.SingleDiff = ADC_SINGLE_ENDED;
  sConfig.OffsetNumber = ADC_OFFSET_NONE;
  sConfig.Offset = 0;
  if (HAL_ADC_ConfigChannel(&hadc2, &sConfig) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }
}


void HAL_ADC_MspDeInit(ADC_HandleTypeDef* adcHandle)
{

  if(adcHandle->Instance==ADC1)
  {
  /* USER CODE BEGIN ADC1_MspDeInit 0 */

  /* USER CODE END ADC1_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_ADC_CLK_DISABLE();
  
    /**ADC1 GPIO Configuration    
    PC1     ------> ADC1_IN2 
    */
    HAL_GPIO_DeInit(CHECK_VDD_GPIO_Port, CHECK_VDD_Pin);

  /* USER CODE BEGIN ADC1_MspDeInit 1 */

  /* USER CODE END ADC1_MspDeInit 1 */
  }
  
   if(adcHandle->Instance==ADC2)
  {
  /* USER CODE BEGIN ADC1_MspDeInit 0 */

  /* USER CODE END ADC1_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_ADC_CLK_DISABLE();
  
    /**ADC1 GPIO Configuration    
    PC1     ------> ADC1_IN2 
    */
    HAL_GPIO_DeInit(GPIOC, GPIO_PIN_0);

  /* USER CODE BEGIN ADC1_MspDeInit 1 */

  /* USER CODE END ADC1_MspDeInit 1 */
  }
} 

/* USER CODE BEGIN 1 */

/**
  * @brief  Conversion complete callback in non blocking mode
  * @param  AdcHandle : AdcHandle handle
  * @note   This example shows a simple way to report end of conversion, and
  *         you can add your own implementation.
  * @retval None
  */
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* AdcHandle)
{
  /* Get the converted value of regular channel */
  uhADCxConvertedValue = HAL_ADC_GetValue(AdcHandle);
}

#include "unit.h"
uint8_t hal_check_vdd( void )
{
    HAL_ADC_Start(&hadc1);
      
    HAL_ADC_PollForConversion(&hadc1, 50);
    
    if(HAL_IS_BIT_SET(HAL_ADC_GetState(&hadc1), HAL_ADC_STATE_REG_EOC))
    {
        /*##-3- Get the converted value of regular channel  ######################*/
        uhADCxConvertedValue = HAL_ADC_GetValue(&hadc1);
    }
    
    HAL_ADC_Stop(&hadc1);
    
 
    //if( ( uhADCxConvertedValue < 650) || ( uhADCxConvertedValue > 2100))
    {
        log(DEBUG,"read value adc1  = %d\n" , uhADCxConvertedValue);
        
        //return FALSE;
    }

      
	return( TRUE );
}

uint32_t read_module_vdd( void )
{
    HAL_ADC_Start(&hadc1);
      
    HAL_ADC_PollForConversion(&hadc1, 50);
    
    if(HAL_IS_BIT_SET(HAL_ADC_GetState(&hadc1), HAL_ADC_STATE_REG_EOC))
    {
        /*##-3- Get the converted value of regular channel  ######################*/
        uhADCxConvertedValue = HAL_ADC_GetValue(&hadc1);
    }
    
    HAL_ADC_Stop(&hadc1);
    
    log(DEBUG,"read value = %d\n" , uhADCxConvertedValue);
	
	return( uhADCxConvertedValue );
}

uint32_t read_choose_module_value( void )
{

	uint16_t readVdd[10] , max = 0  , min = 0xFFFF ,vddValue = 0;
	uint32_t total = 0 ,returnvalue = 0;

	
	for( uint8_t i = 0 ; i < 10 ; i++ )
	{
		HAL_ADC_Start(&hadc2);		  
		HAL_ADC_PollForConversion(&hadc2, 50);
		if(HAL_IS_BIT_SET(HAL_ADC_GetState(&hadc2), HAL_ADC_STATE_REG_EOC))
		{
			readVdd[i] = HAL_ADC_GetValue(&hadc2);
		}
		HAL_ADC_Stop(&hadc2);
	
	}
	
	for( uint8_t i = 0 ; i < 10 ; i++ )
	{
		max = MAX(max , readVdd[i]);
		min = MIN(min , readVdd[i]);
		
		total += readVdd[i];
	}
	
	vddValue = (total - max - min ) / 8;

	returnvalue =  ((vddValue*330)/4096)+5;
		
    log(DEBUG,"read value adc2 = %d \n" , returnvalue );
 
	return( returnvalue );
}

/* USER CODE END 1 */

/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

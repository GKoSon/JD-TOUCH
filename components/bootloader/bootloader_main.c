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
#include "unit.h"
#include "serial\serial.h"
#include "bl_spi_flash.h"
#include "syscfg.h"
#include "crc16.h"
#include "crc32.h"
#include "mqtt_ota.h"
#include "chipFlash.h"
/* USER CODE END Includes */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
typedef  void       (*pFunction)(void);
#define             APPLICATION_ADDRESS    (uint32_t)0x0800A000

/* Private variables ---------------------------------------------------------*/
SystemConfigType    cfg;

pFunction           JumpToApplication;
uint32_t            JumpAddress;
void                *console_port = NULL;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void MX_FREERTOS_Init(void);
void MX_NVIC_Init(void);

/* USER CODE BEGIN PFP */
/* Private function prototypes -----------------------------------------------*/

/* USER CODE END PFP */

extern void spi_flash_init( void );
/* USER CODE BEGIN 0 */


void console_getchar(uint8_t ch)
{

}
void serial_console_init( void )
{
    console_port = serial.open("serial1");
    
    if( console_port == NULL)
    {
        //beep.write(BEEP_ALARM);
        return ;
    }
    serial.init(console_port  , 921600 , console_getchar);
}


void timer_isr( void )
{

}


uint8_t ota_ver_file( void )
{
	uint32_t crc32 = 0xFFFFFFFF;
	uint32_t crcTbl;
	uint8_t buff[512];
	uint32_t readAddr = OTA_START_ADDR;
	uint16_t readSize = 512 ;
        __IO int32_t len =0;

	while( len < cfg.otaVar.fileSize )
	{
		if( len + readSize > cfg.otaVar.fileSize)
		{
			readSize = cfg.otaVar.fileSize - len;
		}

		memset(buff , 0x00 , 512);
		flash.read(readAddr+len , buff , readSize);

		for (uint32_t i = 0; i!= readSize; ++i)
		{
			crcTbl = (crc32 ^ buff[i]) & 0xFF;
			crc32 = ((crc32 >> 8) & 0xFFFFFF) ^ gdwCrc32Table[crcTbl];
		}
		len += readSize;
		HAL_IWDG_Refresh(&hiwdg);

	}

	crc32 = ~crc32;

	if(( crc32 == cfg.otaVar.crc32)&&( APPLICATION_ADDRESS+cfg.otaVar.fileSize < USER_FLASH_END_ADDRESS))
	{
		log(WARN,"文件校验正确，calc CRC=%x , get crc = %x\n" , crc32 , cfg.otaVar.crc32);
		return TRUE;
	}

	log(ERR,"文件校验失败，calc CRC=%x , get crc = %x\n" , crc32 , cfg.otaVar.crc32);

	return FALSE;
}



uint8_t sys_cfg_read(SystemConfigType *data)
{
    return (chip_flash_read( DSYS_CFG_ADDR , (uint8_t *)data , sizeof(SystemConfigType) ));
}
uint8_t sys_cfg_write(SystemConfigType *data)
{
  
    return ( chip_flash_write( DSYS_CFG_ADDR , (uint8_t *)data , sizeof(SystemConfigType) ));
}

void sysCfg_save( void )
{

    cfg.crc16 = 0;
    
    memset(fb , 0xFF , sizeof(fb));
    memcpy(fb , &cfg , sizeof(SystemConfigType));
    
    cfg.crc16 =  crc16_ccitt(fb , sizeof(fb));
    
    sys_cfg_write(&cfg);
}

uint8_t read_sys_cfg( void )
{
    uint16_t crc , calcCrc;
    uint8_t cnt = 20;
    
    do
    {
        sys_cfg_read(&cfg);
        if((cfg.crc16 == 0xFFFF) && (cfg.mark == 0xFFFFFFFF))
        {
            log(WARN,"[BOOT]读取sys_cfg_read都是FF 说明无需OTA直接返回\n");
            
            return FALSE;
        }
        crc = cfg.crc16;
        cfg.crc16 = 0;
        memset(fb , 0xFF , sizeof(fb));
        memcpy(fb , &cfg , sizeof(SystemConfigType));
        
        calcCrc =  crc16_ccitt(fb , sizeof(fb));

        log(ERR,"[BOOT]读取系统配置文件,计算的CRC16 = %x, 读取到的cfg.crc16 = %x \n" , calcCrc , crc);
  
    }while((calcCrc != crc)&& (--cnt) );

    if( cnt ==0 )
    {
        log(WARN,"[BOOT]配置文件被修改 ,需要重新加载配置未见  return FALSE\n");

        return FALSE;
    }

    return TRUE;
}

/**
  * @brief  Unlocks Flash for write access
  * @param  None
  * @retval None
  */
static void FLASH_Init(void)
{
  /* Unlock the Program memory */
  HAL_FLASH_Unlock();

  /* Clear all FLASH flags */
  __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_PGSERR | FLASH_FLAG_WRPERR | FLASH_FLAG_OPTVERR);
  /* Unlock the Program memory */
  HAL_FLASH_Lock();
}

/**
  * @brief  Gets the bank of a given address
  * @param  Addr: Address of the FLASH Memory
  * @retval The bank of a given address
  */
static uint32_t GetBank(uint32_t Addr)
{
    uint32_t bank = 0;

    if (READ_BIT(SYSCFG->MEMRMP, SYSCFG_MEMRMP_FB_MODE) == 0)
    {
        /* No Bank swap */
        if (Addr < (FLASH_BASE + FLASH_BANK_SIZE))
        {
            bank = FLASH_BANK_1;
        }
        else
        {
            bank = FLASH_BANK_2;
        }
    }
    else
    {
        /* Bank swap */
        if (Addr < (FLASH_BASE + FLASH_BANK_SIZE))
        {
            bank = FLASH_BANK_2;
        }
        else
        {
            bank = FLASH_BANK_1;
        }
    }

    return bank;
}

/**
  * @brief  Gets the page of a given address
  * @param  Addr: Address of the FLASH Memory
  * @retval The page of a given address
  */
static uint32_t GetPage(uint32_t Addr)
{
  uint32_t page = 0;
  
  if (Addr < (FLASH_BASE + FLASH_BANK_SIZE))
  {
    /* Bank 1 */
    page = (Addr - FLASH_BASE) / FLASH_PAGE_SIZE;
  }
  else
  {
    /* Bank 2 */
    page = (Addr - (FLASH_BASE + FLASH_BANK_SIZE)) / FLASH_PAGE_SIZE;
  }
  
  return page;
}

/**
  * @brief  This function does an erase of all user flash area
  * @param  start: start of user flash area
  * @retval FLASHIF_OK : user flash area successfully erased
  *         FLASHIF_ERASEKO : error occurred
  */
static uint32_t FLASH_Erase( void )
{
  uint32_t FirstPage = 0, BankNumber = 0 ,PAGEError = 0;;
  FLASH_EraseInitTypeDef EraseInitStruct;
  
  /* Unlock the Flash to enable the flash control register access *************/
  HAL_FLASH_Unlock();

  /* Erase the user Flash area
    (area defined by FLASH_USER_START_ADDR and FLASH_USER_END_ADDR) ***********/

  /* Clear OPTVERR bit set on virgin samples */
  __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_OPTVERR); 
  /* Get the 1st page to erase */
  FirstPage = GetPage(APPLICATION_ADDRESS);

  /* Get the bank */
  BankNumber = GetBank(APPLICATION_ADDRESS);
  
  
    /* Fill EraseInit structure*/
    EraseInitStruct.TypeErase   = FLASH_TYPEERASE_PAGES;
    EraseInitStruct.Banks       = BankNumber;
    EraseInitStruct.Page        = FirstPage;
    EraseInitStruct.NbPages     = 128-FirstPage;

    /* Note: If an erase operation in Flash memory also concerns data in the data or instruction cache,
     you have to make sure that these data are rewritten before they are accessed during code
     execution. If this cannot be done safely, it is recommended to flush the caches by setting the
     DCRST and ICRST bits in the FLASH_CR register. */
    if (HAL_FLASHEx_Erase(&EraseInitStruct, &PAGEError) != HAL_OK)
    {
     HAL_FLASH_Lock();
     return FLASHIF_ERASEKO;
    }

    HAL_IWDG_Refresh(&hiwdg);
    /* Fill EraseInit structure*/
    EraseInitStruct.TypeErase   = FLASH_TYPEERASE_PAGES;
    EraseInitStruct.Banks       = GetBank(0x08040000);
    EraseInitStruct.Page        = GetPage(0x08040000);
    EraseInitStruct.NbPages     = GetPage(USER_FLASH_END_ADDRESS);

    /* Note: If an erase operation in Flash memory also concerns data in the data or instruction cache,
     you have to make sure that these data are rewritten before they are accessed during code
     execution. If this cannot be done safely, it is recommended to flush the caches by setting the
     DCRST and ICRST bits in the FLASH_CR register. */
    if (HAL_FLASHEx_Erase(&EraseInitStruct, &PAGEError) != HAL_OK)
    {
     HAL_FLASH_Lock();
     return FLASHIF_ERASEKO;
    }
      

  
  
  /* Lock the Flash to disable the flash control register access (recommended
     to protect the FLASH memory against possible unwanted operation) *********/
  HAL_FLASH_Lock();
  
  return FLASHIF_OK;
}

static uint32_t FLASH_Write(uint32_t destination, uint32_t *p_source, uint32_t length)
{
    uint32_t status = FLASHIF_OK;
    uint32_t i = 0;

    /* Unlock the Flash to enable the flash control register access *************/
    HAL_FLASH_Unlock();

    /* DataLength must be a multiple of 64 bit */
    for (i = 0; (i < length/2) && (destination <= (USER_FLASH_END_ADDRESS-8)); i++)
    {
        /* Device voltage range supposed to be [2.7V to 3.6V], the operation will
        be done by word */ 
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, destination, *((uint64_t *)(p_source+2*i))) == HAL_OK)      
        {
            /* Check the written value */
            if (*(uint64_t*)destination != *(uint64_t *)(p_source+2*i))
            {
                /* Flash content doesn't match SRAM content */
                status = FLASHIF_WRITINGCTRL_ERROR;
                break;
            }
            /* Increment FLASH destination address */
            destination += 8;
        }
        else
        {
            /* Error occurred while writing data in Flash memory */
            status = FLASHIF_WRITING_ERROR;
            break;
        }
    }

    /* Lock the Flash to disable the flash control register access (recommended
    to protect the FLASH memory against possible unwanted operation) *********/
    HAL_FLASH_Lock();

    return status;
}

int8_t bootload_download_to_flash( void )
{
    uint32_t baseAddr = APPLICATION_ADDRESS;
    uint32_t readAddr = OTA_START_ADDR;
    uint32_t ramsource , len =0;
    
    uint8_t buff[1024]; 

    FLASH_Erase();
    
    for( len = 0;  len < cfg.otaVar.fileSize; len += 1024 )
    {
	flash.read(readAddr+len , buff , 1024);
        
        ramsource = (uint32_t)&buff;
        
        if( FLASH_Write(baseAddr+len ,  (uint32_t*) ramsource , 256) != FLASHIF_OK)
        {
            log(ERR,"升级操作失败 , 复位设备\n");
            NVIC_SystemReset();
        }
        printf("...");
        HAL_IWDG_Refresh(&hiwdg);
    }
    
    log(INFO,"升级完成");
    
    
    return TRUE;
}

void clear_ota_mark( void )
{
	cfg.otaVar.otaUpgMark = 0;
	cfg.otaVar.crc32 = 0;
	cfg.otaVar.fileSize =0;
	cfg.otaVar.ver =0;
	sysCfg_save();
}



uint8_t bootloader_iap( void )
{
    if( read_sys_cfg() == FALSE)
    {
        return FALSE;
    }
    log(INFO,"cfg.otaVar.otaUpgMark=%X UPG_MARK=%X\n",cfg.otaVar.otaUpgMark,UPG_MARK);
    if( cfg.otaVar.otaUpgMark == UPG_MARK)
    {
        if( cfg.otaVar.fileSize == 0)
        {
            log(ERR,"文件长度为0 ， 下载文件有错误，擦除升级标志，重新下载文件\n");
            clear_ota_mark();
            return FALSE;
        }
    
        log(INFO,"需要升级程序\n");
        log(DEBUG,"*****************************************************\n");
        log(DEBUG,"文件大小 = %d , 文件 crc32 = %u ,版本号 = %d\n" , cfg.otaVar.fileSize , cfg.otaVar.crc32 , cfg.otaVar.ver);
        log(DEBUG,"*****************************************************\n");
        log(INFO,"开始校验文件完整性\n");
        if( ota_ver_file() == FALSE)
        {
            log(INFO,"文件校验失败，擦除Flash\n");
            clear_ota_mark();
            return FALSE;
        }
        log(INFO,"文件校验成功，开始擦写覆盖用户程序\n");
        if( bootload_download_to_flash() == TRUE)
        {
        	clear_ota_mark();
            log(INFO,"更新成功\n");
            return TRUE;
        }
        
    }
    return FALSE;
}

void jump_application( void )
{
    if (((*(__IO uint32_t*)APPLICATION_ADDRESS) & 0x1FFE0000 ) == 0x10000000) //stack use ram1
    {
      log(DEBUG,"[BOOT]跳转至应用程序\n\n\n");
      /* Jump to user application */
      __set_PRIMASK(1); //must be close en all
      JumpAddress = *(__IO uint32_t*) (APPLICATION_ADDRESS + 4);
      JumpToApplication = (pFunction) JumpAddress;
      /* Initialize user application's Stack Pointer */
      __set_MSP(*(__IO uint32_t*) APPLICATION_ADDRESS);
      JumpToApplication();
    } 
    else  
    {  
        log(DEBUG,"[BOOT]没有应用程序\n");  
    } 
}

/* USER CODE END 0 */
int main(void)
{

  /* USER CODE BEGIN 1 */

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
  MX_USART1_UART_Init();
  MX_IWDG_Init();
  //MX_TIM7_Init();
  /* Initialize interrupts */
  //MX_NVIC_Init();
  FLASH_Init();
  /* USER CODE BEGIN 2 */
  serial_console_init();
  spi_flash_init();
  log(DEBUG,"\n");
  log(DEBUG,"********************* 2021 Copyright by koson boot********************* \n");
  log(DEBUG,"引导系统系统编译时间: %s %s .\r\n" ,__DATE__,__TIME__);
  log(DEBUG,"检查是否是需要升级应用程序\n");
  bootloader_iap();
  
  jump_application();
  /* USER CODE END 2 */


  /* We should never get here as control is now taken by the scheduler */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
  /* USER CODE END WHILE */
    log(DEBUG,"boot i am ok!!!\n");
    HAL_Delay(1000);
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

/** NVIC Configuration
*/
void MX_NVIC_Init(void)
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

int fputc(int ch, FILE *f)
{
    if(ch == '\n')
    {
        serial.putc(console_port,'\r');
    }
    
	serial.putc(console_port, ch);
    
	return ch;
}

uint16_t osGetCPUUsage(void)
{
    return 0;
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

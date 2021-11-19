#include "main.h"
#include "stdio.h"
#include "stdint.h"
#include "string.h"
#include "stm32l4xx_hal.h"
#include "cmsis_os.h"
#include "adc.h"
#include "iwdg.h"
#include "rtc.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"
#include "unit.h"
#include "chipFlash.h"
#include "config.h"

#define FLASH_USER_END_ADDR         0x0807FFFF

/**
  * @brief  Unlocks Flash for write access
  * @param  None
  * @retval None
  */
void FLASH_Init(void)
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


uint32_t FLASH_Write(uint32_t destination, uint32_t *p_source, uint32_t length)
{
    uint32_t status = FLASHIF_OK;
    uint32_t i = 0;
    uint32_t endAddress = destination +2048;

    __set_PRIMASK(1);
    
    /* Unlock the Flash to enable the flash control register access *************/
    HAL_FLASH_Unlock();

    /* DataLength must be a multiple of 64 bit */
    for (i = 0; (i < length/2) && (destination <= (endAddress-8)); i++)
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

    __set_PRIMASK(0);
    
    return status;
}




void chip_flash_init( void )
{
    FLASH_Init();
}


uint32_t chip_flash_earse( uint32_t addr )
{
    uint32_t  PAGEError = 0;;
    FLASH_EraseInitTypeDef EraseInitStruct;

    __set_PRIMASK(1);
    
    /* Unlock the Flash to enable the flash control register access *************/
    HAL_FLASH_Unlock();

    /* Erase the user Flash area
    (area defined by FLASH_USER_START_ADDR and FLASH_USER_END_ADDR) ***********/

    /* Clear OPTVERR bit set on virgin samples */
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_OPTVERR); 

    /* Fill EraseInit structure*/
    EraseInitStruct.TypeErase   = FLASH_TYPEERASE_PAGES;
    EraseInitStruct.Banks       = GetBank(addr);
    EraseInitStruct.Page        = GetPage(addr);
    EraseInitStruct.NbPages     = 1;

    /* Note: If an erase operation in Flash memory also concerns data in the data or instruction cache,
    you have to make sure that these data are rewritten before they are accessed during code
    execution. If this cannot be done safely, it is recommended to flush the caches by setting the
    DCRST and ICRST bits in the FLASH_CR register. */
    if (HAL_FLASHEx_Erase(&EraseInitStruct, &PAGEError) != HAL_OK)
    {
        HAL_FLASH_Lock();
        
        __set_PRIMASK(0);
        
        return FLASHIF_ERASEKO;
    }

    /* Lock the Flash to disable the flash control register access (recommended
    to protect the FLASH memory against possible unwanted operation) *********/
    HAL_FLASH_Lock();

    __set_PRIMASK(0);
    
    return FLASHIF_OK;    
}

        
uint32_t chip_flash_write( uint32_t addr , uint8_t* data , uint16_t len)
{
    uint32_t ramsource = 0;
    uint8_t buff[2048];

    
    memset(buff , 0x00 , 2048);
    memcpy(buff , data , len);
    
    if( chip_flash_earse(addr) != FLASHIF_OK)
    {
        return FALSE;
    }
    
    ramsource = (uint32_t)&buff;
    
    if( FLASH_Write(addr ,  (uint32_t*) ramsource , 512) != FLASHIF_OK)
    {
        log(ERR,"内部FLASH写入失败 ,return FALSE\n");
        return FALSE;
    }
    

    return TRUE;
}

uint32_t chip_flash_read( uint32_t addr , uint8_t* data , uint16_t len )
{

    if( addr + len < FLASH_USER_END_ADDR)
    {
        for(uint16_t i = 0 ; i < len ; i++)
        {
            data[i] = *(uint8_t*)addr++;
        }
    }
    else
    {
        log(WARN,"内部FLASH读取越界\n");
        return FALSE;
    }
    
    return TRUE;
}


uint8_t chip_flash_get_lock( void )
{
    if( xSemaphoreTake( xChipFlashSemaphore, 3000 ) == pdTRUE )
    {
        //vTaskSuspendAll(); 
        __set_PRIMASK(1);  
        return TRUE;
    }
    
    return FALSE;
}

void chip_flash_release_lock( void )
{
     __set_PRIMASK(0);  
    //xTaskResumeAll();
    xSemaphoreGive( xChipFlashSemaphore );
}


/*void chip_flash_test( void )
{
    uint8_t message[512];
    
    for(uint16_t i = 0 ; i < 512 ; i++)
    {
        message[i] = i;
    }
    
    //chip_flash_write( DSYS_CFG_ADDR , message , 501 );
    //chip_flash_write( DSYS_INFO_ADDR , message , 501 );
    
    memset(message , 0x00 , 512);
    
    chip_flash_read( DSYS_CFG_ADDR , message , 501 );
    
    log_arry(DEBUG,"message" , message , 512);
    
    memset(message , 0x00 , 512);
    
    chip_flash_read( DSYS_INFO_ADDR , message , 501 );
    
    log_arry(DEBUG,"message" , message , 512);
}*/



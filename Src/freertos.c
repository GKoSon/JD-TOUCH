/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
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
#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os.h"

/* USER CODE BEGIN Includes */
#include "config.h"
#include "bsp.h"
#include "components_ins.h"
#include "module_ins.h"
#include "iwdg.h"
#include "BleDataHandle.h"
#include "beep.h"
#include "key_task.h"
#include "card.h"
#include "open_log.h"
#include "net.h"
#include "err_log.h"
#include "adc.h"
#include "socket.h"
#include "permi_list.h"
#include "tagComponent.h"
#include "mqtt_task.h"
#include "tsl_mqtt.h"
#include "mqtt_ota.h"
#include "sysCntSave.h"
#include "magnet.h"
/* USER CODE END Includes */

/* Variables -----------------------------------------------------------------*/
osThreadId mainHandle;

/* USER CODE BEGIN Variables */
SemaphoreHandle_t    xConsoleSemaphore;
SemaphoreHandle_t    xFlashSemaphore;
SemaphoreHandle_t    xChipFlashSemaphore;
SemaphoreHandle_t    xBtSemaphore;
SemaphoreHandle_t    xUsartNetSemaphore;
SemaphoreHandle_t    xSocketSemaphore;
SemaphoreHandle_t    xSocketSendSemaphore;
SemaphoreHandle_t    xSocketCmdSemaphore;
SemaphoreHandle_t    xSocketConnectSemaphore;
SemaphoreHandle_t    xMqttRecvSemaphore;
SemaphoreHandle_t    xMqttOtaSemaphore;
SemaphoreHandle_t    xBleUpdateNetSemaphore;

EventGroupHandle_t   xWatchdogEventGroup;
QueueHandle_t        xLogQueue;
QueueHandle_t        xKeyQueue;
QueueHandle_t        xMqttSendQueue;
QueueHandle_t        xMqttRecvQueue;


__IO uint8_t        rtcTimerEnable = FALSE;
__IO uint32_t       clearFlashFlag = 0;
/* USER CODE END Variables */

/* Function prototypes -------------------------------------------------------*/
void main_task(void const * argument);

extern void MX_FATFS_Init(void);
void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/* USER CODE BEGIN FunctionPrototypes */
void get_resert_status(void)
{
    if( READ_BIT(RCC->CSR, RCC_CSR_FWRSTF) != RESET)
    {
        log_err("????????????\n");
    }
    if( READ_BIT(RCC->CSR, RCC_CSR_OBLRSTF) != RESET)
    {
        log_err("????????????\n");
    }
    if( READ_BIT(RCC->CSR, RCC_CSR_PINRSTF) != RESET)
    {
        log(INFO,"????????????\n");
    }
    if( READ_BIT(RCC->CSR, RCC_CSR_BORRSTF) != RESET)
    {
        log_err("????????????????\n");
    }
    if( READ_BIT(RCC->CSR, RCC_CSR_SFTRSTF) != RESET)
    {
        log_err("????????\n");
    }
    if( READ_BIT(RCC->CSR, RCC_CSR_IWDGRSTF) != RESET)
    {
        log_err("??????????????\n");
    }
    if( READ_BIT(RCC->CSR, RCC_CSR_WWDGRSTF) != RESET)
    {
        log_err("??????????????\n");
    }
    if( READ_BIT(RCC->CSR, RCC_CSR_LPWRRSTF) != RESET)
    {
        log_err("??????????????????\n");
    }
    __HAL_RCC_CLEAR_RESET_FLAGS();
}

blinkParmType   checkErr =
{
    .mode = BLINK_OPEN_NOW,
    .openCnt = 4,
    .openTime = 40,
    .closeTime = 40,
    .delayTime = 0,
};




void MX_FREERTOS_Init(void)
{

    configASSERT((xFlashSemaphore = xSemaphoreCreateMutex()));
    configASSERT((xChipFlashSemaphore = xSemaphoreCreateMutex()));

  
    configASSERT((xConsoleSemaphore = xSemaphoreCreateBinary()));    //????
    configASSERT((xBtSemaphore = xSemaphoreCreateBinary()));        //????
    configASSERT((xUsartNetSemaphore = xSemaphoreCreateBinary()));    //??????????
    configASSERT((xSocketCmdSemaphore = xSemaphoreCreateBinary()));    //SIMCOM800C ??????????????????????????
    configASSERT((xSocketSendSemaphore = xSemaphoreCreateBinary()));    //SOCKET SIM800C??????????????????????????
    configASSERT((xSocketConnectSemaphore = xSemaphoreCreateBinary()));    //SIM800C ????
    configASSERT((xSocketSemaphore = xSemaphoreCreateBinary()));    //socket ????
    configASSERT((xMqttRecvSemaphore = xSemaphoreCreateBinary()));    //mqtt ????
    configASSERT((xMqttOtaSemaphore = xSemaphoreCreateBinary()));    //mqtt ota ????

    configASSERT((xBleUpdateNetSemaphore = xSemaphoreCreateBinary()));	//????

    xSemaphoreGive( xChipFlashSemaphore );
    xSemaphoreGive(xSocketSemaphore);
    configASSERT((xWatchdogEventGroup = xEventGroupCreate()));
    
    /* USER CODE END RTOS_SEMAPHORES */

    /* USER CODE BEGIN RTOS_TIMERS */
    /* start timers, add new ones, ... */
    /* USER CODE END RTOS_TIMERS */

    /* Create the thread(s) */
    /* definition and creation of main */
    osThreadDef(main, main_task, osPriorityRealtime, 0, configMINIMAL_STACK_SIZE*8);
    mainHandle = osThreadCreate(osThread(main), NULL);

    /* USER CODE BEGIN RTOS_THREADS */
    /* add threads, ... */

    /* USER CODE END RTOS_THREADS */

    /* USER CODE BEGIN RTOS_QUEUES */
    /* add queues, ... */
    xKeyQueue = xQueueCreate( QUEUE_KEY_TASK_LENGTH, sizeof( keyTaskType ) );
    configASSERT(xKeyQueue);

    /* Create the queue. */
    xLogQueue = xQueueCreate( QUEUE_LOG_LENGTH, sizeof( journalTaskQueueType ) );
    configASSERT(xLogQueue);

    xMqttSendQueue = xQueueCreate( QUEUE_MQTT_SEND_LENGTH, sizeof( mqttSendMsgType ) );
    configASSERT(xMqttSendQueue);

    xMqttRecvQueue = xQueueCreate( QUEUE_MQTT_RECV_LENGTH, sizeof( mqttRecvMsgType ) );
    configASSERT(xMqttRecvQueue);


    /* USER CODE END RTOS_QUEUES */
}

void show_no_feed_task( uint32_t bits )
{
    
    if( (bits & TASK_BT_BIT ) == 0 )
    {
        printf("???? ??");
    }
    
    if( (bits & TASK_KEY_BIT)  == 0 )
    {
        printf("???? ?? ");
    }
    
    if( (bits & TASK_LOG_BIT ) == 0 )
    {
        printf("???? ?? ");
    }
    
    if( (bits & TASK_SWIPE_BIT)  == 0 )
    {
        printf("?????? ");
    }
    printf("\r\n");
}
/*????????????*/
void device_set_default( void )
{

    err_log_format();
    permi.clear();
    journal.clear();

    config.write(CFG_SET_RESTORE , NULL , FALSE);

}


#include <stdlib.h>
void startota(void)
{
  printf("<<<<<<<<<--------????OTA????-------->>>>>>>>>>>\r\n");

  serverAddrType ip_port;
  memcpy(ip_port.ip,"ibinhub.com",strlen("ibinhub.com"));
  ip_port.port = 80;
  config.write(CFG_OTA_ADDR ,&ip_port,0); 



char *path = "/upload/1487627177.bin";
config.write(CFG_OTA_URL ,path,0);



otaType otaCfg;
uint8_t Md5[16]={0};
memcpy_down(Md5,"2d5b4efd001049a67f7cd5e1e5da4c66",32);
otaCfg.crc32=CRC16_CCITT(Md5,16);      
      


otaCfg.fileSize=142430;
      
      
otaCfg.ver=InterVer("1.2.3");
          
      

   /*?????????????????? ??????????*/


  config.write(CFG_OTA_CONFIG , &otaCfg,1);
  show_OTA();    
  xSemaphoreGive(xMqttOtaSemaphore);
  
  printf("<<<<<<<<<--------????OTA????-------->>>>>>>>>>>\r\n");
}
/* main_task function */

void main_task(void const * argument)
{
      EventBits_t uxBits;

      spi_flash_init();
      sysCfg_init();            //??????????????????
      bsp_gpio_map_init();        //GPIO????????????
      modules_init();             //??????????
      components_init();          //????????
      get_resert_status();        //????????????

      bluetooth_drv_init();        //??????????????
      tag_component_init();        //??????NFC????
      socket_compon_init();       //??????SOCKET
      //creat_console_task();       //????????????
      creat_buletooth_task();     //??????????
      creat_key_task();           //??????????
      creat_swipe_task();         //??????????
      creat_open_log_task();      //??????????????
      creat_mqtt_ota_task();           //ota????
      create_mqtt_task();
      
      printf("<<<<<<<<<--------????????-------->>>>>>>>>>>\r\n");

    for(;;)
    { 
        uxBits = xEventGroupWaitBits(xWatchdogEventGroup,TASK_ALL_BIT, pdTRUE,pdTRUE,2500);
        if((uxBits & TASK_ALL_BIT) == TASK_ALL_BIT)
        {
            HAL_IWDG_Refresh(&hiwdg);
            xEventGroupClearBits(xWatchdogEventGroup, uxBits);
        }
        else
        {
            log(INFO,"ubits = 0X%04X ???????????" , uxBits);
            show_no_feed_task(uxBits);
        }
        if( rtcTimerEnable)     //????????????
        {
            soft_system_resert(__func__);
        }
        if( clearFlashFlag )
        {
            if( clearFlashFlag & FLASH_PERMI_LIST_BIT)
            {
                permi.clear();
                clearFlashFlag &= ~FLASH_PERMI_LIST_BIT;
            } else  if( clearFlashFlag == FLASH_ALL_DATA)
            {
                device_set_default();
                clearFlashFlag = 0;
            }  

        }
        //read_task_stack(__func__,mainHandle);
    }
    /* USER CODE END main_task */
}

/* USER CODE BEGIN Application */
void set_clear_flash( uint32_t bits)
{
    clearFlashFlag |= bits;
}

/* USER CODE BEGIN Application */
void task_keep_alive( uint32_t taskBit)
{
    /*if(TASK_SWIPE_BIT != taskBit)
    {
        log(DEBUG , "????????:");
        if( (taskBit & TASK_CONSOLE_BIT ) )
        {
            printf("?????????? ");
        }
        
        if( (taskBit & TASK_BT_BIT ))
        {
            printf("???? ");
        }
        
        if( (taskBit & TASK_KEY_BIT))
        {
            printf("????  ");
        }
        
        if( (taskBit & TASK_LOG_BIT ) )
        {
            printf("????  ");
        }
        
        if( (taskBit & TASK_SWIPE_BIT))
        {
            printf("???? ");
        }
        printf("\r\n");
    }*/
    xEventGroupSetBits(xWatchdogEventGroup, taskBit);
}
/* USER CODE END Application */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

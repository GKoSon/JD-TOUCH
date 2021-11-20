#ifndef _CONFIG_H_
#define _CONFIG_H_

#include "stm32l4xx_hal.h"
#include "cmsis_os.h"

/* 
 *  HardFalut debug version
 */
#define HARDWARE_VERSION        "V1.0.1"
#define SOFTWARE_VERSION        "V2.0.2"

#define FLASH_ALL_DATA          0X0001
#define FLASH_PERMI_LIST_BIT    0x0002
#define    FLASH_PWD_BIT            0x0004
#define FLASH_ALL_BIT_BUT_CFG   0x0008 


#define TASK_CONSOLE_BIT        0x0001
#define TASK_BT_BIT             0x0002
#define TASK_KEY_BIT            0x0004
#define TASK_LOG_BIT            0x0008
#define TASK_SWIPE_BIT          0x0010
#define TASK_NET_BIT            0x0020
#define TASK_NET_SNED_BIT       0x0040
#define TASK_QR_CODE_BIT        0X0080
#define TASK_MATE_BIT           0x0100


#define TASK_ALL_BIT            (TASK_BT_BIT|TASK_KEY_BIT|TASK_LOG_BIT|TASK_SWIPE_BIT)  

   
extern __IO uint8_t        rtcTimerEnable;

extern uint8_t cy3116_verification( void );

extern SemaphoreHandle_t    xBtSemaphore;
extern SemaphoreHandle_t    xUsartNetSemaphore;
extern SemaphoreHandle_t    xSocketSemaphore;
extern SemaphoreHandle_t    xSocketSendSemaphore;
extern SemaphoreHandle_t    xSocketCmdSemaphore;
extern SemaphoreHandle_t    xOtaSemaphore;
extern SemaphoreHandle_t    xSocketConnectSemaphore;
extern SemaphoreHandle_t    xMqttRecvSemaphore;
extern SemaphoreHandle_t    xMqttOtaSemaphore;
extern SemaphoreHandle_t    xChipFlashSemaphore;

extern QueueHandle_t        xKeyQueue;
extern QueueHandle_t        xLogQueue;
extern QueueHandle_t        xMqttSendQueue;
extern QueueHandle_t        xMqttRecvQueue;


extern xTaskHandle                mqttTask;

extern void task_keep_alive( uint32_t taskBit);
extern void set_clear_flash( uint32_t bits);
void MX_NVIC_SetIRQ( uint8_t status );
#endif
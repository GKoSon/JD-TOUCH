#include "key_task.h"
#include "unit.h"
#include "beep.h"
#include "sysCfg.h"
#include "timer.h"
#include "open_door.h"
#include "config.h"
#include "bsp_rtc.h"
#include "open_log.h"
#include "permi_list.h"
#include "err_log.h"
#include "backlight.h"
#include "open_log.h"


static xTaskHandle     keyTask;


static void system_key_handle(keyTaskType *key)
{
    switch(key->keyValue)
    {
    case KEY_1_SEC:
    {
         beep.write(BEEP_NORMAL);      
         log(DEBUG,"使能安装工刷卡\n");
    }break;
    case KEY_8_SEC:
    {
        beep.write_base(&beepRestore);
        set_clear_flash(FLASH_ALL_DATA);
    }break;
    default:log_err("按键键值错误，VALUE=%d\n" ,key->keyValue);
    }
    
    
}

static void fun_key_handle(keyTaskType *key)
{
    switch(key->keyValue)
    {
    case OPEN_DOOR_KEY:
    {
        openlogDataType    logData;      
        memset(&logData , 0x00 , sizeof(openlogDataType));
        open_door();
        beep.write(BEEP_NORMAL);
        logData.type = OPENLOG_FORM_KEY;
        
        log(ERR,"门内开门成功\n");

//journal.save_log(&logData);  一句话使能log上报
        
    }break;

    default:log_err("按键键值错误，VALUE=%d\n" ,key->keyValue);
    }


}


static void key_handle(keyTaskType *key)
{
    switch(key->cmd)
    {
    case SYS_KEY:
    {
        system_key_handle(key);
    }break;

    case FUN_KEY:
    {
        fun_key_handle(key);
    }break;
    default:log_err("按键命令错误，CMD=%d\n" ,key->cmd);
    }
}

static void key_task( void const *pvParameters)
{

    keyTaskType key;

    configASSERT( ( ( unsigned long ) pvParameters ) == 0 );  

    memset(&key,0x00,sizeof(keyTaskType));
    
    while(1)
    { 
 
        if(xQueueReceive( xKeyQueue, &key, 1000 ) == pdTRUE)
        {
            key_handle(&key);
            memset(&key,0x00,sizeof(keyTaskType));
        }
        
        task_keep_alive(TASK_KEY_BIT);
    }
}


void creat_key_task( void )
{
    osThreadDef( key, key_task , osPriorityHigh, 0, configMINIMAL_STACK_SIZE);
    keyTask = osThreadCreate(osThread(key), NULL);
    configASSERT(keyTask);
}
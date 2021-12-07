#include "magnet.h"
#include "unit.h"
#include "bsp_gpio.h"
#include "sysCfg.h"
#include "timer.h"
#include "modules_init.h"
#include "net.h"
#include "config.h"
#include "tsl_mqtt.h"
#include "mqtt_task.h"
#include "beep.h"

extern void uploadAccessSensor(int sensorStatus)  ;

#define MAXpushTime   3
static uint8_t alarmTime=0;
static uint32_t pushTime = 0;
static uint8_t    sendedFlag = FALSE;
static pinValueEnum  magnetCloseStatus = PIN_HIGH;


static void  magnet_input_timer_isr()
{
    if(pin_ops.pin_read(MAGNET_PIN)  != magnetCloseStatus)
    {
        log(ERR,"门磁触发%d次\n",pushTime++);
    }
    else
    {
        pushTime = 0;
        if( sendedFlag == TRUE )
        {
                  log(ERR,"门磁触发已经结束 电平恢复正常 发送一个STOP消息\n");
                  sendedFlag = FALSE;         
                  uploadAccessSensor(0);            
                  
        }            
                
    }

    if( pushTime > alarmTime*60)
    //if( pushTime > alarmTime)
    {
          pushTime = 0;               
          sendedFlag = TRUE;          
          uploadAccessSensor(1);
          log(ERR,"门磁触发已经开始 发送一个START消息\n");
    }
}

/*CFG写为1*/
void magnet_input_status_init( char save )
{

    magnetCloseStatus = (pinValueEnum)pin_ops.pin_read(MAGNET_PIN);

    config.write(CFG_SYS_MAGNET_STATUS , &magnetCloseStatus , save);
    
    log(ERR,"magnet_input_status_init 设置正常门磁电平是%d\r\n",magnetCloseStatus);   

}
       


/*INIT*/
static void magnet_input_init( void )
{
    pin_ops.pin_mode(MAGNET_PIN , PIN_MODE_INPUT_PULLUP);

    timer.creat( 1000 , TRUE , magnet_input_timer_isr);  
    
    magnetCloseStatus = (pinValueEnum)config.read(CFG_SYS_MAGNET_STATUS , NULL);
    
    alarmTime = config.read(CFG_SYS_ALARM_TIME , NULL);
    
    log(ERR,"magnet_set_init_status 设置正常门磁电平是【%d】 如果监测电平和 它不同 持续时间【%d】就会报警\r\n",magnetCloseStatus,alarmTime); 
}


MODULES_INIT_EXPORT(magnet_input_init , "magnet input");



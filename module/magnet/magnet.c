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

extern void upuploadAccessSensor(long logTime ,int sensorStatus)  ;

#define MAXpushTime   3
static uint8_t alarmTime=0;
static uint32_t pushTime = 0;
static uint8_t	sendFlag = FALSE;
static pinValueEnum  magnetCloseStatus = PIN_HIGH;
uint8_t magnetAlarmStatus = STATUS_CLOSE;
uint32_t alarmStartTimer = 0;

static void  magnet_input_timer_isr()
{
	if(pin_ops.pin_read(MAGNET_PIN)  != magnetCloseStatus)
	{
          	log(ERR,"门磁触发%d次低电平\n",pushTime++);
	}
	else
	{
            pushTime = 0;
		if( sendFlag == TRUE )
		{
                  log(ERR,"门磁触发已经结束 高电平\n");
                  sendFlag = FALSE;
                  magnetAlarmStatus = STATUS_CLOSE;           
                  upuploadAccessSensor(alarmStartTimer,0);
                  log(ERR,"发送设备报警STATUS_CLOSE\r\n");               
                  
		}            
                
	}

	if( pushTime > alarmTime*60)
    //if( pushTime > alarmTime)
	{
          pushTime = 0;
                  
          sendFlag = TRUE;
          
          magnetAlarmStatus = STATUS_OPEN;
          
          if( alarmStartTimer == 0 )
          {
                  alarmStartTimer = rtc.read_stamp();
          }

          upuploadAccessSensor(alarmStartTimer,1);
          log(ERR,"发送设备报警STATUS_OPEN\r\n");
	}
}

/*CFG写为1*/
void magnet_input_status_init( char save )
{

    magnetCloseStatus = (pinValueEnum)pin_ops.pin_read(MAGNET_PIN);

    config.write(CFG_SYS_MAGNET_STATUS , &magnetCloseStatus , save);
    
    log(ERR,"magnet_input_status_init 此时修改设备门磁正常为：读到当前-门磁--电平是%d--作为正常数字\r\n",magnetCloseStatus);   

}
       


/*INIT*/
static void magnet_input_init( void )
{
    pin_ops.pin_mode(MAGNET_PIN , PIN_MODE_INPUT_PULLUP);

    timer.creat( 1000 , TRUE , magnet_input_timer_isr);  
    
    magnetCloseStatus = (pinValueEnum)config.read(CFG_SYS_MAGNET_STATUS , NULL);
    
    alarmTime = config.read(CFG_SYS_ALARM_TIME , NULL);
    
    log(ERR,"magnet_set_init_status 此时--正常--门磁--电平是【%d】MODULES_INIT_EXPORT alarmTime=【%d】\r\n",magnetCloseStatus,alarmTime); 
}


MODULES_INIT_EXPORT(magnet_input_init , "magnet input");



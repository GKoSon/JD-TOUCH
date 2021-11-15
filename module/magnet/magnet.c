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
          	log(ERR,"�ŴŴ���%d�ε͵�ƽ\n",pushTime++);
	}
	else
	{
            pushTime = 0;
		if( sendFlag == TRUE )
		{
                  log(ERR,"�ŴŴ����Ѿ����� �ߵ�ƽ\n");
                  sendFlag = FALSE;
                  magnetAlarmStatus = STATUS_CLOSE;           
                  upuploadAccessSensor(alarmStartTimer,0);
                  log(ERR,"�����豸����STATUS_CLOSE\r\n");               
                  
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
          log(ERR,"�����豸����STATUS_OPEN\r\n");
	}
}

/*CFGдΪ1*/
void magnet_input_status_init( char save )
{

    magnetCloseStatus = (pinValueEnum)pin_ops.pin_read(MAGNET_PIN);

    config.write(CFG_SYS_MAGNET_STATUS , &magnetCloseStatus , save);
    
    log(ERR,"magnet_input_status_init ��ʱ�޸��豸�Ŵ�����Ϊ��������ǰ-�Ŵ�--��ƽ��%d--��Ϊ��������\r\n",magnetCloseStatus);   

}
       


/*INIT*/
static void magnet_input_init( void )
{
    pin_ops.pin_mode(MAGNET_PIN , PIN_MODE_INPUT_PULLUP);

    timer.creat( 1000 , TRUE , magnet_input_timer_isr);  
    
    magnetCloseStatus = (pinValueEnum)config.read(CFG_SYS_MAGNET_STATUS , NULL);
    
    alarmTime = config.read(CFG_SYS_ALARM_TIME , NULL);
    
    log(ERR,"magnet_set_init_status ��ʱ--����--�Ŵ�--��ƽ�ǡ�%d��MODULES_INIT_EXPORT alarmTime=��%d��\r\n",magnetCloseStatus,alarmTime); 
}


MODULES_INIT_EXPORT(magnet_input_init , "magnet input");



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
        log(ERR,"�ŴŴ���%d��\n",pushTime++);
    }
    else
    {
        pushTime = 0;
        if( sendedFlag == TRUE )
        {
                  log(ERR,"�ŴŴ����Ѿ����� ��ƽ�ָ����� ����һ��STOP��Ϣ\n");
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
          log(ERR,"�ŴŴ����Ѿ���ʼ ����һ��START��Ϣ\n");
    }
}

/*CFGдΪ1*/
void magnet_input_status_init( char save )
{

    magnetCloseStatus = (pinValueEnum)pin_ops.pin_read(MAGNET_PIN);

    config.write(CFG_SYS_MAGNET_STATUS , &magnetCloseStatus , save);
    
    log(ERR,"magnet_input_status_init ���������Ŵŵ�ƽ��%d\r\n",magnetCloseStatus);   

}
       


/*INIT*/
static void magnet_input_init( void )
{
    pin_ops.pin_mode(MAGNET_PIN , PIN_MODE_INPUT_PULLUP);

    timer.creat( 1000 , TRUE , magnet_input_timer_isr);  
    
    magnetCloseStatus = (pinValueEnum)config.read(CFG_SYS_MAGNET_STATUS , NULL);
    
    alarmTime = config.read(CFG_SYS_ALARM_TIME , NULL);
    
    log(ERR,"magnet_set_init_status ���������Ŵŵ�ƽ�ǡ�%d�� �������ƽ�� ����ͬ ����ʱ�䡾%d���ͻᱨ��\r\n",magnetCloseStatus,alarmTime); 
}


MODULES_INIT_EXPORT(magnet_input_init , "magnet input");



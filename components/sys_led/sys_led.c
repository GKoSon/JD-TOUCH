/**
  ******************************************************************************
  * @file    sysLed.c
  * @author  Terminus hardware team
  * @version V1.0.0
  * @date    17-9-2017
  * @brief   This file provides all the buzzer firmware functions.
  ******************************************************************************
  */

#include "sys_led.h"
#include "timer.h"

#define     SYS_LED_PIN        SYS_STATUS_LED
    
/****
**
Buzzer configuration parameters
**/
static uint8_t sysLedOpenNormalCnt = 1;
static uint8_t sysLedOpenAlarmCnt = 3;
uint32_t sysLedOpenAlarmTime = 2000;

/****
**
Buzzer board parm for twinkle.
**/
twinkleType    boardsysLed;

/****
**
Operation mode definition of buzzer
**/
blinkParmType sysLedNormal=
{
    .mode = BLINK_OPEN_ALWAYS,
    .openCnt = 1,
    .openTime = 60,
    .closeTime = 60,
    .delayTime = 2000,
};



blinkParmType sysLedRestore=
{
    .mode = BLINK_OPEN_NOW,
    .openCnt = 4,
    .openTime = 150,
    .closeTime = 50,
    .delayTime = 0,
};


/****
**
buzzer functions
**/
sysLedFunsType    sysLed=
{
    .write = sysLed_write,
    .write_base = sysLed_write_base,
    .config = sysLed_config,
};


/****
**
buzzer  base openation
**/
void sysLed_write_base(blinkParmType *parm)
{
    twinkle_set(&boardsysLed , parm->mode , parm->openCnt , 
                           parm->openTime , parm->closeTime , parm->delayTime );
}


/****
**
note: Operate the buzzer immediately

:openCont   :Ring cnt
:openTime   :Open duration time
:closeTime  :Close duration time
:delayTime  :Delayed running time

**/
void sysLed_write_now(uint8_t openCnt , uint32_t openTime ,
                    uint32_t closeTime ,uint32_t delayTime)
{
    twinkle_set(&boardsysLed , BLINK_OPEN_NOW ,
                           openCnt , openTime , closeTime , delayTime );
}

/****
**
note: Delay operate the buzzer

:openCont   :Ring cnt
:openTime   :Open duration time
:closeTime  :Close duration time
:delayTime  :Delayed running time

**/
void sysLed_write_delay(uint8_t openCnt , uint32_t openTime ,
                      uint32_t closeTime ,uint32_t delayTime)
{
    if((boardsysLed.openFlag == FALSE)&&(boardsysLed.mode != BLINK_OPEN_DELAY))
    {
        twinkle_set(&boardsysLed , BLINK_OPEN_DELAY , 
                           openCnt , openTime , closeTime , delayTime );
    }
}

/****
**
note: Buzzer runs all the time

:openCont   :Ring cnt
:openTime   :Open duration time
:closeTime  :Close duration time
:delayTime  :Delayed running time

**/
void sysLed_write_always(uint8_t openCnt , uint32_t openTime ,
                       uint32_t closeTime ,uint32_t delayTime)
{
    twinkle_set(&boardsysLed , BLINK_OPEN_ALWAYS ,
                           openCnt , openTime , closeTime , delayTime );
}

/****
**
note: Operate the buzzer into normal mode

**/
void sysLed_operation_normal( void )
{
    sysLed_write_now(sysLedOpenNormalCnt,60,60,0);
}

/****
**
note: Operate the buzzer into alarm mode

**/
void sysLed_operation_alarm( void )
{
    sysLed_write_now(sysLedOpenAlarmCnt,60,60,0);
}

/****
**
note: Config buzzer parm.

:classType  :configuration option
:val        :configuration parameter

**/
void sysLed_config(uint8_t classType , uint32_t val)
{
    switch(classType)
    {
        case SYS_LED_OPEN_NORMAL_CNT:
        {
            sysLedOpenNormalCnt =  val;
        }break;
        case SYS_LED_OPEN_ALARM_CNT:
        {
            sysLedOpenAlarmCnt = val;
        }break;
        case SYS_LED_OPEN_DELAY_TIME:
        {
            sysLedOpenAlarmTime = val;
        }break;
        default:break;      
    }
}
/****
**
note: Operate the buzzer 

**/

void sysLed_write(uint8_t mode)
{
    switch(mode)
    {
        case SYS_LED_NORMAL:
        {
            twinkle_set(&boardsysLed , BLINK_OPEN_ALWAYS ,1 , 50 , 200 , 1500 );
        }break;
        case SYS_LED_CONNECT_NET:
        {
            twinkle_set(&boardsysLed , BLINK_OPEN_ALWAYS ,2 , 50 , 200 , 1500 );
        }break;
        case SYS_LED_CONNECT_SERVER:
        {			
            twinkle_set(&boardsysLed , BLINK_OPEN_ALWAYS ,3 , 50 , 200 , 1500 );
        }break;

        default:break;
    }
}

/****
**
note: Test functios

**/
void sysLed_test_mode( void )
{
    sysLed_write(SYS_LED_NORMAL);
}

void twinkle_timer_isr_led( twinkleType *p )
{
	if( p->cnt < p->flashCnt)
	{
		if( p->timeCnt ++ < p->openTime)
		{
			pin_ops.pin_write(p->pin , PIN_HIGH);
		}
		else
		{
			pin_ops.pin_write(p->pin , PIN_LOW);
		}
		
		if( p->timeCnt > p->openTime+p->closeTime)
		{
			p->timeCnt = 0;
			p->cnt++;
		}
	}
	else
	{
		if(p->timeCnt++ > p->delayTime)
        {
            p->timeCnt = 0;
            p->cnt = 0;
        }
	}
   
}


void sysLed_timer_isr( void )
{
    twinkle_timer_isr_led(&boardsysLed);
}

/****
**
buzzer init
**/
static void sysLed_init( void )
{
    boardsysLed.pin = SYS_LED_PIN;
    pin_ops.pin_mode(SYS_LED_PIN , PIN_MODE_OUTPUT);
    
    timer.creat(1 , TRUE , sysLed_timer_isr);
	
	sysLed_write(SYS_LED_NORMAL);
}


INIT_EXPORT(sysLed_init , "sysLed");



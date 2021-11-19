/**
  ******************************************************************************
  * @file    beep.c
  * @author  Terminus hardware team
  * @version V1.0.0
  * @date    17-9-2017
  * @brief   This file provides all the buzzer firmware functions.
  ******************************************************************************
  */

#include "beep.h"
#include "timer.h"

#define     BEEP_PIN        0
    
/****
**
Buzzer configuration parameters
**/
static uint8_t beepOpenNormalCnt = 1;
static uint8_t beepOpenAlarmCnt = 3;
static uint32_t beepOpenAlarmTime = 2000;

/****
**
Buzzer board parm for twinkle.
**/
twinkleType    boardBeep;

/****
**
Operation mode definition of buzzer
**/
blinkParmType beepNormal=
{
    .mode = BLINK_OPEN_NOW,
    .openCnt = 1,
    .openTime = 60,
    .closeTime = 60,
    .delayTime = 2000,
};



blinkParmType beepRestore=
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
beepFunsType    beep=
{
    .write = beep_write,
    .write_base = beep_write_base,
    .config = beep_config,
};


/****
**
buzzer  base openation
**/
void beep_write_base(blinkParmType *parm)
{
    twinkle_set(&boardBeep , parm->mode , parm->openCnt , 
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
void beep_write_now(uint8_t openCnt , uint32_t openTime ,
                    uint32_t closeTime ,uint32_t delayTime)
{
    twinkle_set(&boardBeep , BLINK_OPEN_NOW ,
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
void beep_write_delay(uint8_t openCnt , uint32_t openTime ,
                      uint32_t closeTime ,uint32_t delayTime)
{
    if((boardBeep.openFlag == FALSE)&&(boardBeep.mode != BLINK_OPEN_DELAY))
    {
        twinkle_set(&boardBeep , BLINK_OPEN_DELAY , 
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
void beep_write_always(uint8_t openCnt , uint32_t openTime ,
                       uint32_t closeTime ,uint32_t delayTime)
{
    twinkle_set(&boardBeep , BLINK_OPEN_ALWAYS ,
                           openCnt , openTime , closeTime , delayTime );
}

/****
**
note: Operate the buzzer into normal mode

**/
void beep_operation_normal( void )
{
    beep_write_now(beepOpenNormalCnt,60,60,0);
}

/****
**
note: Operate the buzzer into alarm mode

**/
void beep_operation_alarm( void )
{
    beep_write_now(beepOpenAlarmCnt,60,60,0);
}

/****
**
note: Config buzzer parm.

:classType  :configuration option
:val        :configuration parameter

**/
void beep_config(uint8_t classType , uint32_t val)
{
    switch(classType)
    {
        case BEEP_OPEN_NORMAL_CNT:
        {
            beepOpenNormalCnt =  val;
        }break;
        case BEEP_OPEN_ALARM_CNT:
        {
            beepOpenAlarmCnt = val;
        }break;
        case BEEP_OPEN_DELAY_TIME:
        {
            beepOpenAlarmTime = val;
        }break;
        default:break;      
    }
}
/****
**
note: Operate the buzzer 

**/
void beep_write(uint8_t mode)
{
    switch(mode)
    {
        case BEEP_NORMAL:
        {
            beep_write_now(beepOpenNormalCnt,60,10,0);
        }break;
        case BEEP_ALARM:
        {
            beep_write_now(beepOpenAlarmCnt,40,40,0);
        }break;
        case BEEP_DEALY:
        {            
            beep_write_delay(beepOpenAlarmCnt,40,40,beepOpenAlarmTime);
        }break;
        
        case BEEP_CLEAR:
        {
            twinkle_set(&boardBeep , BLINK_OPEN_NOW ,
                 0 , 0 , 0 , 0 );
        }break;
        case BEEP_ERR_LOAD_CFG:
        {
            beep_write_always(5 , 30 , 30 , 0 );
        }break;
        default:break;
    }
}

/****
**
note: Test functios

**/
void beep_test_mode( void )
{
    beep_write(BEEP_NORMAL);
}

void beep_timer_isr( void )
{
    twinkle_timer_isr(&boardBeep);
}

/****
**
buzzer init
**/
static void beep_init( void )
{
    boardBeep.pin = BEEP_PIN;
    pin_ops.pin_mode(BEEP_PIN , PIN_MODE_OUTPUT);
    
    //timer.creat(1 , TRUE , beep_timer_isr);
}


INIT_EXPORT(beep_init , "beep");



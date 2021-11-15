#include "twinkle.h"
#include "unit.h"
#include "bsp_gpio.h"

void twinkle_set(twinkleType *p , uint8_t Mode , uint8_t OpenCnt , uint32_t OpenTime ,uint32_t CloseTime ,uint32_t DelayTime)
{
    p->timeCnt = 0;
    p->mode = Mode;
    if( p->mode == BLINK_OPEN_DELAY)
    {  
        p->openFlag = FALSE;
    }
    else
    {
        p->openFlag = TRUE;
    }
    p->cnt = 0;
    p->flashCnt = OpenCnt;
    p->delayTime = DelayTime;
    p->openTime = OpenTime;
    p->closeTime = CloseTime;
      
}

void twinkle_timer_isr( twinkleType *p )
{
    if(p->mode == BLINK_OPEN_DELAY)
    {
        if(p->timeCnt++ > p->delayTime)
        {
            p->timeCnt = 0;
            p->openFlag = TRUE;
            p->mode = BLINK_OPEN_INIT;
        }
    }
    if(p->openFlag == TRUE)
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
            if( p->mode != BLINK_OPEN_ALWAYS)
            {
                if( ++p->cnt >= p->flashCnt)
                {
                    p->openFlag = FALSE;
                }
            }
        }
    }    
}


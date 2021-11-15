#ifndef _PeripheralTwinkle_H_
#define _PeripheralTwinkle_H_

#include <stdio.h>
#include <stdint.h>


typedef struct
{
    uint8_t         pin;
    uint32_t        timeCnt;
    uint8_t         mode;
    uint8_t         openFlag;
    int         	cnt;
    int         	flashCnt;
    uint32_t        delayTime;
    uint32_t        openTime;
    uint32_t        closeTime;
}twinkleType;


typedef struct
{
    uint8_t mode;
    uint8_t openCnt;
    uint32_t openTime;
    uint32_t closeTime;
    uint32_t delayTime;
}blinkParmType;

typedef enum
{
    BLINK_OPEN_INIT,
    BLINK_OPEN_DELAY,
    BLINK_OPEN_NOW,
    BLINK_OPEN_ALWAYS,
}TwinkleOpenType;


void twinkle_timer_isr( twinkleType *p );

void twinkle_set(twinkleType *p , uint8_t Mode , uint8_t OpenCnt , uint32_t OpenTime ,uint32_t CloseTime ,uint32_t DelayTime);



#endif



#ifndef _BEEP_H_
#define _BEEP_H_

#include "bsp_gpio.h"
#include "twinkle.h"
#include "component.h"



/* Function declaration *******************************/
typedef struct
{
    void (*init)        (void);
    void (*write)       (uint8_t mode);
    void (*write_base)  (blinkParmType *parm);
    void (*config)      (uint8_t classType , uint32_t val);
}beepFunsType;

/* eum **********************************************/
typedef enum
{
    BEEP_INIT   = 0,
    BEEP_NORMAL,
    BEEP_ALARM,
    BEEP_DEALY,
    BEEP_CLEAR,
    BEEP_ERR_LOAD_CFG,
    BEEP_MAX,
}beepOperationEnum;

typedef enum
{
    BEEP_OPEN_NORMAL_CNT = 0,
    BEEP_OPEN_ALARM_CNT,
    BEEP_OPEN_DELAY_TIME
}beepConfigTypeEnum;

/* External variable **********************************/
extern twinkleType      boardBeep;
extern beepFunsType     beep;

/* External variable for run mode ********************/
extern blinkParmType    beepNormal;
extern blinkParmType    beepRestore;

/* Buzzer functions **********************************/
void beep_init( void );
void beep_operation_normal( void );
void beep_operation_alarm( void );
void beep_write_delay(uint8_t openCnt , uint32_t openTime ,uint32_t closeTime ,uint32_t delayTime);
void beep_write_now(uint8_t openCnt , uint32_t openTime ,uint32_t closeTime ,uint32_t delayTime);
void beep_write_base(blinkParmType *parm);
void beep_write(uint8_t mode);
void beep_config(uint8_t classType , uint32_t val);


/****
* @C99
* @IAR FOR AMR 7.7
*/

#endif


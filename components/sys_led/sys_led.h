#ifndef __H_
#define __H_

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
}sysLedFunsType;

/* eum **********************************************/
typedef enum
{
    SYS_LED_INIT   = 0,
    SYS_LED_NORMAL,
	SYS_LED_CONNECT_NET,
	SYS_LED_CONNECT_SERVER,
    SYS_LED_MAX,
}sysLedOperationEnum;

typedef enum
{
    SYS_LED_OPEN_NORMAL_CNT = 0,
    SYS_LED_OPEN_ALARM_CNT,
    SYS_LED_OPEN_DELAY_TIME
}sysLedConfigTypeEnum;

/* External variable **********************************/
extern twinkleType			boardBeep;
extern sysLedFunsType		sysLed;

/* External variable for run mode ********************/
extern blinkParmType    sysLedNormal;
extern blinkParmType    sysLedRestore;

/* Buzzer functions **********************************/
void sysLed_init( void );
void sysLed_operation_normal( void );
void sysLed_operation_alarm( void );
void sysLed_write_delay(uint8_t openCnt , uint32_t openTime ,uint32_t closeTime ,uint32_t delayTime);
void sysLed_write_now(uint8_t openCnt , uint32_t openTime ,uint32_t closeTime ,uint32_t delayTime);
void sysLed_write_base(blinkParmType *parm);
void sysLed_write(uint8_t mode);
void sysLed_config(uint8_t classType , uint32_t val);


/****
* @C99
* @IAR FOR AMR 7.7
*/

#endif


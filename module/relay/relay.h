#ifndef _RELAY_H_
#define _RELAY_H_

#include "bsp_gpio.h"
#include "twinkle.h"



/* Function declaration *******************************/
typedef struct
{
    void (*init)        (void);
    void (*open)        (uint32_t delay_time);
    void (*control)        (uint8_t status);
}relayFunsType;

extern twinkleType  boardRelay;
extern relayFunsType relay;

#endif


#ifndef _BACKLIGHT_H_
#define _BACKLIGHT_H_

#include "stdint.h"
#include "bsp_gpio.h"
#include "twinkle.h"

typedef enum
{
    BACKLIGHT_OFF,
    BACKLIGHT_ON,
}backlightCtrlEnum;

extern twinkleType    backlight;
void backlight_set( backlightCtrlEnum status);
#endif

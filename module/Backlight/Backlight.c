#include "backlight.h"
#include "gpio.h"
#include "unit.h"
#include "component.h"
#include "pwm.h"


#define	BACKLIGHT_PIN			15

twinkleType    backlight;

void backlight_set( backlightCtrlEnum status)
{
    if( status == BACKLIGHT_OFF)
    {
    	twinkle_set(&backlight ,BLINK_OPEN_DELAY , 1 ,0 , 0,3000);
    }
    else if( status == BACKLIGHT_ON)
    {
        pin_ops.pin_write(BACKLIGHT_PIN , PIN_HIGH );
    }
}

void backlight_init( void )
{
	backlight.pin = BACKLIGHT_PIN;
	pin_ops.pin_mode(BACKLIGHT_PIN , PIN_MODE_OUTPUT);
	pin_ops.pin_write(BACKLIGHT_PIN , PIN_HIGH );
    
    pwm_init();
}

INIT_EXPORT(backlight_init , "backlight");


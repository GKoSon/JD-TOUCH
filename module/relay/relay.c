#include "relay.h"
#include "modules_init.h"
#include "timer.h"



twinkleType    boardRelay;

void relay_init( void );

void leds_init( void );
void relay_open( uint32_t delay_time );
void relay_control(uint8_t status);

relayFunsType relay = 
{
    .open = relay_open,
	.control = relay_control,
};

void relay_control(uint8_t status)
{
	pin_ops.pin_write(RELAY_PIN , status);
}

void relay_open( uint32_t delay_time )
{
    twinkle_set(&boardRelay ,BLINK_OPEN_NOW , 1 ,delay_time*100 , 0,0);
}

void relay_timer_isr( void )
{
    twinkle_timer_isr(&boardRelay);
}

void relay_init( void )
{
    boardRelay.pin = RELAY_PIN;
    pin_ops.pin_mode(boardRelay.pin , PIN_MODE_OUTPUT);
    
    timer.creat(1 , TRUE , relay_timer_isr);
}

MODULES_INIT_EXPORT(relay_init , "relay");

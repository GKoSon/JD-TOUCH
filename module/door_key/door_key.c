#include "config_key.h"
#include "timer.h"
#include "bsp_gpio.h"
#include "modules_init.h"
#include "key_task.h"
#include "config.h"



static uint32_t pushTime=0;
static uint8_t doorKeyTimerHandel = 0xFF;

static void door_key_timer_isr(void)
{
    if(pin_ops.pin_read(DOOR_KEY_PIN)  == PIN_LOW)
    {
        pushTime++;
    }
    else
    {
        pushTime = 0;
        timer.stop(doorKeyTimerHandel);
    }
    if( pushTime == 2)
    {
        keyTaskType key;

        key.cmd = FUN_KEY;
        key.keyValue = OPEN_DOOR_KEY;
        xQueueSendFromISR( xKeyQueue, ( void* )&key, NULL );
    }

}


void door_key_exit_interrupt( void )
{
    if(pin_ops.pin_read(DOOR_KEY_PIN)  == PIN_LOW)
    {
        timer.start(doorKeyTimerHandel);
    }
    //log(INFO,"中断检测到门内按键输入，开启定时器，需要持续按键超过200ms ,INPUT =%d\n" ,pin_ops.pin_read(DOOR_KEY_PIN));
}

static void door_key_init( void )
{
    pin_ops.pin_mode(DOOR_KEY_PIN , PIN_MODE_INPUT_PULLUP);
    pin_ops.pin_exit(DOOR_KEY_PIN , door_key_exit_interrupt);
    doorKeyTimerHandel = timer.creat( 100 , FALSE , door_key_timer_isr);
}


MODULES_INIT_EXPORT(door_key_init , "door key");




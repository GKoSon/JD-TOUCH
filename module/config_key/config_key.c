#include "config_key.h"
#include "timer.h"
#include "gpio.h"
#include "component.h"
#include "key_task.h"
#include "bsp_gpio.h"
#include "config.h"
#include "modules_init.h"


static uint8_t configKeyHandle = 0xFF;
static uint32_t pushTime=0;

static void config_key_timer_isr(void)
{
    if(pin_ops.pin_read(CONFIG_KEY_PIN)  == PIN_LOW)
    {
        pushTime++;
    }
    else
    {
        pushTime = 0;
        timer.stop(configKeyHandle);
    }
    if( pushTime == 2)
    {
    	keyTaskType key;

    	key.cmd = SYS_KEY;
    	key.keyValue = KEY_1_SEC;
    	xQueueSendFromISR( xKeyQueue, ( void* )&key, NULL );
    }
    else if( pushTime == 40)
    {
    	keyTaskType key;

		key.cmd = SYS_KEY;
		key.keyValue = KEY_8_SEC;
		xQueueSendFromISR( xKeyQueue, ( void* )&key, NULL );
    }
}

static void config_key_exit_interrupt( void )
{
    if( pin_ops.pin_read(CONFIG_KEY_PIN) == PIN_LOW)
    {
        timer.start(configKeyHandle);
        log(INFO,"中断检测到系统按键输入 ,INPUT =%d\n" ,pin_ops.pin_read(CONFIG_KEY_PIN));
    }
}

static void config_key_init( void )
{
    pin_ops.pin_mode(CONFIG_KEY_PIN , PIN_MODE_INPUT_PULLUP);
    pin_ops.pin_exit(CONFIG_KEY_PIN , config_key_exit_interrupt);
    
    configKeyHandle = timer.creat( 100 , FALSE , config_key_timer_isr);   
}


MODULES_INIT_EXPORT(config_key_init , "config key");




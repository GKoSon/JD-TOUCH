#include "i2c.h"
#include "unit.h"
#include "gpio.h"
#include "beep.h"
#include "i2c.h"
#include "key_task.h"

#define	EN_PIN		                (8)
#define	ISR_PIN		                (9)
#define BS18B20A_TASK_PARAMETER     ( 0x22UL )

static SemaphoreHandle_t    xBs83b12Semaphore;
static xTaskHandle 	        bs83b12Task;
static void                 *tranPort;



static uint8_t bs83b12a_read(void* port,uint8_t slaveAdr,uint16_t subAdr,uint8_t *buff,uint16_t bufLen)
{
    i2cPortType *i2c = (i2cPortType *)port;
    volatile uint8_t dummy = 50;
    uint8_t  startAddr=0;	

    while(subAdr >=256)
    {
        subAdr = subAdr - 256;
        slaveAdr = slaveAdr + 0x02;
    }		
    startAddr = (uint8_t)subAdr;

    dummy = 20;
    
	while(dummy--){
		if (swI2c.access_start(i2c, slaveAdr, I2C_TRANS_WRITE) == FALSE)
        {
			continue;
		}
		if (swI2c.send_byte(i2c, startAddr) == I2C_NON_ACKNOWLEDGE)
        {
			continue;
		}
		if (swI2c.access_start(i2c, slaveAdr, I2C_TRANS_READ) == FALSE)
        {
			continue;
		}
		while(bufLen--)
        {
			*buff = swI2c.receive_byte(i2c, (bufLen>0)?TRUE:FALSE);
			buff++;
		}
		break;
	}
	swI2c.stop(i2c);    
    
    return TRUE;
}

  
static touchKeyEnum bs83b12a_read_value( void )
{
    uint8_t uD[4]={0x00,0x00,0x00,0x00};
    uint16_t touchDataTemp = 0;
    touchKeyEnum key = KEY_INIT;
    bs83b12a_read(tranPort,0xA0,8 ,uD , 4);
    touchDataTemp = uD[1];
    touchDataTemp <<= 8;
    touchDataTemp |= uD[0];
 
    switch(touchDataTemp)
	{
		case 0x0001: key = KEY_ENT; break;
		case 0x0002: key = KEY_0; break;
		case 0x0004: key = KEY_DEL; break;
		case 0x0008: key = KEY_9; break;
		case 0x0010: key = KEY_8; break;
		case 0x0020: key = KEY_7; break;
		case 0x0040: key = KEY_6; break;
		case 0x0080: key = KEY_5; break;
		case 0x0100: key = KEY_4; break;
		case 0x0200: key = KEY_3; break;
		case 0x0400: key =KEY_2; break;
		case 0x0800: key = KEY_1; break;
		default : log(DEBUG,"Read error value = %x.\r\n" , touchDataTemp);break;
	}

    return key;  
}


static void bs83b12_task( void *pvParameters)
{
    //unsigned portBASE_TYPE uxHighWaterMark;

    configASSERT( ( ( unsigned long ) pvParameters ) == BS18B20A_TASK_PARAMETER );

    while(1)
    {
    	if( xSemaphoreTake( xBs83b12Semaphore, 1000 ) == pdTRUE )
		{
			keyTaskType key;

			key.cmd = TOUCH_KEY;
			key.keyValue = bs83b12a_read_value();
			if(key.keyValue != KEY_INIT)
			{
				xQueueSendFromISR( xKeyTaskQueue, ( void* )&key, NULL );
			}
		}
        
        xEventGroupSetBits(xCreatedEventGroup, TASK_TOUCH_IC_BIT); 
        
        #if 0
        uxHighWaterMark = uxTaskGetStackHighWaterMark(bs83b12Task);
        if( uxHighWaterMark < 128)
        {
            log(ERR,"[%s]stack surplus is too little = %d.\r\n" ,__func__,uxHighWaterMark);
        }
        #endif
    }
}


static void creat_bs83b12_task( void )
{
    /* Create the queue. */
	xBs83b12Semaphore = xSemaphoreCreateBinary();

    if( xBs83b12Semaphore != NULL )
	{
		xTaskCreate( bs83b12_task,					    /* The function that implements the task. */
					"bs83b12 task", 					    	/* The text name assigned to the task - for debug only as it is not used by the kernel.*/
					 configMINIMAL_STACK_SIZE, 			/* The size of the stack to allocate to the task. */
					( void * )BS18B20A_TASK_PARAMETER,  	/* The parameter passed to the task - just to check the functionality. */
					TOUCH_IC_PRIORITY,                      /* The priority assigned to the task. */
					&bs83b12Task );				/* The task handle is not required, so NULL is passed. */
	}
}

static void bs83b12a_exit_interrupt( void )
{
    if( pin_ops.pin_read(ISR_PIN) == PIN_HIGH)
    {
        static BaseType_t xHigherPriorityTaskWoken;
        xSemaphoreGiveFromISR( xBs83b12Semaphore, &xHigherPriorityTaskWoken );
    }
}

static void bs83b12a_init( void )
{
	tranPort = swI2c.open("i2c2");

	if( tranPort == NULL)
	{
		beep.write(BEEP_ALARM);
		log(ERR,"BS83B12Ad I2C´ò¿ªÊ§°Ü\n");
		return ;
	}
    swI2c.init(tranPort);
	pin_ops.pin_mode(EN_PIN , PIN_MODE_OUTPUT);
	pin_ops.pin_mode(ISR_PIN , PIN_MODE_INPUT);
    pin_ops.pin_exit(ISR_PIN ,bs83b12a_exit_interrupt );
    pin_ops.pin_write(EN_PIN , PIN_HIGH);
	creat_bs83b12_task();
}

MODULES_INIT_EXPORT(bs83b12a_init , "´¥ÃþÐ¾Æ¬---bs83b12a");


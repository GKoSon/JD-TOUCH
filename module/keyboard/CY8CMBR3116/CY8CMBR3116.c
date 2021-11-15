#include "i2c.h"
#include "unit.h"
#include "bsp_gpio.h"
#include "beep.h"
#include "i2c.h"
#include "key_task.h"
#include "modules_init.h"
#include "config.h"
#include "sysCfg.h"


#define DEVICE_ADDR               (0x37u)
/* I2C and EZI2C slave address defines */
#define I2C_I2C_SLAVE_ADDR_POS    (0x01u)    /* 7-bit address shift */
#define I2C_I2C_SLAVE_ADDR_MASK   (0xFEu)    /* 8-bit address mask */
/* Internal I2C component constants */
#define I2C_I2C_READ_FLAG         (0x01u) /* Read flag of the Address */
/* Return 8-bit address. The input address should be 7-bits */
#define I2C_GET_I2C_8BIT_ADDRESS(addr) (((unsigned long) ((unsigned long) (addr) << I2C_I2C_SLAVE_ADDR_POS)) & I2C_I2C_SLAVE_ADDR_MASK)



//static SemaphoreHandle_t    xTouchKeySemaphore;
//static xTaskHandle 	        touchKeyTask;
static void                 *tranPort;


unsigned char CY8CMBR3116_config[128] = {
    0xF8u, 0x7Fu, 0xF8u, 0x7Fu, 0x00u, 0x00u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x7Fu, 0x7Fu, 0x7Fu, 0x80u,
    0x80u, 0x80u, 0x80u, 0x80u, 0x80u, 0x80u, 0x80u, 0x80u,
    0x80u, 0x80u, 0x80u, 0x7Fu, 0x03u, 0x00u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
    0x05u, 0x00u, 0x00u, 0x02u, 0x00u, 0x02u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x21u, 0x01u, 0x01u,
    0x00u, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu,
    0xFFu, 0x00u, 0x00u, 0x00u, 0x24u, 0x03u, 0x01u, 0x59u,
    0x00u, 0x37u, 0x01u, 0x00u, 0x00u, 0x0Au, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0xD9u, 0x55u
};


static uint8_t cy3116_write(void* port,uint8_t slaveAdr,uint16_t subAdr,uint8_t *buff,uint16_t bufLen)
{
    i2cPortType *i2c = (i2cPortType *)port;
    volatile uint8_t dummy = 20;
    uint8_t slaveAddress = I2C_GET_I2C_8BIT_ADDRESS(slaveAdr);
    
	while(dummy--)
    {
		if (swI2c.access_start(i2c, slaveAddress, I2C_TRANS_WRITE) == FALSE)
        {
			continue;
		}
		if (swI2c.send_byte(i2c, subAdr) == I2C_NON_ACKNOWLEDGE)
        {
			continue;
		}
        
		for( uint16_t i =0; i < bufLen ;i++)
        {			
            if (swI2c.send_byte(i2c, buff[i]) == I2C_NON_ACKNOWLEDGE)
            {
                goto err;
            }
		}
		break;

	}
    
    if( dummy == 0xFF)
    {
        log(INFO,"I2C Timer out\r\n");
    }
    
	swI2c.stop(i2c);    
    
    return TRUE;
err:
    log(INFO,"I2C send err\r\n");

    return FALSE;
}



static uint8_t cy3116_read(void* port,uint8_t slaveAdr,uint16_t subAdr,uint8_t *buff,uint16_t bufLen)
{
    i2cPortType *i2c = (i2cPortType *)port;
    volatile uint8_t dummy = 20;

	while(dummy--){
		if (swI2c.access_start(i2c, slaveAdr, I2C_TRANS_WRITE) == FALSE)
        {
			continue;
		}
		if (swI2c.send_byte(i2c, subAdr) == I2C_NON_ACKNOWLEDGE)
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

  
touchKeyEnum cy3116_read_key( void )
{
    uint8_t uD[4]={0x00,0x00,0x00,0x00};
    uint16_t touchDataTemp = 0;
    
    touchKeyEnum key = KEY_INIT;
    
    cy3116_read(tranPort,DEVICE_ADDR<<1,0xaa ,uD , 4);
    
    touchDataTemp = uD[1];
    touchDataTemp <<= 8;
    touchDataTemp |= uD[0];
 
    //log(DEBUG,"Read value = %x.\r\n" , touchDataTemp);
      
    switch(touchDataTemp)
	{
		case 0x10:   key = TOUCHV? KEY_7:KEY_ENT; break;
		case 0x100:  key = TOUCHV? KEY_4:KEY_0;   break;  
		case 0x08:   key = TOUCHV? KEY_1:KEY_DEL; break; 
		case 0x20:   key = TOUCHV? KEY_8:KEY_9;   break;  
		case 0x200:  key = TOUCHV? KEY_5:KEY_8;   break;  
		case 0x4000: key = TOUCHV? KEY_2:KEY_7;   break;
		case 0x40:   key = TOUCHV? KEY_9:KEY_6;   break;  
		case 0x400:  key = TOUCHV? KEY_6:KEY_5;   break;  
		case 0x2000: key = TOUCHV? KEY_3:KEY_4;   break;   
		case 0x80:   key = TOUCHV? KEY_DEL:KEY_3; break;  
		case 0x800:  key = TOUCHV? KEY_0: KEY_2;  break;    
		case 0x1000: key = TOUCHV? KEY_ENT:KEY_1; break; 
		default : //log(DEBUG,"Read error value = %x.\r\n" , touchDataTemp);
        break;
	}

    return key;  
}


void touch_key_config( void )
{
    uint8_t saveCmd = 0x02;
    uint8_t resert = 0xFF;

    if( cy3116_verification() != TRUE)
    {
        log_err("´¥ÃþÐ¾Æ¬¼ÓÔØ´íÎó\n");
        return;
    }
    HAL_IWDG_Refresh(&hiwdg);
    cy3116_write(tranPort , DEVICE_ADDR ,0x00,CY8CMBR3116_config,128);
    sys_delay(100);
    HAL_IWDG_Refresh(&hiwdg);
    cy3116_write(tranPort , DEVICE_ADDR ,0x86,&saveCmd,1);
    sys_delay(100);
    HAL_IWDG_Refresh(&hiwdg);
    cy3116_write(tranPort , DEVICE_ADDR ,0x86,&resert,1);
    sys_delay(100);
    HAL_IWDG_Refresh(&hiwdg);
}


#if 0
static void touch_key_task( void *pvParameters)
{
    //unsigned portBASE_TYPE uxHighWaterMark;

    configASSERT( ( ( unsigned long ) pvParameters ) == TASK_PARAMETER );

    touch_key_config();
    
    while(1)
    {
    	if( xSemaphoreTake( xTouchKeySemaphore, 1000 ) == pdTRUE )
		{
			keyTaskType key;

			key.cmd = TOUCH_KEY;
			key.keyValue = touch_key_read_value();
			if(key.keyValue != KEY_INIT)
			{
				xQueueSendFromISR( xKeyTaskQueue, ( void* )&key, NULL );
			}
		}
        
        xEventGroupSetBits(xCreatedEventGroup, TASK_TOUCH_IC_BIT); 

    }
}

static void creat_touch_key_task( void )
{
    /* Create the queue. */
	xTouchKeySemaphore = xSemaphoreCreateBinary();

    if( xTouchKeySemaphore != NULL )
	{
		xTaskCreate( touch_key_task,					    /* The function that implements the task. */
					"touch key", 					    	/* The text name assigned to the task - for debug only as it is not used by the kernel.*/
					 configMINIMAL_STACK_SIZE*2, 			/* The size of the stack to allocate to the task. */
					( void * )TASK_PARAMETER,  	/* The parameter passed to the task - just to check the functionality. */
					TOUCH_IC_PRIORITY,                      /* The priority assigned to the task. */
					&touchKeyTask );				/* The task handle is not required, so NULL is passed. */
	}
}
#endif


static void touch_key_exit_interrupt( void )
{
    if( pin_ops.pin_read(CY3116_ISR_PIN) == PIN_LOW)
    {
        static BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        keyTaskType key;
        
        //log(WARN,"ÓÐ´¥Ãþ°´¼üÖÐ¶Ï²úÉú\n");

        key.cmd = TOUCH_KEY;
        xQueueSendFromISR( xKeyQueue, ( void* )&key, NULL );
        
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);  

    }
}

uint8_t cy3116_verification( void )
{
    uint8_t id =0;
    
    cy3116_read(tranPort,DEVICE_ADDR<<1,0x8f ,&id , 1);
    
    if( id != 154)
    {
        return FALSE;
    }
    
    return TRUE;
}

void touch_key_init( void )
{
	tranPort = swI2c.open("i2c2");

	if( tranPort == NULL)
	{
		beep.write(BEEP_ALARM);
		log_err("I2C 2´ò¿ªÊ§°Ü\n");
		return ;
	}
                swI2c.init(tranPort);
	
	


		pin_ops.pin_mode(CY3166_RST_PIN , PIN_MODE_OUTPUT);
		pin_ops.pin_mode(CY3116_ISR_PIN , PIN_MODE_INPUT);
		
		pin_ops.pin_exit(CY3116_ISR_PIN ,touch_key_exit_interrupt );
		
		pin_ops.pin_write(CY3166_RST_PIN , PIN_LOW);
		sys_delay(10);
		pin_ops.pin_write(CY3166_RST_PIN , PIN_HIGH);

    
}

MODULES_INIT_EXPORT(touch_key_init , "cy3116");


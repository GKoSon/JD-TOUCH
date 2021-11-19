#include "w5500_drv.h"
#include "hal_spi.h"
#include "bsp_gpio.h"
#include "unit.h"
#include "modules_init.h"


void *w5500Port = NULL;





void w5500_drv_resert( void )
{
    pin_ops.pin_write(W5500_RST_PIN ,PIN_LOW);
    
    sys_delay(2);
    
    pin_ops.pin_write(W5500_RST_PIN ,PIN_HIGH);
    
    sys_delay(100);
}

void w5500_chip_enable( void )
{
    halSpi.chip_select(w5500Port,ENABLE);
}

void w5500_chip_disable( void )
{ 
    halSpi.chip_select(w5500Port,DISABLE);
}

void w5500_write_byte( uint8_t txCh)
{
    halSpi.send_reveive_byte( w5500Port , txCh);
    
   // printf("%0.2x",txCh);
}

uint8_t w5500_read_byte( void )
{
    return ( halSpi.send_reveive_byte(w5500Port , 0xFF) );
}


void w5500_mutex_enter(void)
{    
    __set_PRIMASK(1);  
}

void w5500_mutex_exit(void)
{
    __set_PRIMASK(0);
}


void w5500_drv_init( void )
{
    w5500Port = halSpi.open("spi3");
    
    if( w5500Port == NULL)
    {
        log(WARN," w5500 spi init error\n");
        return ;
    }
    
    pin_ops.pin_mode(W5500_RST_PIN , PIN_MODE_OUTPUT);
    pin_ops.pin_write(W5500_RST_PIN ,PIN_HIGH);
    
    halSpi.init(w5500Port); 
    
}
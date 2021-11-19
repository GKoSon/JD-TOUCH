#include "SN75176BDR.h"
#include "beep.h"
#include "serial.h"
#include "modules_init.h"




void *sn75176Port = NULL;


void sn7516_receive_byte(uint8_t ch )
{
    printf("%x " ,ch);
}


void sn75176_set_en( uint8_t status )
{
    pin_ops.pin_write(RS485_EN_PIN , status);
}


void sn75176_sendbyte( uint8_t ch)
{
    sn75176_set_en(PIN_HIGH);
    
    serial.putc(sn75176Port , ch );
    
    sn75176_set_en(PIN_LOW);
}


void sn75176_send( uint8_t *pData , uint16_t len)
{
    sn75176_set_en(PIN_HIGH);

    //sys_delay(5);

    serial.puts(sn75176Port , pData , len);
    
    //sys_delay(5);

    sn75176_set_en(PIN_LOW);
}


void sn75176_init( void )
{
    sn75176Port = serial.open("serial5");
    
    if( sn75176Port == NULL)
    {
        beep.write(BEEP_ALARM);
        return ;
    }

    serial.init(sn75176Port  , 115200 ,sn7516_receive_byte);

    pin_ops.pin_mode(RS485_EN_PIN , PIN_MODE_OUTPUT);
    sn75176_set_en(PIN_LOW);
}


rs485OpsType sn7517Ops = 
{
    .putc = sn75176_sendbyte,
    .puts = sn75176_send,
};

//MODULES_INIT_EXPORT(sn75176_init , "sn75176");


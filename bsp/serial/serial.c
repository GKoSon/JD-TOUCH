#include "serial.h"

static void *serial_init( void *port , uint32_t band , rxIsrCb isr)
{
    UART_HandleTypeDef *serial = (UART_HandleTypeDef *)port;

    serial->RxIsrCb = isr;

    HAL_UART_SetIrq(serial);
    
    return port;
}

static void serial_putc(void *port , uint8_t ch)
{
    UART_HandleTypeDef *serial = (UART_HandleTypeDef *)port;
    
    HAL_UART_Transmit(serial, (uint8_t *)&ch, 1, 0xFFFF);
}

static void serial_puts(void *port,uint8_t *Data , uint16_t Length)
{
    UART_HandleTypeDef *serial = (UART_HandleTypeDef *)port;
    
    HAL_UART_Transmit(serial, (uint8_t *)Data, Length, 0xFFFF);
}


device_serial   serial_t[]=
{
    {"serial1" , &huart1},
    {"serial2" , &huart2},
    {"serial3" , &huart3},
    {"serial4" , &huart4},
    {"serial5" , &huart5},
};

void *serial_open( char *name )
{
    for( uint8_t i = 0 ; i < sizeof (serial_t) / sizeof (serial_t[0]);  i++)
    {
        if( strcmp(name , serial_t[i].name) == 0 )
        {
            return serial_t[i].serial;
        }
    }
    
    return NULL;
}


serialOpsType    serial=
{
    serial_open,
    serial_init,
    serial_putc,
    serial_puts,
};

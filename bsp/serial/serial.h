#ifndef _SERIAL_H_
#define _SERIAL_H_


#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "usart.h"

typedef void (*rxIsrCb)    (uint8_t);


typedef struct _stm32_uart_ops
{
    void*   (*open)     ( char *name );
    void*   (*init)     ( void *serial , uint32_t band , rxIsrCb isr);
    void    (*putc)     ( void *port, uint8_t c);
    void    (*puts)     ( void *port, uint8_t *data , uint16_t length);
}serialOpsType;

typedef struct
{
    char *name;
    void *serial;
}device_serial;


extern serialOpsType    serial;

#endif


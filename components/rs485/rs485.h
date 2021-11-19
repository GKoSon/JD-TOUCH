#ifndef _RS485_H_
#define _RS485_H_

#include "stdlib.h"
#include "stdint.h"

typedef struct
{
    void (*putc)    (uint8_t ch);
    void (*puts)    (uint8_t *pData , uint16_t len);
    void (*getch)    (uint8_t ch);
}rs485OpsType;



void rs485_puts(uint8_t *pData , uint16_t len);
void rs485_putc( uint8_t ch );


#endif


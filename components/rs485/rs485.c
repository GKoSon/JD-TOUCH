#include "rs485.h"
#include "SN75176BDR.h"
#include "component.h"



rs485OpsType *rs485Com = NULL;

void rs485_putc( uint8_t ch )
{
    rs485Com->putc(ch);
}

void rs485_puts(uint8_t *pData , uint16_t len)
{
    rs485Com->puts(pData , len);
}


void rs485_init( void )
{    
    rs485Com = &sn7517Ops;    
}

//INIT_EXPORT(rs485_init , "485");
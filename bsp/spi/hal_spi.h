#ifndef _HAL_SPI_H_
#define _HAL_SPI_H_

#include <stdint.h>
#include <string.h>
#include "spi.h"



typedef struct
{
    char *name;
    void *spi;
}device_spi;

typedef struct _stm32_spi_ops
{
    void*   (*open)             ( char *name );
    void*   (*init)             ( void *port);
    void    (*chip_select)      ( void *port , uint8_t NewState );
    uint8_t (*send_reveive_byte)( void *port , uint8_t  data) ;
    uint8_t (*send_reveive_buff)( void *port , uint8_t *pCommand, uint16_t length, uint8_t *pResponse);
}halSpiType;



extern halSpiType    halSpi;

#endif

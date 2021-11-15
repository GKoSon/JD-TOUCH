#ifndef _W5500_DRV_H_
#define _W5500_DRV_H_

#include "stdint.h"

void w5500_mutex_exit(void);
void w5500_mutex_enter(void);

void w5500_chip_disable( void );
void w5500_chip_enable( void );

uint8_t w5500_read_byte( void );
void w5500_write_byte( uint8_t txCh);

void w5500_drv_init( void );
void w5500_drv_resert( void );

#endif
#ifndef _HAL_NFC_SPI_H_
#define	_HAL_NFC_SPI_H_

#include <string.h>
#include <stdbool.h>
#include "stm32l4xx.h"
#include "hal_spi.h"
#include "bsp_gpio.h"
#include "config.h"
#include "unit.h"
#include "st95int.h"


typedef struct
{
	void (*SpiInit)			(void);
	//void (*InterruptInit)	(void);
	uint8_t (*SendReceiveByte)	(uint8_t);
	void (*SendReceive)		(uc8 *pCommand, uint16_t length, uint8_t *pResponse); 
	void (*ChipSelet)		( uint8_t );
	void (*ChipWakeUp)		( uint8_t );
	void (*ChipInterr)		( uint8_t );
	void (*DelayMs)			( uint32_t );
	void (*DelayUs)			( uint32_t );
}HalNfcType;


void HalDelayMs( uint32_t DelayTime );

extern HalNfcType *nSpi;

#endif




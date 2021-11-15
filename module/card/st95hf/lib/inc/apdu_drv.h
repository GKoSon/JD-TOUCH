#ifndef __APDU_DRV_H_
#define __APDU_DRV_H_


#include "lib_PCD.h"

extern drv95HF_ConfigStruct  drv95HFConfig;



// CR95HF status
#define CR95HF_SUCCESS_CODE												RESULTOK
#define CR95HF_NOREPLY_CODE												0x01

// error code
#define	CR95HF_ERRORCODE_DEFAULT										0xFE
#define	CR95HF_ERRORCODE_TIMEOUT										0xFD
#define	CR95HF_ERRORCODE_UARTDATARATEUNCHANGED			                0xFC
#define	CR95HF_ERRORCODE_UARTDATARATEPROCESS				            0xFB
#define	CR95HF_ERRORCODE_FILENOTFOUND								    0xFA
#define	CR95HF_ERRORCODE_READALLMEMORY							        0xF9
#define	CR95HF_ERRORCODE_TAGDETECTINGCALIBRATION		                0xF8
#define	CR95HF_ERRORCODE_CUSTOMCOMMANDUNKNOWN				            0xF7
#define	CR95HF_ERRORCODE_TAGDETECTING								    0xF5
#define	CR95HF_ERRORCODE_NOTAGFOUND									    0xF4
#define CR95HF_ERROR_CODE												0x40
#define CR95HF_ERRORCODE_PARAMETERLENGTH						        0x41
#define CR95HF_ERRORCODE_PARAMETER									    0x42
#define CR95HF_ERRORCODE_COMMANDUNKNOWN							        0x43
#define CR95HF_ERRORCODE_PORERROR										0x44


#define CR95HF_COMMAND_SEND													0x00
#define CR95HF_COMMAND_RESET												0x01
#define CR95HF_COMMAND_RECEIVE											    0x02
#define CR95HF_COMMAND_POLLING											    0x03
/* Offset definitions for global buffers */
#define CR95HF_COMMAND_OFFSET										        0x00
#define CR95HF_LENGTH_OFFSET										        0x01
#define CR95HF_DATA_OFFSET											        0x02


#define     drv_spi                 st95_SPI
#define     drvSpiSendRecvByte      nSpi->SendReceiveByte
#define     drvSpiSendRecvBuffer    nSpi->SendReceive
#define     drv_cs_low              nSpi->ChipSelet(DISABLE)
#define     drv_cs_high             nSpi->ChipSelet(ENABLE)


#define     CR95HF_FLAG_DATA_READY									        0x08
#define     CR95HF_FLAG_DATA_READY_MASK							            0xF8

// CR95HF polling status
#define     CR95HF_POLLING_CR95HF											0x00
#define     CR95HF_POLLING_TIMEOUT											0x01



int8_t halCheckSendRecv(uc8 *pCommand, uint8_t *pResponse) ;
void drvCr95hfFiedOn ( void );
void drvCr95hfFiedOff( void );

#endif



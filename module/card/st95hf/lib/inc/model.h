#include "stdint.h"
#include "lib_iso14443a.h"
#include "lib_iso14443bpcd.h"
#include "lib_iso18092pcd.h"
#include "AnyID_R322_ST95HF_HL.h"
//#include "apdu_phone.h"

#ifndef _Model_H
#define _Model_H

typedef enum {
	RFTRANS_95HF_INTERFACE_UART = 0,
	RFTRANS_95HF_INTERFACE_SPI,
	RFTRANS_95HF_INTERFACE_TWI
}RFTRANS_95HF_INTERFACE;

typedef enum {
	RFTRANS_95HF_SPI_POLLING = 0,
	RFTRANS_95HF_SPI_INTERRUPT,
}RFTRANS_95HF_SPI_MODE;



/**
 *	@brief  the RF transceiver can be configured as either a reader or a card emulator
 *	@brief  or as P2P device
 */
typedef enum {
	RFTRANS_95HF_MODE_UNKNOWN = 0,
	RFTRANS_95HF_MODE_READER ,
	RFTRANS_95HF_MODE_CARDEMULATOR ,
	RFTRANS_95HF_MODE_PASSIVEP2P ,
	RFTRANS_95HF_MODE_ACTIVEP2P ,
}RFTRANS_95HF_MODE;

/**
 *	@brief  the Rf transceiver supports the differrent protocols
 */
typedef enum {
	RFTRANS_95HF_PROTOCOL_UNKNOWN = 0,
	RFTRANS_95HF_PCD_14443A,
	RFTRANS_95HF_PCD_14443B,
	RFTRANS_95HF_PCD_15693,
	RFTRANS_95HF_PCD_18092,
	RFTRANS_95HF_PICC_14443A,
	RFTRANS_95HF_PICC_14443B,
	RFTRANS_95HF_PICC_15693,
	RFTRANS_95HF_PICC_18092,
}RFTRANS_95HF_PROTOCOL;

typedef enum {
    QIN = 0,
	QJA = 0x30,
	QJB,
	QJC,
	QJD,
	QJE
}IC_VERSION;


#endif

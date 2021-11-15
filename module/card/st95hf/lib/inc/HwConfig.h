#ifndef _HWCONFIG_H_
#define _HWCONFIG_H_

#include "HalNfcSpi.h"
#include "Drv95HF.h"
#include "unit.h"
#include "lib_95HF.h"
#include "lib_iso15693pcd.h"
#include "lib_iso14443Apcd.h"
#include "lib_iso14443Bpcd.h"
#include "lib_iso18092pcd.h"


/* Manager status and erroc code ----------------------------------------------------*/
#define MANAGER_SUCCESSCODE															RESULTOK
#define MANAGER_ERRORCODE_DEFAULT												0xF1
#define MANAGER_ERRORCODE_PORERROR											0xF2
#define MANAGER_ERRORCODE_COMMUNICATION_LOST						0xF3

/* Flags for PICC/PCD tracking  ----------------------------------------------------------*/
#define	TRACK_NOTHING		0x00
#define	TRACK_NFCTYPE1 		0x01 /* 0000 0001 */
#define	TRACK_NFCTYPE2 		0x02 /* 0000 0010 */
#define	TRACK_NFCTYPE3 		0x04 /* 0000 0100 */
#define	TRACK_NFCTYPE4A 	0x08 /* 0000 1000 */
#define	TRACK_NFCTYPE4B 	0x10 /* 0001 0000 */
#define	TRACK_NFCTYPE5 		0x20 /* 0010 0000 */
#define TRACK_ALL 			0xFF /* 1111 1111 */

/* Flags for Initiator/Target tracking  ------------------------------------------------------*/
#define	P2P_NOTHING				0x00
#define	INITIATOR_NFCA 		0x01 /* 0000 0001 */
#define	INITIATOR_NFCF 		0x02 /* 0000 0010 */
#define	TARGET_NFCA 			0x04 /* 0000 0100 */
#define	TARGET_NFCF 			0x08 /* 0000 1000 */
#define	TARGET_LLCPA 			0x10 /* 0000 0100 */
#define	TARGET_LLCPF 			0x20 /* 0000 1000 */
#define	INITIATOR_LLCPA 	0x40 /* 0000 0100 */
#define	INITIATOR_LLCPF 	0x80 /* 0000 1000 */
#define P2P_ALL 					0xFF /* 1111 1111 */

/* Flags for Proprietary P2P tracking  ------------------------------------------------------*/
#define	PP2P_NOTHING					0x00
#define	PP2P_INITIATOR_NFCA 	0x01 /* 0000 0001 */
#define	PP2P_TARGET_NFCA 		0x10 /* 0000 0100 */
#define PP2P_ALL 							0xFF /* 1111 1111 */

/* Flags for SelectMode  ----------------------------------------------------------*/
#define	SELECT_NOTHING		0x00
#define	SELECT_PCD			 	0x01 /* 0000 0001 */
#define	SELECT_PICC			 	0x02 /* 0000 0010 */
#define	SELECT_P2P			 	0x04 /* 0000 0100 */
#define SELECT_ALL 				0xFF /* 1111 1111 */

#define UID_SIZE                16
/* structure of the manager state --------------------------------------------------*/

#include "swipeTag.h"


void  st95hf_hal_init( void );
uint8_t TagHuntingResetM1( tagBufferType *tag);
uint8_t st95hf_verification( void );
#endif



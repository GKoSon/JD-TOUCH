#ifndef _DRV95HF_H_
#define	_DRV95HF_H_


#include "HalNfcSpi.h"
#include "model.h"
#include "modules_init.h"

#define	delay_us						nSpi->DelayUs
#define	delay_ms						nSpi->DelayMs
#define	delayHighPriority_ms			nSpi->DelayMs

//#define MAX(x,y) 				((x > y)? x : y)
//#define MIN(x,y) 				((x < y)? x : y)  
#define ABS(x) 					((x)>0 ? (x) : -(x))  
#define CHECKVAL(val, min,max) 	((val < min || val > max) ? false : true) 
	 
#define GETMSB(val) 		((val & 0xFF00 )>>8 ) 
#define GETLSB(val) 		( val & 0x00FF ) 
 
#define RESULTOK 							0x00 
#define ERRORCODE_GENERIC 		1 

#ifndef errchk
#define errchk(fCall) if (status = (fCall), status != RESULTOK) \
	{goto Error;} else
#endif
      
      
      

/* Enum for ST95 state */
typedef enum {UNDEFINED_MODE=0,PICC,PCD}DeviceMode_t;
typedef enum {UNDEFINED_TAG_TYPE=0,TT1,TT2,TT3,TT4A,TT4B,TT5,MIFARE,BUS_CARD}TagType_t;

/* TT1 */
#define NFCT1_MAX_TAGMEMORY																(120+2)

/* TT2 */
#define NFCT2_MAX_TAGMEMORY																512//2048 (must be a multiple of 8)
#define NFCT2_MAX_CC_SIZE																	4	
#define NFCT2_MAX_CONFIG																	12	
#define NFCT2_MAX_NDEFFILE																(NFCT2_MAX_TAGMEMORY-NFCT2_MAX_CC_SIZE-NFCT2_MAX_CONFIG)

/* TT3 */
#define NFCT3_ATTRIB_INFO_SIZE 														16
#define NFCT3_MAX_NDEFFILE																2048
#define NFCT3_MAX_TAGMEMORY 															(NFCT3_MAX_NDEFFILE+NFCT3_ATTRIB_INFO_SIZE)
#define NFCT3_NB_BLOC_MSB																	((NFCT3_MAX_TAGMEMORY/16)>>8)
#define NFCT3_NB_BLOC_LSB																	((NFCT3_MAX_TAGMEMORY/16)&0x00FF)

/* TT4 */
#define NFCT4_MAX_CCMEMORY																16
#define NFCT4A_MAX_NDEFMEMORY															8192
#define NFCT4B_MAX_NDEFMEMORY															8192
#define NFCT4_MAX_NDEFMEMORY															NFCT4A_MAX_NDEFMEMORY

/* TT5 */
#define NFCT5_MAX_TAGMEMORY																8192

#define NFC_DEVICE_MAX_NDEFMEMORY	


/* Regarding board antenna (and matching) appropriate value may be modified to optimized RF performances */
/*
 Analogue configuration register
 ARConfigB	bits 7:4	MOD_INDEX	Modulation index to modulator
								 3:0	RX_AMP_GAIN	Defines receiver amplifier gain
 uc16

 For type A you can also adjust the Timer Window
*/

/******************  PCD  ******************/
/* ISO14443A */
#define PCD_TYPEA_ARConfigA	0x01
#define PCD_TYPEA_ARConfigB	0xDF

#define PCD_TYPEA_TIMERW    0x5A

/* ISO14443B */
#define PCD_TYPEB_ARConfigA	0x01
#define PCD_TYPEB_ARConfigB	0x51

/* Felica */
#define PCD_TYPEF_ARConfigA	0x01
#define PCD_TYPEF_ARConfigB	0x51

/* ISO15693 */
#define PCD_TYPEV_ARConfigA	0x01
#define PCD_TYPEV_ARConfigB	0xD1

/******************  PICC  ******************/
/* ISO14443A */
#define PICC_TYPEA_ACConfigA 0x27  /* backscaterring */



/* RFtransceiver HEADER command definition ---------------------------------------------- */
#define RFTRANS_95HF_COMMAND_SEND               0x00
#define RFTRANS_95HF_COMMAND_RESET              0x01
#define RFTRANS_95HF_COMMAND_RECEIVE            0x02
#define RFTRANS_95HF_COMMAND_POLLING            0x03
#define RFTRANS_95HF_COMMAND_IDLE               0x07

/* RFtransceiver mask and data to check the data (SPI polling)--------------------------- */
#define RFTRANS_95HF_FLAG_DATA_READY            0x08
#define RFTRANS_95HF_FLAG_DATA_READY_MASK       0x08

/* RF transceiver status	--------------------------------------------------------------- */
#define RFTRANS_95HF_SUCCESS_CODE               RESULTOK
#define RFTRANS_95HF_NOREPLY_CODE               0x01
#define	RFTRANS_95HF_ERRORCODE_DEFAULT          0xFE
#define	RFTRANS_95HF_ERRORCODE_TIMEOUT          0xFD
#define RFTRANS_95HF_ERRORCODE_POR              0x44

/* RF transceiver polling status	------------------------------------------------------- */
#define RFTRANS_95HF_POLLING_RFTRANS_95HF       0x00
#define RFTRANS_95HF_POLLING_TIMEOUT            0x01

/* RF transceiver number of byte of the buffers------------------------------------------ */
#define RFTRANS_95HF_RESPONSEBUFFER_SIZE        0x20E
#define RFTRANS_95HF_MAX_BUFFER_SIZE            0x20E
#define RFTRANS_95HF_MAX_ECHO                   768		// Max of 256 (for QJC) and 768 (for QJE. Should be 512 but bug Design)

/* RF transceiver Offset of the command and the response -------------------------------- */
#define RFTRANS_95HF_COMMAND_OFFSET             0x00
#define RFTRANS_95HF_LENGTH_OFFSET              0x01
#define RFTRANS_95HF_DATA_OFFSET                0x02

/* ECHO response ------------------------------------------------------------------------ */
#define ECHORESPONSE                            0x55

/* Sleep parameters --------------------------------------------------------------------- */
#define IDLE_SLEEP_MODE                         0x00
#define IDLE_HIBERNATE_MODE                     0x01

#define IDLE_CMD_LENTH                          0x0E

#define WU_TIMEOUT                              0x01
#define WU_TAG                                  0x02
#define WU_FIELD                                0x04
#define WU_IRQ                                  0x08
#define WU_SPI                                  0x10

#define HIBERNATE_ENTER_CTRL                    0x0400
#define SLEEP_ENTER_CTRL                        0x0100
#define SLEEP_FIELD_ENTER_CTRL                  0x0142

#define HIBERNATE_WU_CTRL                       0x0400
#define SLEEP_WU_CTRL                           0x3800

#define LEAVE_CTRL                              0x1800

/* Calibration parameters---------------------------------------------------------------- */
#define WU_SOURCE_OFFSET						0x02
#define WU_PERIOD_OFFSET						0x09
#define DACDATAL_OFFSET							0x0C
#define DACDATAH_OFFSET							0x0D
#define NBTRIALS_OFFSET							0x0F

/* baud rate command -------------------------------------------------------------------- */
#define	BAUDRATE_LENGTH						  	0x01
#define	BAUDRATE_DATARATE_DEFAULT			  	57600

#define MAX_BUFFER_SIZE  						528 

#define ECHO									0x55
#define LISTEN									0x05
#define DUMMY_BYTE								0xFF


#define READERREPLY_MAX_BUFFER_SIZE			    0x40	


#define ST95HF_DUMMY_BYTE					    0xFF

//ST95HF Comm HEADER command definition
#define ST95HF_COMM_CMD_WRITE                       0x00
#define ST95HF_COMM_CMD_RESET                       0x01
#define ST95HF_COMM_CMD_READ                        0x02
#define ST95HF_COMM_CMD_POLLING                     0x03
#define ST95HF_COMM_CMD_LISTEN                      0x05
#define ST95HF_COMM_CMD_TRANS                       0x06
#define ST95HF_COMM_CMD_ACFILTER                    0x0D
#define ST95HF_COMM_CMD_ECHO                        0x55

//polling flag
#define ST95HF_POLLING_WRITE_FLAG                   0x04
#define ST95HF_POLLING_READ_FLAG                    0x08
#define ST95HF_POLLING_DELAY                        10000   //10ms


//ST95HF command definition
#define ST95HF_CMD_IDN                              0x01
#define ST95HF_CMD_SEL_PROTOCOL                     0x02
#define ST95HF_CMD_POLL_FIELD                       0x03
#define ST95HF_CMD_SEND_RECEIVE                     0x04
#define ST95HF_CMD_LISTEN                           0x05
#define ST95HF_CMD_SEND                             0x06
#define ST95HF_CMD_IDLE                             0x07
#define ST95HF_CMD_READ_REG                         0x08
#define ST95HF_CMD_WRITE_REG                        0x09
#define ST95HF_CMD_BAUD_RATE                        0x0A
#define ST95HF_CMD_SUB_FREQ_RES                     0x0B
#define ST95HF_CMD_AC_FILTER                        0x0D
#define ST95HF_CMD_TEST_MODE                        0x0E
#define ST95HF_CMD_SLEEP_MODE                       0x0F
#define ST95HF_CMD_ECHO                             0x55
#define ST95HF_CMD_ECHORESPONSE                     0x55      
      
#define ST95HF_ERR_SUCCESS                          0x00
#define ST95HF_ERR_RSPCODE                          0x06
#define ST95HF_ERR_TAG                              0x07
#define ST95HF_ERR_PARITY                           0x08
#define ST95HF_ERR_BCC                              0x09
#define ST95HF_ERR_ISO14443A4                       0x0A
#define ST95HF_ERR_NAK                              0x0B
#define ST95HF_ERR_AUTH                             0x0C
#define ST95HF_ERR_TRANS                            0x0D
#define ST95HF_ERR_COLL                             0x0F
#define ST95HF_ERR_OTHER                            0x10
#define ST95HF_ERR_TIMEOUT                          0x11
#define ST95HF_ERR_LENGTH                           0x12
#define ST95HF_ERR_CRC                              0x13
#define ST95HF_ERR_RFOFF                            0x14
      

//command resp code
#define ST95HF_RSP_CODE_OK                          0x00
//IDN
#define ST95HF_IDN_RSP_LEN                          0x0F
//send_receive命令的响应
#define ST95HF_SENDRCV_RSPCODE_ERR                  0x00
#define ST95HF_SENDRCV_RSPCODE_BIT                  0x90
#define ST95HF_SENDRCV_RSPCODE_DAT                  0x80
//15693返回标志
#define ST95HF_SENDRCV_15693_ERR_COL                0x01
#define ST95HF_SENDRCV_15693_ERR_CRC                0x02
//14443A请求标志
#define ST95HF_SENDRCV_14443A_FLG_TOPAZ             0x80
#define ST95HF_SENDRCV_14443A_FLG_SPLIT             0x40
#define ST95HF_SENDRCV_14443A_FLG_APPEND_CRC        0x20
#define ST95HF_SENDRCV_14443A_FLG_PARITY_MOD        0x10
#define ST95HF_SENDRCV_14443A_FLG_LSB_BIT1          0x01
#define ST95HF_SENDRCV_14443A_FLG_LSB_BIT2          0x02
#define ST95HF_SENDRCV_14443A_FLG_LSB_BIT3          0x03
#define ST95HF_SENDRCV_14443A_FLG_LSB_BIT4          0x04
#define ST95HF_SENDRCV_14443A_FLG_LSB_BIT5          0x05
#define ST95HF_SENDRCV_14443A_FLG_LSB_BIT6          0x06
#define ST95HF_SENDRCV_14443A_FLG_LSB_BIT7          0x07
#define ST95HF_SENDRCV_14443A_FLG_LSB_BIT8          0x08
//14443A返回标志
#define ST95HF_SENDRCV_14443A_RSPTRIL_LEN           0x03
#define ST95HF_SENDRCV_14443A_ERR_OK                0x00
#define ST95HF_SENDRCV_14443A_ERR_COL               0x80
#define ST95HF_SENDRCV_14443A_ERR_CRC               0x20
#define ST95HF_SENDRCV_14443A_ERR_PRITY             0x10
#define ST95HF_SENDRCV_14443A_ERR_BITMASK           0x07
//18092返回标志
#define ST95HF_SENDRCV_18092_ERR_CRC                0x02
//14443B返回标志
#define ST95HF_SENDRCV_14443B_ERR_CRC               0x02


//Listen response code
#define ST95HF_LISTEN_IVLD_LEN                      0x82
#define ST95HF_LISTEN_IVLD_PROTOCOL                 0x83
#define ST95HF_LISTEN_RF_OFF                        0x8F


//protocol allowed
#define ST95HF_PROTOCOL_FIELDOFF                    0x00
#define ST95HF_PROTOCOL_ISO15693                    0x01
#define ST95HF_PROTOCOL_ISO14443A                   0x02
#define ST95HF_PROTOCOL_ISO14443B                   0x03
#define ST95HF_PROTOCOL_ISO18092                    0x04

//14443a params
#define ST95HF_ISO14443A_TXRATE_106                 0x00
#define ST95HF_ISO14443A_TXRATE_212                 0x40
#define ST95HF_ISO14443A_RXRATE_106                 0x00
#define ST95HF_ISO14443A_RXRATE_212                 0x10

//14443b params
#define ST95HF_ISO14443B_TXRATE_106                 0x00
#define ST95HF_ISO14443B_TXRATE_212                 0x40
#define ST95HF_ISO14443B_TXRATE_424                 0x80
#define ST95HF_ISO14443B_TXRATE_848                 0xC0
#define ST95HF_ISO14443B_RXRATE_106                 0x00
#define ST95HF_ISO14443B_RXRATE_212                 0x10
#define ST95HF_ISO14443B_RXRATE_424                 0x20
#define ST95HF_ISO14443B_RXRATE_848                 0x30
#define ST95HF_ISO14443B_APPEND_CRC                 0x01

//18092 params
#define ST95HF_ISO18092_TXRATE_424                  0x80
#define ST95HF_ISO18092_TXRATE_212                  0x40
#define ST95HF_ISO18092_RXRATE_424                  0x20
#define ST95HF_ISO18092_RXRATE_212                  0x10
#define ST95HF_ISO18092_APPEND_CRC                  0x01
//slot1需要等待2.4ms；其他每增加一个slot，增加1.2ms
#define ST95HF_ISO18092_FWT_2P4MS                   0x00 //slot1
#define ST95HF_ISO18092_FWT_PPMM                    0x01 //
#define ST95HF_ISO18092_SLOT_MASK                   0x0F //1~16SLOT
#define ST95HF_ISO18092_SLOT_1                      0x00
#define ST95HF_ISO18092_SLOT_2                      0x01
#define ST95HF_ISO18092_SLOT_3                      0x02
#define ST95HF_ISO18092_SLOT_4                      0x03
#define ST95HF_ISO18092_SLOT_5                      0x04
#define ST95HF_ISO18092_SLOT_6                      0x05
#define ST95HF_ISO18092_SLOT_7                      0x06
#define ST95HF_ISO18092_SLOT_8                      0x07
#define ST95HF_ISO18092_SLOT_9                      0x08
#define ST95HF_ISO18092_SLOT_10                     0x09
#define ST95HF_ISO18092_SLOT_11                     0x0A
#define ST95HF_ISO18092_SLOT_12                     0x0B
#define ST95HF_ISO18092_SLOT_13                     0x0C
#define ST95HF_ISO18092_SLOT_14                     0x0D
#define ST95HF_ISO18092_SLOT_15                     0x0E
#define ST95HF_ISO18092_SLOT_16                     0x0F

//15693 protocol params
#define ST95HF_ISO15693_RATA_26                     0x00
#define ST95HF_ISO15693_RATA_52                     0x10
#define ST95HF_ISO15693_RATA_6                      0x20
#define ST95HF_ISO15693_DELAY_312US                 0x00
#define ST95HF_ISO15693_WAIT_SOF                    0x08
#define ST95HF_ISO15693_MODL_100                    0x00
#define ST95HF_ISO15693_MODL_10                     0x04
#define ST95HF_ISO15693_SINGLE_SUB                  0x00
#define ST95HF_ISO15693_DOUBLE_SUB                  0x02
#define ST95HF_ISO15693_NO_CRC                      0x00
#define ST95HF_ISO15693_APPEND_CRC                  0x01

//时间(单位:us)
#define ISO14443A_TO_TIME                   100000
#define ISO14443A_FDT_INV                   1000
#define ISO14443A_FDT_READ                  10000
#define ISO14443A_FDT_WRITE                 10000
//14443命令
#define ISO14443A_CMD_IDLE                  0x00    // IDLE command
#define ISO14443A_CMD_REQALL                0x52    // 唤醒命令
#define ISO14443A_CMD_REQIDL                0x26    // 请求命令
#define ISO14443A_CMD_SELECT0               0x93
#define ISO14443A_CMD_SELECT1               0x95
#define ISO14443A_CMD_SELECT2               0x97
#define ISO14443A_CMD_HALTA                 0x50   //将卡挂起
#define ISO14443A_CMD_AUTHENT_A             0x60   //AUTHENT A command
#define ISO14443A_CMD_AUTHENT_B             0x61   //AUTHENT B command
#define ISO14443A_CMD_READ                  0x30   //READ command
#define ISO14443A_CMD_WRITE16               0xA0   //WRITE 16 bytes command
#define ISO14443A_CMD_WRITE4                0xA2   //WRITE 4 bytes command
#define ISO14443A_CMD_INCREMENT             0xC1   //INCREMENT command,
                                                    //将block的Value模式数据加上命令帧中的数据
                                                    //然后将结果保存在标签内部的寄存器区
//读写命令的应答帧格式
#define ISO14443A_ACK_LEN                   0x04
#define ISO14443A_ACK_MASK                  0x0F
#define ISO14443A_ACK_OK                    0x0A
#define ISO14443A_ACK_NOT_ALLOW             0x04
#define ISO14443A_ACK_TRS_ERROR             0x05

#define ISO14443A_M1BLOCK_LEN               16
/** @addtogroup _95HF_Driver
 * 	@{
 */

/** @addtogroup drv_95HF
 * 	@{
 */




/**
 *	@brief  the different states of the RF transceiver
 */
typedef enum {
	RFTRANS_95HF_STATE_UNKNOWN = 0,
	RFTRANS_95HF_STATE_HIBERNATE ,
	RFTRANS_95HF_STATE_SLEEP,
	RFTRANS_95HF_STATE_POWERUP,
	RFTRANS_95HF_STATE_TAGDETECTOR,
	RFTRANS_95HF_STATE_READY,
	RFTRANS_95HF_STATE_READER,
	RFTRANS_95HF_STATE_TAGHUNTING,
}RFTRANS_95HF_STATE;
#if 0
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
#endif


/**
 *	@brief  structure to store driver information
 */
typedef struct {
	
	RFTRANS_95HF_INTERFACE 			uInterface;
	RFTRANS_95HF_STATE 			    uState;
	RFTRANS_95HF_MODE 				uMode;
	RFTRANS_95HF_SPI_MODE 			uSpiMode;
	RFTRANS_95HF_PROTOCOL			uCurrentProtocol;

}drv95HF_ConfigStruct;


	 
#define EXTI_RFTRANS_95HF_PIN        				GPIO_Pin_11
#define EXTI_GPIO_PORT       			      		GPIOB  
#define EXTI_RFTRANS_95HF_PIN_SOURCE        		GPIO_PinSource11
#define EXTI_RFTRANS_95HF_GPIO_CLOCK				RCC_APB2Periph_GPIOB
#define EXTI_RFTRANS_95HF_GPIO_PORT_SOURCE			GPIO_PortSourceGPIOB
#define EXTI_RFTRANS_95HF_LINE						EXTI_Line11
	
	
int8_t  drv95HF_SendReceive(uc8 *pCommand, uint8_t *pResponse);	
void drv95HF_ResetSPI ( void );
uint8_t drv95HF_GetSerialInterface ( void );
void drv95HF_SendIRQINPulse(void);
void Drv95HF_Init( void );
void drv95HF_SendSPICommand(uc8 *pData);
#endif



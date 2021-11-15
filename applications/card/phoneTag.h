#ifndef _PHONE_CARD_H_
#define _PHONE_CARD_H_


#define CMD_WRITE_MODE (0)
#define CMD_READ_MODE  (1)



typedef struct APUD_MEG
{
    uint8_t ucCmd;
    uint8_t ucType;
    uint8_t pucMsg[256];
    uint8_t ucLength;
}ApduRecvDataType;

typedef struct DATA_MSG
{
    uint8_t ucLength;
    uint8_t pucMsg[256];
}ApduDataMsgType;

typedef struct
{
	uint8_t DrvCommand;
    uint8_t Cla;
    uint8_t Ins;
    uint8_t P1;
    uint8_t P2;
}ApduAgreementHeardType;

typedef struct
{
    ApduAgreementHeardType   pHdr;
    //uint8_t                ucSize;
    uint8_t                ucCmd;
    uint8_t                ucType;
    uint8_t                ucDataLength;
    uint8_t                ucData[256];
}ApduType;


typedef enum
{
    SEND_MAC = 0,
    SNED_SEARCH_KEY,
    SEND_OPEN_SUCCESS,
    SEND_OPEN_FAIL,
    SEND_PASSWORD_ERR,
}ApduRunType;

#define READ_APDU_EVENT         0x0001
#define READ_APDU_GAP           50

#define READ_PHONE_ONCE         (1)

#define OFFSET_LENGTH			CR95HF_LENGTH_OFFSET
#define CRC_MASK				0x02
#define CRC_ERROR_CODE			0x02
#define COLISION_MASK			0x01
#define COLISION_ERROR_CODE		0x01

/*
#define CR95HF_HEARD_LEN        2
#define APUD_BIT_LEN            1  
#define APUD_CRC_LEN            2
#define CR95HF_END_LEN          3
#define NDEF_DATA_OFFSET_LEN    (APUD_BIT_LEN+APUD_CRC_LEN+CR95HF_END_LEN)
#define NDEF_DATA_OFFESET       APUD_BIT_LEN
*/

#define GAGENT_ERROR            0x1000
#define GAGENT_NOTIC            0X2000
#define LOG_LEVEL               (GAGENT_ERROR|GAGENT_NOTIC)

#define LOG_PROTOCOL            0X0001
#define LOG_SELECT_UUID         0X0002
#define LOG_SELECT_APP          0X0004
#define LOG_READ_MEG            0x0008
#define LOG_USE_KEY             0x0010
#define LOG_INFO                (LOG_SELECT_UUID|LOG_SELECT_APP|LOG_USE_KEY)


#define     SELECT_REPEAT_MAX   (5)


typedef enum
{
    APDU_INIT = 0 ,
    APDU_SUCCESS,
    SELECT_PROTOCOL_SUCCESS,
    SELECT_APP_SUCCESS,
    SEARCH_UUID_SUCCESS,
    READ_USER_SUCCESS,
    WRITE_MSG_SUCCESS,
    OPEN_SUCCESS,
    OPEN_SUCCESS_ERR,
    READ_USER_ERR,
    SELECT_APP_ERR,
    SEARCH_ERR,
    SELECT_ERR,
    SELECT_PROTOL_ERR,
    CONFIG_DEVICE_ERR,
    SET_GAIN_ERR,
    WRITE_MSG_ERR,
    OPEN_PASSWORD_ERR,
    OPEN_DOOR_ERR,
    APDU_DEAFULT = 0xFF,
}ApduResaultType;



#define OPEN_DOOR_CMD           "nfc_open"


#define     PHONE_DETECTION_FAIL        (0)
#define     PHONE_DETECTION_SUCCESS     (1)

#define     DETECTION_LEAVE_MAX         (1)

#define     DEVICE_MAC_SIZE             (6)
#define     RETURN_DATA_SIZE            (6)

//CMD MAX: application---->device
#define APP_TO_DEVICE           0x0F
//Type: application---->device
#define APDU_SELECT_AID         0x01
#define APDU_READY_HANDLE       0x02
#define APDU_OPEN_COMMAND       0x03
#define APDU_WAIT_HANDLE        0x04
#define APDU_NO_KEY             0x05
#define APDU_UNKNOW_COMMAND     0x06
#define APDU_CLOSE_COMMADN      0x07
#define APDU_REPEAT_MAC         0x08

//CMD : device---->application
#define APDU_NFC_OPEN           0x10
//Type: device---->application
#define APDU_SEND_MAC           0x01
#define APDU_SEARCH_KEY         0x02
#define APDU_SEARCH_LADDERKEY   0x03
#define APDU_OPEN_SUCCESS       0x04
#define APDU_OPEN_FAIL          0x05
#define APDU_PASSWORD_ERR       0x06


extern uint8_t gucWriteOrReadCmd;
extern uint8_t pucReturnApduData[6];
extern uint32_t gdOpenTimeFormPhone;
extern ApduAgreementHeardType gucWriteHeard;
extern ApduAgreementHeardType gucReadHeard;

uint8_t tag_read_phone_nfc( void );

#endif


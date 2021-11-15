#include "model.h"
#ifndef _MIFARE_H
#define _MIFARE_H
//#include "types.h"
#include "hwConfig.h"
#define ISO14443B_READ_TIME                 15000
#define ISO14443B_SIZE_UID                  8

typedef enum{
  Card_unKnow=0,
  Card_Mifare,//mifare?
  Card_ISO14443A_Type4,//ISO14443A Type4 ?
  Card_ISO15693A,//ISO15693 type A
  Card_ISO14443B,//ISO14443B???
  Card_Weixin,//??
  Card_Phone,//????
  Card_IDCard,//
  Card_CardMax //
}CardTagType;


#define ST95HF_FRAME_LEN        50

typedef union st95hfFrameFlag{
    uint8_t cmd; 
    uint8_t code;
}ST95HF_FRAMEFLAG;

typedef struct st95hfFrame{
    ST95HF_FRAMEFLAG parms;
    uint16_t len;
    uint8_t frame[ST95HF_FRAME_LEN];
    //uint32_t to;     //超时时间，单位us
}ST95HF_FRAME;

extern ISO14443A_MIFAREAUTH MifareCard;

unsigned char ISO14443B_GetUID(unsigned char *pUid);


//unsigned char Read14443A_Block(unsigned char* uR256,uint8_t *uRet );
unsigned char Write14443A_Block(uint8_t block, uint8_t* uW256 ,tagBufferType *tag);
unsigned char Read14443A_Block(uint8_t block, uint8_t* ubuff ,tagBufferType *tag);
unsigned char read_fukai_card_data(uint8_t* ubuff ,tagBufferType *tag );

unsigned char Reader_GetErrorType(unsigned char err);
unsigned char ReadMifareBlockKey11(unsigned char blockAddr,unsigned char* pR,unsigned char uLen ,uint8_t *TagUID );
unsigned char ReadMifareBlockKeyFF(unsigned char blockAddr,unsigned char* pR,unsigned char uLen ,uint8_t *TagUID);

uint8_t WriteMifareBlockKey11(uint8_t blockAddr,uint8_t* pW,uint8_t uLen ,uint8_t *TagUID);
uint8_t WriteMifareBlockKeyFF(uint8_t blockAddr ,uint8_t *TagUID );
extern unsigned char ReadMifareBlockKey_JTS(unsigned char blockAddr,unsigned char* pR,unsigned char uLen );

#endif

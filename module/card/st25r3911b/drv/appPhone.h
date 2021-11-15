#ifndef _APP_PHONE_H_
#define _APP_PHONE_H_

#include "phoneTag.h"

uint8_t apduSelectApp( void );

uint8_t st25WriteMessageToPhone(uint8_t ucCmd,uint8_t ucType,uint8_t *pucSendBUff ,uint8_t ucBuffLength, ApduRecvDataType *pucReadBuff);

#endif


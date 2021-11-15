#ifndef _TAG_COMPONENT_H_
#define _TAG_COMPONENT_H_

#include "stdint.h"
#include "swipeTag.h"
#include "phoneTag.h"


typedef struct
{
	void	(*init)						(void);
	uint8_t (*iso15693_get_uid)			(tagBufferType *card);
	uint8_t (*iso15693_read_data)		(uint8_t Address , uint8_t Length ,uint8_t *Respone);
	uint8_t (*iso14443a_get_uid)		(tagBufferType *card);
	uint8_t (*iso14443b_get_uid)		(tagBufferType *card);
	uint8_t (*read_m1_data)				(tagBufferType *card);
	uint8_t (*cqBus_read_data)			(uint8_t *data);
	uint8_t (*apdu_select_app)			(void );
	uint8_t (*write_and_read_app)		(uint8_t ucCmd,uint8_t ucType,uint8_t *pucSendBUff ,uint8_t ucBuffLength, ApduRecvDataType *pucReadBuff);
	uint8_t (*fm1208_read_data)			(uint8_t *keydata,uint8_t *readdata);
	void	(*turnOffField)				(tagBufferType *tag);
}tagObjType;


extern tagObjType	*tagComp;
extern void tag_component_init( void );
#endif


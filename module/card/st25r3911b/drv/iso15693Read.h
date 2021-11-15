#ifndef _ISO15693_READ_H_
#define _ISO15693_READ_H_


uint8_t st25ReadISO15693Data( uint8_t Address , uint8_t Length ,uint8_t *Respone);
uint8_t st25ReadISO15693TagUid(  tagBufferType *card );
uint8_t st25ReadIso15693Msg( tagBufferType *tag );

#endif

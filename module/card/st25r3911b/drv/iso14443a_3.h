#ifndef _ISO14443A_3_H_
#define _ISO14443A_3_H_

#include "swipeTag.h"


uint8_t iso14443a_read_uuid(  tagBufferType *card );
uint8_t st25WriteAndReadIso14443aData( uint8_t *data , uint8_t len , uint8_t *response , uint16_t *rxSize);

#endif
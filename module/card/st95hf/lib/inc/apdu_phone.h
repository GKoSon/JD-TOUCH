#ifndef __APDU_PHONE_H
#define __APDU_PHONE_H


#include "lib_iso14443A.h"
#include "swipeTag.h"



void hal_log(uint8_t *pdata);


#define apdu_log(level, fmt, ...)     log(level , fmt, ## __VA_ARGS__) 
//#define apdu_log(level, fmt, ...)  

#define system_delay            McuDelay1ms


#define ST95_HEARD_LEN        2
#define ST95_APUD_BIT_LEN     1  
#define ST95_APUD_CRC_LEN     2
#define ST95_END_LEN          3
#define ST95_NDEF_DATA_OFFSET_LEN    (ST95_APUD_BIT_LEN+ST95_APUD_CRC_LEN+ST95_END_LEN)
#define ST95_NDEF_DATA_OFFESET       (ST95_HEARD_LEN + ST95_APUD_BIT_LEN)



#endif


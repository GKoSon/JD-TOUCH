#ifndef _TSL_DATA_HANDLE_H_
#define _TSL_DATA_HANDLE_H_

#include "swipeTag.h"
#include "sysCfg.h"
#include "rtc.h"
#include "timer.h"

typedef struct 
{  
  uint8_t class;      
  uint8_t type;     
  uint8_t uid[4];       
  uint8_t ID[9];     
  uint8_t crc;    
}headtype;

typedef struct 
{  
  uint8_t group[11];    
  uint8_t endtime[3];     
  uint8_t crc;      
  uint8_t power;   
}bodytype;

typedef union 
{
    uint8_t  crc[2];
    uint16_t crc16;
}CRCtype;

typedef struct 
{
   headtype  head;
   bodytype  body[5];   
   CRCtype   crc;    
}shanghaicardtype;


uint8_t tag_shanghai_card_process( tagBufferType *tag);

#endif
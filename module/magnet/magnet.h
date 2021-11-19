#ifndef MAGNET_INPUT_H_
#define MAGNET_INPUT_H_

#include "stdint.h"


typedef enum
{
    STATUS_CLOSE = 0,
    STATUS_OPEN ,
}magnetStatusEnum;

extern uint8_t magnetAlarmStatus;
extern uint32_t alarmStartTimer;
void magnet_input_status_init( char save );


#endif



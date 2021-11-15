#ifndef _TIMER_H_
#define _TIMER_H_

#include <stdlib.h>
#include <string.h>
#include "tim.h"
#include "unit.h"
#include "component.h"

typedef void    (*time_call_back)(void);

typedef struct  _time
{
	void        *next;
	uint8_t     handle;
	uint8_t     start;
	uint32_t    cnt;
	uint32_t    time_out;
	void        (*fun)(void);
}time_type;

typedef struct
{
    uint8_t (*creat) (  uint32_t time_out ,uint8_t start, time_call_back call_back);
    uint8_t (*stop)  (  uint8_t handle);
    uint8_t (*start) (  uint8_t handle);
}time_ops_type;

extern time_ops_type   timer;


void timer_isr( void );


#endif



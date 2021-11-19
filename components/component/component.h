#ifndef _COMPONENT_H_
#define _COMPONENT_H_

#include <stdio.h>
#include <stdint.h>
#include "unit.h"


#define comp_log(level, fmt, ...)     log(level , fmt, ## __VA_ARGS__) 
//#define comp_log(level, fmt, ...)   


#pragma section ="__component_init" 


typedef struct _component_init {
    uint8_t     *name;
    void        (*fun)( void);
}component_init;

  
extern component_init  *__component_start;
extern component_init  *__component_end;


#define SELECT  @ "__component_init"//__attribute__ ((unused,section("u_boot_cmd")))

#define INIT_EXPORT(fun , name) \
__root component_init __component_init_##fun SELECT = {name ,fun}



void components_init( void );



#endif
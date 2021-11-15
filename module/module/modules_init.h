#ifndef _MODULES_INIT_H_
#define _MODULES_INIT_H_

#include <stdio.h>
#include <stdint.h>



#pragma section ="__modules_init" 


#define mdu_log(level, fmt, ...)     log(level , fmt, ## __VA_ARGS__)
//#define mdu_log(level, fmt, ...)



typedef struct _component_init_tbls
{
    uint8_t     *name;
	void        (*fun)( void);
}modules_init_tbls;

  
extern modules_init_tbls  *__modules_init_start;
extern modules_init_tbls  *__modules_init_end;


#define MODULES_SELECT  @ "__modules_init"//__attribute__ ((unused,section("u_boot_cmd")))

#define MODULES_INIT_EXPORT(fun , name) \
__root modules_init_tbls __modules_init_##fun MODULES_SELECT = {name ,fun}



void modules_init( void );



#endif

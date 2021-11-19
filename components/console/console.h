#ifndef _CONSOLE_H_
#define _CONSOLE_H_

#include <stdio.h>
#include <stdint.h>
#include "string.h"
#include "components_ins.h"

#pragma section ="u_boot_cmd"   

#define QUEUE_CONSOLE_PARAMETER     ( 0x22UL )

#define CFG_CBSIZE                    50        /* Console I/O Buffer Size    */
#define    CFG_MAXARGS                    16        /* max number of command args    */

#define QUEUE_CONSOLE_LENGTH        (3)

#define con_log(level, fmt, ...)     log(level , fmt, ## __VA_ARGS__) 
//#define con_log(level, fmt, ...)   



typedef struct
{
    uint8_t buff[CFG_CBSIZE];
    uint16_t cnt;    
}consoleBufferType;


struct cmd_tbl_s {
    char    *name;        /* Command Name            */
    int     maxargs;    /* maximum number of arguments    */
    int     repeatable;    /* autorepeat allowed?        */
                        /* Implementation function    */
    int     (*cmd)(struct cmd_tbl_s *, int, int, char *[]);
    char    *usage;        /* Usage message    (short)    */
    char    *help;        /* Help  message    (long)    */
};

typedef struct cmd_tbl_s    cmd_tbl_t;

extern cmd_tbl_t  *__u_boot_cmd_start;
extern cmd_tbl_t  *__u_boot_cmd_end;

cmd_tbl_t *find_cmd (const char *cmd);

#define SECTION  @ "u_boot_cmd"

#define U_BOOT_CMD(name,maxargs,rep,cmd,usage,help) \
__root cmd_tbl_t __u_boot_cmd_##name SECTION= {#name, maxargs, rep, cmd, usage, help}


extern SemaphoreHandle_t    xConsoleSemaphore;

void creat_console_task( void );
void serial_console_init( void );
#endif


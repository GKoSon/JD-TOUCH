#ifndef _BLEDATAHANDLE_H_
#define _BLEDATAHANDLE_H_
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include "bb0906_protocol.h"

#include "sysCfg.h"



typedef enum
{
    APP_OK = 0,
    APP_OK_NO_BEEP,
    APP_PWD_ERR,
    APP_ERR,
}BleAppDataErrType;



typedef struct _AppHandleArry
{
    uint8_t cmd;
    uint8_t (*EventHandlerFn)(BleProtData *pag);
}AppHandleArryType;


                              
void creat_buletooth_task( void );
void BleDataHandle(BleProtData *pag);
////////////////////////////////////////////////////                                          
                                          
#endif


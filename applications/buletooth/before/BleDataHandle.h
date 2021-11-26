#ifndef _BLEDATAHANDLE_H_
#define _BLEDATAHANDLE_H_
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include "bb0906_protocol.h"
#include "EncryDecry.h"
#include "sysCfg.h"
#include "twinkle.h"


typedef enum
{
    APP_OK = 0,
    APP_OK_NO_BEEP,
    APP_PWD_ERR,
    APP_ERR,
}BleAppDataErrType;



typedef struct _AppHandleArry
{
    uint16_t Rxid;
    uint16_t Txid;
    uint8_t (*EventHandlerFn)( ProtData_T *pag ,uint16_t ackid);
}AppHandleArryType;


                              
void creat_buletooth_task( void );
void BleDataProcess(ProtData_T *pag);

////////////////////////////////////////////////////                                          
                                          
#endif


#if (USE_NANO_PROTO==0)
#ifndef _BLEDATAHANDLE_H_
#define _BLEDATAHANDLE_H_
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include "bb0906_protocol.h"
#include "EncryDecry.h"
#include "sysCfg.h"
#include "twinkle.h"

#define BLE_DATA_DEBUG              (1)

#define uint16_t_CHANGE_BIT_MODE(x)     (((x>>8)&0x00ff)| ((x << 8)&0xff00))








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


typedef struct
{
    uint8_t type;
    uint8_t parm[BLEMODE_FARM_MAX+50];
    uint8_t length;
}TerminalMsgType;




#define QUEUE_TERMINAL_LENGTH           (5)

#define GET_BYTE(x,i)       (((x[i]<<4)&0xf0)|(x[i+1]&0x0f))
#define GET_STR(x)          (StrRefer[x])
#define GET_MSB_STR(x)      (StrRefer[((x>>4)&0x0f)])
#define GET_LSB_STR(x)      (StrRefer[(x&0x0f)])




extern uint8_t const    StrRefer[];

                                
void creat_buletooth_task( void );
void BleDataProcess(ProtData_T *pag);

////////////////////////////////////////////////////                                          
                                          
#endif
#endif //#if (USE_NANO_PROTO==0)

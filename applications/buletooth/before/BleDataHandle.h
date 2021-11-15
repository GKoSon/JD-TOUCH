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

#define BLE_A9A10
#define BLE_DATA_DEBUG              (1)

#ifdef BLE_A9A10
#define PAIR_PASSWORD_OFFSET        (2)
#define TYPE_LOW_OFFSET             (1)    
#else
#define PAIR_PASSWORD_OFFSET        (25)
#define TYPE_LOW_OFFSET             (47)  
#endif

#define uint16_t_CHANGE_BIT_MODE(x)     (((x>>8)&0x00ff)| ((x << 8)&0xff00))


typedef enum
{
    USE_USER_PRIV = 0,
    USE_PAIR_PRIV,
}UseCmdPrivType;


typedef enum
{
    APP_OK = 0,
    APP_OK_NO_BEEP,
    APP_LENGTH_ERR,
    APP_NO_DATA,
    APP_DECRY_ERR,
    APP_DECRY_CMD_ERR,
    APP_DECRY_PWD_ERR,
    APP_NO_COMMAND_ERR,
    APP_POWER_ERR,
    APP_MODIFY_PWD_ERR,
    APP_NO_POWER_ERR,
}BleAppDataErrType;



typedef struct
{   
    uint8_t    Active;
    uint8_t    PhoneType;
    uint8_t    TimeStamp[11];
    uint8_t    User[11];
}BtOpenUserType;

typedef struct _AppHandleArry
{
    uint8_t Id;
    uint8_t (*EventHandlerFn)(BleAppMsgType *pMsg);
}AppHandleArryType;


typedef struct _AppDataHeard
{
    uint8_t FormAddr[6];
    uint16_t Handle;
    uint8_t WriteType;
}AppDataHeardType;

typedef struct
{
    uint8_t type;
    uint8_t parm[BLEMODE_FARM_MAX+50];
    uint8_t length;
}TerminalMsgType;


#define NO_ADMIN            "failx"//非管理员
#define UN_SUPPORT          "failu"//不支持
#define ERR_PASSuint16_t    "fails"//密码错误
#define ERR_MAC             "faila"//Mac错误
#define ERR_LOCK            "failm"//门反锁
#define ERR_DISABLE         "faild"//钥匙被禁用
#define ERR_HANDSHAKE       "failh"//钥匙握手失败

#define QUEUE_TERMINAL_LENGTH           (5)
#define DEV_PARA_DATA_LENGTH            (592)
#define DEV_INSTALL_DATA_LENGTH         (592)
#define DEV_PARA_DATA_LENGTH            (592)

#define GET_BYTE(x,i)       (((x[i]<<4)&0xf0)|(x[i+1]&0x0f))
#define GET_STR(x)          (StrRefer[x])
#define GET_MSB_STR(x)      (StrRefer[((x>>4)&0x0f)])
#define GET_LSB_STR(x)      (StrRefer[(x&0x0f)])

#define OPEN_RECORD_MAX                 (127)

//#define ADDR_RETURN_PEIDUN_START        0x001FD000                      //return_peidun  56byte    LIB 使用FLASH起始位置  使用 2个块
#define ADD_BT_START                        0x001FE000
#define ADD_OPEN_RECORD_POS                 ADD_BT_START
#define ADD_OPEN_RECORD_NUM                 ADD_OPEN_RECORD_POS+4 
#define ADD_OPEN_BLE_RECORD                 ADD_OPEN_RECORD_NUM+4 
#define ADD_USER_BLE_RECORD                 ADD_OPEN_RECORD+32*OPEN_RECORD_MAX 

  
extern blinkParmType    checkErr;
extern uint8_t const    StrRefer[];
extern BleUserMsgType   BleUserMsg;
extern uint8_t          btPhone[20];


                                          
void creat_buletooth_task( void );
uint8_t BleDataProcess(BleAppMsgType *msg);

////////////////////////////////////////////////////                                          
                                          
#endif
#endif //#if (USE_NANO_PROTO==0)

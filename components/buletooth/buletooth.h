#ifndef _BULETOOTH_H_
#define _BULETOOTH_H_

#include <stdio.h>
#include <stdint.h>
#include "unit.h"
#include "bsp.h"
#include "cmsis_os.h"
#include "modules_init.h"
#include "sysCfg.h"
#include "crc16.h"


#define BLEMODE_FARM_MAX                (700)
#define BLE_CONNECT_MAX_NUM             (2)  


//UART
typedef enum{
    BLE_OK = 0 ,
    BLE_INIT_ERR , 
    BLE_WORK_ERR,
    BLE_SEND_LENGTH_ERR,
    BLE_RECV_LENGTH_ERR,
    BLE_RECV_ACTIVE_ERR,
    BLE_RECV_TIMEOUT_ERR,
    BLE_COMMAND_NOBACK_ERR,
    BLE_NO_COMMAND_ERR,
    BLE_SET_BEACON_ERR,
    BLE_OPEN_BEACON_ERR,
    BLE_SET_BEACON_SWITCH_TIME_ERR,
    BLE_ANALYSIS_ERR,
    BLE_ANALYSIS_DATE_NULL_ERR,
    BLE_ANALYSIS_CALSS_ERR,
    BLE_ANALYSIS_CRC_ERR,
    BLE_APP_LENGTH_ERR,
}BleFuncsErrType;


typedef struct  _BleModuleAppMsgType
{
    uint8_t FormAddr[6];
    uint16_t Handle;
    uint8_t WriteType;
    uint8_t Data[BLEMODE_FARM_MAX];
    uint8_t DatLength;
}BleModuleAppMsgType;


typedef struct  _BleModuleAppData
{
    uint8_t                 Class[4];
    uint16_t                Command;
    uint8_t                 Response;
    uint16_t                Length;
    BleModuleAppMsgType     Msg;
    uint8_t                 Crc;
    uint8_t                 cnt;
    uint8_t                 BytePos;
}BleModuleAppDateType;



//////////USER////////
typedef struct  _BleAppMsgType
{

    uint8_t  FormAddr[6];
    uint16_t Handle;
    uint8_t  WriteType;
    uint8_t  Data[BLEMODE_FARM_MAX];
    uint16_t DataLength;
    uint8_t Flag;///BleModuleAppMsgType ADD THIS
}BleAppMsgType;


typedef struct _BleUserMsgType
{
    BleAppMsgType     Msg[BLE_CONNECT_MAX_NUM];
    uint32_t          Length;
}BleUserMsgType;




typedef struct
{

    void        (*resert)              (void);
    uint8_t     (*read_mac)            (uint8_t *mac);
    uint8_t     (*read_ver)            (uint8_t *version);
    uint8_t     (*send)                (uint8_t *ptAddr , uint8_t *ptSenddata , uint16_t uLegnth , uint16_t dHandle);
    uint8_t     (*set_default)         (void);

}btDrvType;

typedef struct
{
    void        (*init)                (void);
    void        (*send)                (BleAppMsgType *nanopb , uint8_t *data , uint16_t length  );
    uint8_t     (*read_mac)            (uint8_t *mac);
    uint32_t    (*read_version)        (void);
    void        (*set_default)         (void);
    void        (*resert)              (void);
    void        (*timer_isr)           (void);
}buletoothDriveType;

extern buletoothDriveType btModule;
extern void bluetooth_drv_init( void );
#endif


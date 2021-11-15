
#ifndef _BLE_MODULE_DRV_H_
#define _BLE_MODULE_DRV_H_

#include <stdio.h>
#include <stdint.h>
#include "unit.h"
#include "bsp.h"
#include "cmsis_os.h"
#include "modules_init.h"
#include "sysCfg.h"
#include "crc16.h"
#include "buletooth.h"




#define BEACON_DEFAULT_PREFIX_LENG      (4)
#define BEACON_DEFAULT_UUID_LENG        (16)

#define GET_16BIT_DATA( data , i )      (data[i] << 8)| (data[i+1])

#define MSB(x)                          ((x)>>8) 
#define LSB(x)                          ((x)&0xff) 


#define BLE_MODULE_CALSEE_LENG          (4)


#define BLE_HANDLE_LEN                  (2)
#define BLE_WRITE_TYPE_LEN              (1)
#define BLE_DATA_HEARD_LEN              (BLE_ADDR_LENGTH+BLE_HANDLE_LEN+BLE_WRITE_TYPE_LEN)

#define BLE_CALSEE_LENG                 (4)
#define BLE_COMMAND_LENG                (2)
#define BLE_RESPONSE_LENG               (1)
#define BLE_LENG_LENG                   (2)
#define BLE_CRC_LENG                    (1)

#define BLE_HEARD_SIZE                  (BLE_CALSEE_LENG+BLE_COMMAND_LENG+BLE_RESPONSE_LENG+BLE_LENG_LENG+BLE_CRC_LENG)              






/*设备和模组命令*/  
typedef enum 
{  
    CMD_TEST            = 0,  /*（0x00）设备测试*/  
    CMD_VERSION         = 1,  /*（0x01）获取软件版本*/  
    CMD_RESET           = 2,  /*（0x02）模块复位*/  
    CMD_ORGL            = 3,  /*（0x03）恢复出厂状态(清除各种设置)*/  
    CMD_STATE           = 11,  /*（0x0b）获取蓝牙工作状态*/  
    CMD_DISCOVERABLE    = 13,  /*（0x0d）可发现和广播状态控制*/  
    CMD_READADDR        = 20,  /*（0x14）读取蓝牙模组的地址*/  
    CMD_REENDISCOVERABLE= 22,  /*（0x16）控制断开后是否自动进入可发现状态*/ 
    CMD_LEADVPARAMS     = 23,  /*（0x17）设置LE广播间隔时间*///(GATT协议专用)  
    CMD_RSSI            = 24,  /*（0x18）获取连接信号强度*/  
    CMD_LECONPARAMS     = 25,  /*（0x19）设置LE连接间隔时间*///(GATT协议专用)  
    CMD_UARTBAUD        = 26,  /*（0x1a）设置UART波特率*/  
    CMD_RENAME          = 27,  /*（0x1b）修改设备名*/  
    CMD_MODBTADDR       = 28,  /*（0x1c）修改模组蓝牙地址*/ 
    CMD_POLL_TIME       = 33,  /*（0x21）报告发送间隔时间(ms)*/ 
    CMD_MOD_IF_CHECKSUM = 38,  /*（0x26）控制是否使用checksum*/ 
    CMD_IAP2_START      = 42,  //（0x2a）用于IAP2 
    CMD_IAP2_FILE       = 44,  //（0x2c）用于IAP2 
    CMD_IAP2_FINISH     = 46,  //（0x2e）用于IAP2 
    CMD_SETUP_BEACON    = 51,     /*设置Beacon参数（只在uart上有效）*/ 
    CMD_SET_GPIO        = 52,  /*设置gpio电平*/ 
    CMD_ADV_DUAL        = 54,  /*设置是否让android识别为双模模组*/ 
    CMD_TESTMODE        = 62,  /*进入测试模式*/ 
    CMD_OTAMODE         = 65,  /*进入OTA模式*/   //   
    CMD_LE_SET_SCAN     = 66,     /*LE使能扫描周围设备*///(GATT协议主机专用)   
    CMD_LE_CONNECT      = 67,  /*LE连接特定地址设备*///(GATT协议主机专用)   
    CMD_LE_EXIT_CONNECT = 68,  /*LE退出连接特定地址设备*///(GATT协议主机专用)   
    CMD_LE_DISCONNECT   = 69,  /*LE断开特定设备连接*///(GATT协议主机专用)   
    CMD_LE_FIND_SERVICES= 70,  /*LE查找服务*///(GATT协议主机专用)   
    CMD_LE_FIND_CHARA   = 71,  /*LE查找属性*///(GATT协议主机专用)   
    CMD_LE_FIND_DESC    = 72,  /*LE查找描述*///(GATT协议主机专用)   
    CMD_LE_SET_NOTIFY   = 73,  /*LE设置使能从机的通知功能*///(GATT协议主机专用)   
    CMD_LE_XFER2S       = 74,  /*LE作为主机角色收发数据,接收从机通知数据*///(GATT协议主机专用) 
    CMD_LE_READ         = 75,  /*LE作为主机角色读属性*///(GATT协议主机专用) 
    CMD_LE_REMOTEN      = 76,    /*LE查询当前连接的设备*///(GATT协议专用)
    CMD_LE_XFER2H       = 77,   /*LE作为从机角色收发数据,接收主机写数据*///(GATT协议从机专用) 
    CMD_LE_CONNSTATE    = 78,    /*LE查询多连接状态*/ 
    CMD_IAPMODE         = 79,    /*进入IAP模式*/   
    CMD_BEACON_ONOFF    = 82,    /*BEACON开关*/ 
    CMD_ADV_SWITCHTIME  = 83,    /*BEACON和ADV切换的间隔时间*/ 
    CMD_ATTR_INDEX      = 84,   /*设置运行模式*/
    //
    CMD_READ_NAME       = 0x801B, /*读设备名*/ 
    CMD_MODIFY_COMPUTE_ID=0x8080, 
    CMD_UNKNOW          = 0xFFFF,  /*未知命令*/ 
} Command_t;

typedef enum {  
    BLE_CMD_TYPE1 = 'i', /*命令类型*/ 
    BLE_CMD_TYPE2 = 't', /*命令类型*/  
    BLE_CMD_TYPE3 = 'c', /*命令类型*/  
    BLE_CMD_TYPE4 = 'z', /*命令类型*/  
    BLE_CMD_UNKNOW_TYPE = 0xff, 
}CommandClassType;

/**********************************
**Ble get data mode , command or data.
*/
typedef enum
{
    BLE_DATA_MODE = 0 , 
    BLE_CMD_MODE,
}bleReceiveModeEnum;


typedef enum
{
    BLE_MODULE_HEARD1_POS = 0,
    BLE_MODULE_HEARD2_POS,
    BLE_MODULE_HEARD3_POS,
    BLE_MODULE_HEARD4_POS,
    BLE_MODULE_COMMAND_H_POS,
    BLE_MODULE_COMMAND_L_POS,
    BLE_MODULE_RESPONSE_POS,
    BLE_MODULE_LENGTH_H_POS,
    BLE_MODULE_LENGTH_L_POS,
    BLE_MODULE_ADDR_POS,
    BLE_MODULE_HANDLE_H_POS,
    BLE_MODULE_HANDLE_L_POS,
    BLE_MODULE_WRITE_TYPE_POS,
    BLE_MODULE_DATA_POS,
    BLE_MODULE_CRC_POS,
}bleModuleCmdPosEnum;
    
typedef enum
{
    BLE_PROTO_HEARDH_POS,
    BLE_PROTO_HEARDL_POS,
    BLE_PROTO_LENGTH_POS,
    BLE_PROTO_TYPE_POS,
    BLE_PROTO_DATA_POS,
    BLE_PROTO_CRCH_POS,
    BLE_PROTO_CRCL_POS,
}bleFrameProtocolPosEnum;



typedef enum
{
    DEFAULT_MODE  = 0,
    NORMAL_WECAT_MODE,
    NORMAL_MODE,
    SINGAL_MODE,
    SINGAL_WECAT_MODE,
}BleRunModeEnum;


typedef struct  _bleModuleReceiveData
{
    uint8_t     class[4];
    uint16_t    command;
    uint8_t     response;
    uint16_t    length;
    uint8_t     formAddr[6];
    uint8_t     crc;
    uint8_t     cnt;
    uint8_t     pos;
}bleModuleReceiveDataType;

typedef struct  _bleModuleReceiveCmd
{
    uint8_t     class[4];
    uint16_t    command;
    uint8_t     response;
    uint16_t    length;
    uint8_t     data[BLEMODE_FARM_MAX];
    uint8_t     crc;
    uint8_t     cnt;
    uint8_t     pos;
    uint8_t     flag;
}bleModuleReceiveCmdType;


typedef struct _bleCmdHandleArry
{
    uint8_t Id;
    uint8_t (*EventHandlerFn)( void *pucData ,void *pucParma);
}bleCmdHandleArryType;


extern QueueHandle_t                xBtOperationQueue;

extern bleReceiveModeEnum           ble_receive_mode;
extern bleModuleReceiveCmdType      bleModuleReceiveCmd;


void ble_cleal_timer( void );
void bb0906_init( void );
extern btDrvType	BB0906Drv;
#endif


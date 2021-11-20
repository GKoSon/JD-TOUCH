#ifndef _SYS_CFG_H_
#define _SYS_CFG_H_


#include <stdio.h>
#include <string.h>
#include <stdint.h>


/*



/* 
 *  软件版本号，APP需要显示104，Server需要显示1.0.4
 */
#define     DEVICE_SW_VERSION         100

/* 
 *  设备类型  ， 用于识别设备大类：具体见excel , TYPE=6为门禁设备
 */
#define    DEVICE_TYPE                0x06

/* 
 *  设备编号， 具体设备的详细编号 
 */
#define DEVICE_NUM                      217  

/* 
 *  设备蓝牙名称，设备名称编码4个ASCII+设备MAC地址
 *  注意，需要在结尾添加'\0' ，不然sprintf无法识别字符串结尾
 *  会造成读取越界
 */

#define    DEVICE_NAME            "CCYA" 


//0----门口机   1----围墙机
#define        UNIT_LOCK_MODE            0
#define        BULID_LOCK_MODE            1
#define        VILLAGE_LOCK_MODE        2


#define        WEIQIANGJI                2        /*只需要VILLAGE判断*/

#define        DEVICD_MODE                WEIQIANGJI



#define       DSYS_CFG_ADDR         0x0807F800


   
//平台地址
#define        NET_IP                  "139.9.66.72"
#define        MQTT_PORT                1883
#define        HTTP_PORT                0
#define        OTA_PORT                 0
#define     DSYS_DIANMA_ADDR        0x0807E000



#define     BLE_MODULE_NAME_SIZE    (strlen(BLE_MODULE_NAME)+1)
#define     BLE_RUN_MODE            NORMAL_WECAT_MODE


#define     BLE_ADMIN_LENGTH        (24)
#define     BLE_PASSWORD_LENGTH     (3)
#define     BLE_NAME_LENGTH         (10)
#define     BLE_VERSION_LENGTH      (3)
#define     BLE_MAC_LENGTH          (6)
#define     BLE_ADDR_LENGTH         (6)
#define     COMMUNITIES_NUM         (4)


#define     DEVICE_CHIP_LENGTH      (12)

#define     DEVICE_NAME_LENG        (16+1)


#define SYS_BLE_RESTORE_BIT                 0x0001
#define SYS_NET_RESTORE_BIT                 0x0002






typedef enum
{
    CFG_DEV_USED,
    CFG_DEV_LEVEL,//NO
    CFG_HTTP_ADDR,
    CFG_PAIR_PWD,
    CFG_USER_PWD,
    CFG_BLE_ADMIN,//NO
    CFG_BLE_RSSI,//NO
    CFG_BLE_MAC,
    CFG_BLE_VERSION,

    CFG_SYS_OPEN_TIME,
    CFG_SYS_CHIP_ID,
    CFG_SYS_HW_VERSION,  
    CFG_SYS_DEVICE_TYPE,
    CFG_SYS_DEVICE_NUM,
    CFG_SYS_DEVICE_NAME,
    CFG_SYS_SW_VERSION,
    CFG_SYS_OTA_VERSION,
    CFG_SYS_NET_TYPE,
    CFG_SYS_BLE_TYPE,
    CFG_SYS_DEVICE_FUN,
    CFG_SYS_ALARM_TIME,
    CFG_SYS_LOCK_MODE,


    CFG_SYS_MAGNET_STATUS,
    CFG_NET_ADDR,
    CFG_OTA_URL,
    CFG_OTA_PORT,
    CFG_OTA_CONFIG,
    CFG_WIFI_INFO,

    CFG_MQTT_CLIENTID,
    CFG_MQTT_USERNAME,
    CFG_MQTT_USERPWD,
    CFG_MQTT_MAC,
    CFG_PRO_PWD,

    CFG_SET_RESTORE,
    CFG_SYS_UPDATA_TIME,
    CFG_SET_RESTORE_FLAG,
    CFG_CLEAR_RESTORE_FLAG,
    CFG_SYS_SHANGHAI,
    CFG_NOTHING,
}systemConfigEnum;


typedef struct
{
    uint8_t     name[BLE_NAME_LENGTH];
    uint8_t     iBeacon_rssi;
    uint8_t     ble_mac[BLE_MAC_LENGTH];
    uint32_t    ble_version;
    uint8_t     ble_admin_num[BLE_ADMIN_LENGTH];
  
}SystemBleInfoType;






typedef struct
{
      uint8_t support_net_types;//入网方式  TSLNetType_TSLGPRS 3种
      uint8_t support_ble_types;//蓝牙 0--BM77 1--0906
      uint8_t lock_mode;//0 1 2 三种 标识安装在哪儿 涉及开门权限
      uint8_t device_type;//写死的 6 设备类型 没有意义
      uint8_t deviceNum;    //写死的217 设备编号 没有意义 BLE给安装工的
      
      uint8_t  alarm_time; //开门
      uint16_t delay_time;//开门
      uint8_t  magnet_status;
      uint8_t  mqttdone; /*1--老设备*/
      uint8_t  chipId[DEVICE_CHIP_LENGTH];
      uint8_t  deviceName[DEVICE_NAME_LENG];
      uint8_t  updataTime;
      uint16_t soft_version;
}SystemParmType;

typedef struct
{
    uint32_t        otaUpgMark;
    uint32_t        ver;
    uint32_t        fileSize;
    uint32_t        crc32;
}otaType;

typedef struct
{
    uint8_t ip[50];
    uint16_t port;
}serverAddrType;
typedef struct
{
    serverAddrType    net;
    uint16_t        httpport;
    uint16_t        otaport;
    char           otaurl[64];
}netAttriType;

typedef struct
{
    uint8_t ssid[50];
    uint8_t pwd[32];
}wifiApInfoType;


typedef struct
{
    char mqttClientId[32+1];
    char mqttUserName[4+1];
    char mqttUserPwd[32+1];
}MqttLoginInfoType;


typedef struct
{
  uint32_t                mark;
  uint8_t                 level;
  SystemBleInfoType       ble;
  SystemParmType          parm;
  otaType                  otaVar;
  netAttriType              server;
  wifiApInfoType          wifi;
  MqttLoginInfoType       mqtt;
  uint32_t                sysRestoreFlag; 
  uint8_t                 pair_pwd[BLE_PASSWORD_LENGTH];
  uint8_t                 user_pwd[BLE_PASSWORD_LENGTH];

  uint16_t                crc16;   
}SystemConfigType;



typedef struct
{
    uint8_t                 pair_pwd[BLE_PASSWORD_LENGTH];
    uint8_t                 user_pwd[BLE_PASSWORD_LENGTH];

    uint16_t                crc;
}SystemInfoType;//已经放弃





typedef struct _cfgTask
{
    uint8_t  (*write)   ( uint8_t mode , void *parma , uint8_t earseFlag);
    uint32_t (*read)    ( uint8_t mode , void **parma);
}cfgTaskType;

void sysCfg_init( void );
void sysCfg_print( void );
extern cfgTaskType    config;
#define GUPMAX 10
typedef struct
{
    uint64_t     ver;
    uint8_t      cnt;
    uint8_t      code[GUPMAX][11];
}GupType;

typedef struct
{
    uint8_t      codedev[11];
    uint8_t      codelocation[11];

    GupType gup;
}_SHType;
#endif


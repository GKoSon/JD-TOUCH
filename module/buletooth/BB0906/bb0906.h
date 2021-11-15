
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






/*�豸��ģ������*/  
typedef enum 
{  
    CMD_TEST            = 0,  /*��0x00���豸����*/  
    CMD_VERSION         = 1,  /*��0x01����ȡ����汾*/  
    CMD_RESET           = 2,  /*��0x02��ģ�鸴λ*/  
    CMD_ORGL            = 3,  /*��0x03���ָ�����״̬(�����������)*/  
    CMD_STATE           = 11,  /*��0x0b����ȡ��������״̬*/  
    CMD_DISCOVERABLE    = 13,  /*��0x0d���ɷ��ֺ͹㲥״̬����*/  
    CMD_READADDR        = 20,  /*��0x14����ȡ����ģ��ĵ�ַ*/  
    CMD_REENDISCOVERABLE= 22,  /*��0x16�����ƶϿ����Ƿ��Զ�����ɷ���״̬*/ 
    CMD_LEADVPARAMS     = 23,  /*��0x17������LE�㲥���ʱ��*///(GATTЭ��ר��)  
    CMD_RSSI            = 24,  /*��0x18����ȡ�����ź�ǿ��*/  
    CMD_LECONPARAMS     = 25,  /*��0x19������LE���Ӽ��ʱ��*///(GATTЭ��ר��)  
    CMD_UARTBAUD        = 26,  /*��0x1a������UART������*/  
    CMD_RENAME          = 27,  /*��0x1b���޸��豸��*/  
    CMD_MODBTADDR       = 28,  /*��0x1c���޸�ģ��������ַ*/ 
    CMD_POLL_TIME       = 33,  /*��0x21�����淢�ͼ��ʱ��(ms)*/ 
    CMD_MOD_IF_CHECKSUM = 38,  /*��0x26�������Ƿ�ʹ��checksum*/ 
    CMD_IAP2_START      = 42,  //��0x2a������IAP2 
    CMD_IAP2_FILE       = 44,  //��0x2c������IAP2 
    CMD_IAP2_FINISH     = 46,  //��0x2e������IAP2 
    CMD_SETUP_BEACON    = 51,     /*����Beacon������ֻ��uart����Ч��*/ 
    CMD_SET_GPIO        = 52,  /*����gpio��ƽ*/ 
    CMD_ADV_DUAL        = 54,  /*�����Ƿ���androidʶ��Ϊ˫ģģ��*/ 
    CMD_TESTMODE        = 62,  /*�������ģʽ*/ 
    CMD_OTAMODE         = 65,  /*����OTAģʽ*/   //   
    CMD_LE_SET_SCAN     = 66,     /*LEʹ��ɨ����Χ�豸*///(GATTЭ������ר��)   
    CMD_LE_CONNECT      = 67,  /*LE�����ض���ַ�豸*///(GATTЭ������ר��)   
    CMD_LE_EXIT_CONNECT = 68,  /*LE�˳������ض���ַ�豸*///(GATTЭ������ר��)   
    CMD_LE_DISCONNECT   = 69,  /*LE�Ͽ��ض��豸����*///(GATTЭ������ר��)   
    CMD_LE_FIND_SERVICES= 70,  /*LE���ҷ���*///(GATTЭ������ר��)   
    CMD_LE_FIND_CHARA   = 71,  /*LE��������*///(GATTЭ������ר��)   
    CMD_LE_FIND_DESC    = 72,  /*LE��������*///(GATTЭ������ר��)   
    CMD_LE_SET_NOTIFY   = 73,  /*LE����ʹ�ܴӻ���֪ͨ����*///(GATTЭ������ר��)   
    CMD_LE_XFER2S       = 74,  /*LE��Ϊ������ɫ�շ�����,���մӻ�֪ͨ����*///(GATTЭ������ר��) 
    CMD_LE_READ         = 75,  /*LE��Ϊ������ɫ������*///(GATTЭ������ר��) 
    CMD_LE_REMOTEN      = 76,    /*LE��ѯ��ǰ���ӵ��豸*///(GATTЭ��ר��)
    CMD_LE_XFER2H       = 77,   /*LE��Ϊ�ӻ���ɫ�շ�����,��������д����*///(GATTЭ��ӻ�ר��) 
    CMD_LE_CONNSTATE    = 78,    /*LE��ѯ������״̬*/ 
    CMD_IAPMODE         = 79,    /*����IAPģʽ*/   
    CMD_BEACON_ONOFF    = 82,    /*BEACON����*/ 
    CMD_ADV_SWITCHTIME  = 83,    /*BEACON��ADV�л��ļ��ʱ��*/ 
    CMD_ATTR_INDEX      = 84,   /*��������ģʽ*/
    //
    CMD_READ_NAME       = 0x801B, /*���豸��*/ 
    CMD_MODIFY_COMPUTE_ID=0x8080, 
    CMD_UNKNOW          = 0xFFFF,  /*δ֪����*/ 
} Command_t;

typedef enum {  
    BLE_CMD_TYPE1 = 'i', /*��������*/ 
    BLE_CMD_TYPE2 = 't', /*��������*/  
    BLE_CMD_TYPE3 = 'c', /*��������*/  
    BLE_CMD_TYPE4 = 'z', /*��������*/  
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


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


#define BLEMODE_FARM_MAX                (235)
#define BLE_CONNECT_MAX_NUM             (2)  



void Clear_ProtBuf(void);
void release_sig(void);



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


__packed  typedef struct _AppDataHeard
{
    uint8_t FormAddr[6];
    uint16_t Handle;
    uint8_t  WriteType;
}AppDataHeardType;

__packed  typedef struct  _BleModuleAppMsgType
{
    AppDataHeardType hdr;
    uint8_t  Data[40];
    uint16_t DatLength;
}BleModuleAppMsgType;


__packed  typedef struct  _BleModuleAppData
{
    uint16_t                Command;
    uint8_t                 Response;
    uint16_t                Length;
    BleModuleAppMsgType     Msg;
    uint8_t                 BytePos;
}BleModuleAppDateType;

    

typedef struct
{

    void        (*resert)              (void);
    uint8_t     (*read_mac)            (uint8_t *mac);
    uint8_t     (*read_ver)            (uint8_t *version);
    uint8_t     (*send)                (uint8_t *ptAddr , uint8_t *ptSenddata , uint8_t uLegnth , uint16_t dHandle);
    uint8_t     (*set_default)         (void);

}btDrvType;


__packed typedef union 
{
    struct 
    {
      uint8_t    msgid:4; 
      uint8_t    encode:1;
      uint8_t    version:3;
    } byte;
    uint8_t data;
}Byte0;

__packed typedef union 
{
    struct 
    {
      uint8_t    seqid:4; 
      uint8_t    seqall:4;
    } byte;
    uint8_t data;
}Byte2;

__packed  typedef struct _BleProtData
{
      Byte0    id;/*ͷ����һ���ֽ� ��ǰֻ���msgid���豸����ʱ��ά��һ����*/
      uint8_t  cmd;/*ͷ���ڶ����ֽ� ��ʾִ�е�����豸����ʱ��0X01����쳣��0X0F��*/
      Byte2    num;/*ͷ���������ֽ� ��ʾ�����е����С��豸����ʱ��0X10��Ϊ�Ҷ�С��һ֡��*/
      uint8_t  len;/*ͷ�����ĸ��ֽ� ��ʾ�����е����� ��ǰ��һ֡�ĳ���*/

      uint8_t  body[BLEMODE_FARM_MAX];
      
      /*ǰ����Э���ͷ+body ��������׷�ӵ� ��Ҫ�Ƿ���memcpy( &ble_app[0] ,&BleModuleAppData.Msg.Data ,BleModuleAppData.Msg.DatLength); ����ֱ�Ӱ�ģ������� ��head+body*/
      uint16_t alllen;  
      AppDataHeardType hdr;//ble_mode_packet ��ɲ��أ�����  9
}BleProtData;



extern BleProtData ble_app[2];


typedef struct
{
    void        (*init)                (void);
    void	 (*send)                (uint8_t *head, uint16_t handle, uint8_t *data, uint16_t length);
    uint8_t      (*read_mac)            (uint8_t *mac);
    uint32_t     (*read_version)        (void);
    void        (*set_default)         (void);
    void        (*resert)              (void);
    void        (*timer_isr)           (void);
}buletoothDriveType;


extern buletoothDriveType btModule;
extern void bluetooth_drv_init( void );
#endif


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


#define BLEMODE_FARM_MAX                (300)
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

typedef struct  _BleModuleAppMsgType
{
    AppDataHeardType hdr;
    uint8_t  Data[40];
    uint16_t DatLength;
}BleModuleAppMsgType;


typedef struct  _BleModuleAppData
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







typedef enum
{
	POS11,
	POS12,
	POS13,
	POS14,
	POS15,
	POS21,
	POS22,
	POS23,
	POS24,
	POS25,
	POS31,
        POS32,
}UsartPos_T;



__packed  typedef struct _ProtData
{
	uint8_t  POS11_head;
	uint8_t  POS12_num;
	uint8_t  POS13_rnum;
	uint16_t POS1415_len;
	uint16_t POS2122T;
	uint16_t POS2324L;
	uint8_t  POS25V[BLEMODE_FARM_MAX];
	uint16_t POS31_CRC;
	
	uint16_t POS25Vlen;
    AppDataHeardType hdr;
}ProtData_T;


extern ProtData_T pag[2];

void show_ProtData(ProtData_T *pag);

char is_crc_ok( ProtData_T *pag );


typedef struct
{
    void        (*init)                (void);
    void	 (*send)                (ProtData_T *nanopb , uint8_t *data , uint16_t length  );
    uint8_t      (*read_mac)            (uint8_t *mac);
    uint32_t     (*read_version)        (void);
    void        (*set_default)         (void);
    void        (*resert)              (void);
    void        (*timer_isr)           (void);
}buletoothDriveType;


extern buletoothDriveType btModule;
extern void bluetooth_drv_init( void );
#endif


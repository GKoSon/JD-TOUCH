#ifndef _SERIAL_SEND_H_
#define _SERIAL_SEND_H_

#include <stdio.h>
#include <stdint.h>
#include "cmsis_os.h"

#define  START_NUM                              (0xFFFF)
#define  HEAD_LEN                               (6)
//#define  HANDLE_TYPE_CMD_LOG                    (1)
//#define  HANDLE_TYPE_CMD_STATUS                 (2)

#define  QUEUE_SERIAL_LENGTH                    (10)

typedef enum 
{
    HANDLE_TYPE_CMD_LOG = 0x01,      //发送log
    HANDLE_TYPE_CMD_STATUS = 0x02,  // 发送报警信息
    HANDLE_TYPE_CMD_LIST = 0x06,    // 发送黑白名单时间戳 
    HANDLE_TYPE_CMD_MAC = 0x08,     // 发送mac地址    
    HANDLE_TYPE_CMD_INSTALL_FINISH = 0x10,     // 发送安装工安装完成
    HANDLE_TYPE_CMD_DEVICE_RESET = 0x11,     // 发送安装工重置命令
    HANDLE_TYPE_CMD_OPEN_SUCC = 0x12,       // 发送开门成功命令
    HANDLE_TYPE_CMD_INSTALL_ONE = 0x13,         // 发送设备注册信息1
    HANDLE_TYPE_CMD_MODIFY_PARA = 0x14,         // 发送读头参数信息
    HANDLE_TYPE_CMD_GET_INSTALL_STATUS = 0x15,   //获取设备注册状态
    HANDLE_TYPE_CMD_SET_PARA = 0x16,         // 发送设备参数信息
    HANDLE_TYPE_CMD_GET_PARA = 0x18,         // 获取设备参数
    HANDLE_TYPE_CMD_SOFTWARE_VERSION = 0x24,         // 发送读头版本号
    
} TslCmdType_t;

typedef struct
{
    uint8_t         handle;   
    uint16_t        length;
}serialTaskQueueType;

extern volatile uint8_t macReceiveFlag;
extern volatile uint8_t devParaReceiveFlag;
extern volatile uint8_t devInstallReceiveFlag;
extern volatile uint8_t installStatus;
extern volatile uint8_t getDevParaFlag;
extern volatile uint16_t devParaDataLen;

void creat_send_task( void );
uint16_t crc16Check(uint8_t *puchMsg, uint16_t usDataLen);
extern int8_t serialSendData(uint8_t cmdType, uint8_t *buf, int32_t len);
extern void send_list_data(void);
extern void send_install_finish_data(void);
extern void send_device_reset_data(void);
extern void send_device_open_succ_data(void);
extern void send_device_install_data(void);
extern void send_device_set_para_data(void);
extern void send_device_get_install_status_data(void);
extern void send_device_get_para_data(void);
extern void send_device_modify_para_data(void);
extern void send_softversion_data(void);

#endif


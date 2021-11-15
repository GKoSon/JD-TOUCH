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
    HANDLE_TYPE_CMD_LOG = 0x01,      //����log
    HANDLE_TYPE_CMD_STATUS = 0x02,  // ���ͱ�����Ϣ
    HANDLE_TYPE_CMD_LIST = 0x06,    // ���ͺڰ�����ʱ��� 
    HANDLE_TYPE_CMD_MAC = 0x08,     // ����mac��ַ    
    HANDLE_TYPE_CMD_INSTALL_FINISH = 0x10,     // ���Ͱ�װ����װ���
    HANDLE_TYPE_CMD_DEVICE_RESET = 0x11,     // ���Ͱ�װ����������
    HANDLE_TYPE_CMD_OPEN_SUCC = 0x12,       // ���Ϳ��ųɹ�����
    HANDLE_TYPE_CMD_INSTALL_ONE = 0x13,         // �����豸ע����Ϣ1
    HANDLE_TYPE_CMD_MODIFY_PARA = 0x14,         // ���Ͷ�ͷ������Ϣ
    HANDLE_TYPE_CMD_GET_INSTALL_STATUS = 0x15,   //��ȡ�豸ע��״̬
    HANDLE_TYPE_CMD_SET_PARA = 0x16,         // �����豸������Ϣ
    HANDLE_TYPE_CMD_GET_PARA = 0x18,         // ��ȡ�豸����
    HANDLE_TYPE_CMD_SOFTWARE_VERSION = 0x24,         // ���Ͷ�ͷ�汾��
    
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


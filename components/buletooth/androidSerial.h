#ifndef ANDROID_SERIAL_H_
#define ANDROID_SERIAL_H_

#include "stdint.h"
#include "cmsis_os.h"

#define	UPG_MARK		     (0xAABB55AA)
#define SERIAL_RECEIVE_MAX   (2048)

#define DATA_HEAD_LED        (6)  

#define DATA_HEAD_OFFSET     (0)  
#define DATA_LEN_OFFSET      (2)
#define DATA_TYPE_OFFSET     (4)
#define DATA_SEQID_OFFSET    (5)
#define DATA_CRC16_LEN       (2)

#define REPLY_SUCCESS        (0)
#define REPLY_FAIL           (1)
#define REPLY_ERROR          (2)

typedef struct
{
    uint8_t  receiveFinsh;
    uint16_t len;
    char     buff[SERIAL_RECEIVE_MAX];
}androidSerialReceiveDataType;

typedef struct
{
    uint8_t			result;
    uint32_t		len;
    uint32_t		ver;
    uint32_t		fileSize;
    uint32_t		crc32;
	uint32_t 		id;
    uint32_t		upgFlag;
    uint32_t		writeAddr;
}otaRecvCmdType;

/* Function declaration *******************************/
typedef struct
{
    void (*send)	( uint8_t *pData , uint16_t len);
}androidSerialFunsType;

extern androidSerialFunsType androidSerial;

extern SemaphoreHandle_t    xAndroidSerialSendSemaphore;
extern SemaphoreHandle_t    xAndroidSerialSemaphore;

void creat_androidSerial_task(void);
#endif
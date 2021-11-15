#ifndef _MATE_H_
#define _MATE_H_



#include "stdlib.h"
#include "stdint.h"

typedef struct
{
    uint8_t sn;
    uint8_t type;
    uint8_t cmd;
    uint8_t length;
    uint8_t data[256];
    uint16_t crc16;
    uint8_t cnt;
}mateDataType;

typedef enum
{
    MATE_HEARD1 = 0 , 
    MATE_HEARD2,
    MATE_SN,
    MATE_TYPE,
    MATE_CMD,
    MATE_LENGTH,
    MATE_DATA,
    MATE_CRC1,
    MATE_CRC2,
}mateRecvStatusEnum;


typedef struct
{
    uint8_t type;
    uint16_t len;
    uint8_t data[64];
}mateSendBufferType;

#define     QUEUE_MATE_LENGTH       (5)

#define     FUN_TYPE                (1)
  #define   OPEN_CMD                (1)

#define     QRCODE_TYPE             (2)
  #define   QRCODE_BUFF_CMD         (1)

#define     ACK_TYPE                (0x79)




void mate_send(uint8_t type ,uint8_t cmd ,uint8_t *msg ,uint8_t length);
void mate_send_ack( void );
void creat_mate_task( void );
#endif


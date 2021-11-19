#ifndef _OPEN_LOG_H_
#define _OPEN_LOG_H_

#include <stdint.h>
#include "cmsis_os.h"


#define QUEUE_OPENLOG_PARAMETER     (22u)

#define LOG_FIND_NULL           (-1)
#define INVALID                 0x5A
#define EFFECTIVE               0xA5
#define LOG_MAX                    (5000)
#define LOG_SIZE                (sizeof(openLogType))
#define    QUEUE_LOG_LENGTH        (5)

#define QR_CODE_INVAILD         (2)
#define QR_PHONE_INVAILD        (3)

typedef enum
{
    OPEN_FOR_CARD,
    OPEN_FOR_ONCE_PWD,
    OPEN_FOR_APP_REMOTE,
    OPEN_FOR_IN_DOOR,
    OPEN_FOR_FACE,
    OPEN_FOR_FINGER,
    OPEN_FOR_PWD,
    OPEN_FOR_DTMF,
    OPEN_FOR_PHONE_BT,
    
}openDoorTypeEnum;


typedef enum
{
    OPENLOG_FORM_KEY,
    OPENLOG_FORM_CARD,
    OPENLOG_FORM_REMOTE,
    OPENLOG_FORM_PWD,
    OPENLOG_FORM_QRCODE
    //OPENLOG_FORM_BAND = 6,??¨°a¨°? ?¨ª¦Ì¡À¡Á?OPENLOG_FORM_CARD
}openLogTypeEnum;


typedef enum
{
    LOG_DEL,
    LOG_SEND,
    LOG_ADD,
}journalCmdEnum;






typedef struct
{
    uint8_t cardIssueType;
    uint8_t cardType;
    uint8_t cardNumber[8];
    uint8_t cardNumberLength;
}openLogUseCardDataType;

typedef struct
{
    uint8_t password[6];
    uint8_t pwdLength;

}openLogUsePwdDataType;

typedef struct
{
    uint32_t messageId;
    uint8_t userId[32];
}openLogUseRemoteDataType;

typedef struct
{
    uint8_t effective;
    uint8_t logType;
    uint8_t openType;
    uint8_t openResult;
    uint32_t openTime;
    uint32_t eraseCnt;
    uint32_t cnt;
}openLogHeardType;

typedef struct
{
    openLogHeardType    hdr;
    uint8_t             data[48];
}openLogType;

typedef struct
{
    openLogTypeEnum type;
    uint32_t        length;
    uint8_t         data[256];
}openlogDataType;


typedef struct
{
    journalCmdEnum  cmd;
    uint16_t         sn;
    openlogDataType openlog;
}journalTaskQueueType;

typedef struct _open_log
{
    //void     (*add_form_key)        (void);
    //void     (*add_form_remote)    ( uint8_t *userId , uint32_t messageId);
    //void    (*add_form_card)    ( tagBufferType *tag , uint8_t openResult);
    //void    (*add_form_pwd)     ( uint8_t *pwd , uint8_t pwdLength);
    void    (*save_log)         ( openlogDataType *saveLog );
    void    (*clear)            (void);
    void    (*stop_timer)       (void);
    void    (*send_queue)       ( journalCmdEnum cmd , uint16_t sn );
}open_log_type;



extern xSemaphoreHandle xBinarySemaphoreOpenLog;
extern open_log_type journal;

uint16_t journal_read_send_log( uint8_t *msg );
void creat_open_log_task( void );

#endif


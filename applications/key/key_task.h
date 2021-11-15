#ifndef _TASK_KEY_H_
#define _TASK_KEY_H_

#include <stdio.h>
#include <stdint.h>
#include "cmsis_os.h"

#define     QUEUE_KEY_TASK_LENGTH       (2)
#define     QUEUE_KEY_TASK_PARAMETER    ( 0x22UL )


#define PASSWORD_MAX_LENGTH             6
#define FIXED_PASSWORD_LENGTH           6
#define TEMPORARY_PASSWORD_LENGTH		4

#if ((TEMPORARY_PASSWORD_LENGTH > PASSWORD_MAX_LENGTH)||(FIXED_PASSWORD_LENGTH > PASSWORD_MAX_LENGTH))
#error  "临时密码或者固定密码长度大于密码缓存最大长度"
#endif


typedef enum
{
    SYS_KEY,
    TOUCH_KEY,
    FUN_KEY,
} keyHandleEnum;


typedef enum
{
    KEY_1_SEC,
    KEY_2_SEC,
    KEY_4_SEC,
    KEY_8_SEC,
} sysKeyEnum;

typedef enum
{
    OPEN_DOOR_KEY,
} funKeyEnum;

typedef enum
{
    KEY_INIT,
    KEY_0,
    KEY_1,
    KEY_2,
    KEY_3,
    KEY_4,
    KEY_5,
    KEY_6,
    KEY_7,
    KEY_8,
    KEY_9,
    KEY_DEL,    // *
    KEY_ENT,    // #
} touchKeyEnum;



typedef struct
{
    uint8_t cmd;
    uint8_t keyValue;
} keyTaskType;

typedef struct
{
    uint8_t buffer[PASSWORD_MAX_LENGTH];
    uint8_t length;
} touchKeyType;

typedef struct
{
    touchKeyEnum key;
    uint8_t value;
} keyValueMaType;




void creat_key_task( void );

extern touchKeyEnum cy3116_read_key( void );


#endif


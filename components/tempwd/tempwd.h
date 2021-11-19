#ifndef _TEMPWD_H_
#define _TEMPWD_H_

#include <stdint.h>
#include "spi_flash.h"


#define    PWD_MAX_NUM                    (5000)
#define    PWD_SIZE                    sizeof(tempwdType)
#define    PWD_INDEX_SIZE                (sizeof(int))
#define EMPTY_ID                     0xFFFFFFFF

#define PWD_SUCCESS                 (1)
#define PWD_NULL_ID                 (-1)
#define PWD_FULL                    (-2)
#define PWD_EXIST                   (-3)

typedef struct
{

    uint32_t pwd;
    uint32_t time;
    uint8_t temp[8];
}tempwdType;


typedef struct _pwd
{
    int32_t     (*find)          ( uint32_t pwd, tempwdType *outPwd );
    int32_t     (*add)           ( tempwdType *pwd );
    int32_t     (*del)           ( uint32_t pwd );
    void        (*clear)         ( void );
    void        (*clear_overdue) ( void );
    void        (*show)          ( void );
}tempwdOpstype;



extern tempwdOpstype tempwd;
uint8_t dev_pwd_read_flash(uint32_t addr,uint8_t* buffer,  uint16_t length);

#endif

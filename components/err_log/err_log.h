#ifndef _ERR_LOG_H_
#define _ERR_LOG_H_

#include "spi_flash.h"

#define		ERR_LOG_INFO_ADDR		ERR_START_ADDR
#define		ERR_LOG_DATA_ADDR		ERR_LOG_INFO_ADDR+4096


typedef enum
{
	ASSERT_ERR = 0,
	DEBUG_MESSAGE,
	SYS_RESERT_ERR_CODE,
	BOARD_VDD_ERR,
	BOARD_TOUCH_IC_ERR,
	BOARD_SWIP_IC_ERR,
	TASK_OVERFLOW_ERR,
	NET_AUTH_ERR,
}errCodeEnum;

typedef struct
{
	uint32_t pos;
	uint32_t cnt;
}errInfoType;

typedef struct
{
	uint32_t time;
	uint8_t buffer[124];
}errMsgType;


void err_log(errCodeEnum errCode,uint8_t *err , uint32_t errLen);
void err_log_get_capacity( void );
void err_log_read(void);
void err_log_format( void );

#endif

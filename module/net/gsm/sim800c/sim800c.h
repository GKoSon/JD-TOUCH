#ifndef SIM800C_H_
#define SIM800C_H_



#include "stdint.h"
#include "socket.h"



#define    gsm_assert( x , z)        do{\
    if( (x) == FALSE)\
    {\
        gsmRunStatus = GSM_POWER_OFF;\
    }\
    else\
    {\
        gsmRunStatus = z;\
    }\
}while(0);


typedef enum
{
    COMMAND_MODE,
    DATA_MODE,
}gsmReceiveModeEnum;

typedef enum
{
    GSM_RUN_INIT,
    GSM_POWER_OFF,
    GSM_POWER_ON,
    GSM_CLOSE_ECHO,
    GSM_SET_IPR,
    GSM_READ_SIM,
    GSM_READ_CSQ,
    GSM_READ_NET,
    GSM_READ_GPRS,
    GSM_SET_SEND_MODE,
    GSM_SET_MUX,
    GSM_CLOSE_CONNECT,
    GSM_SET_APN,
    GSM_ON_GPRS,
    GSM_READ_IP,
    GSM_INIT_FINSG,
    GSM_INIT_SUCCESS,
}gsmRunEnum;

typedef enum
{
    GSM_RECEIVE_NULL = 1,
    GSM_RECEIVE_TYPE,
    GSM_DEACT_TYPE,
    GSM_NOT_READY_TYPE,
    GSM_SEND_READY,
    GSM_CONNECT_FAIL_TYPE,
    GSM_CONNECT_OK_TYPE,
    GSM_CLOSE_TYPE,
    GSM_SEND_OK_TYPE,
    GSM_SEND_FAIL_TYPE,
    GSM_DATA_ACCEPT_TYPE,
    GSM_ERROR_TYPE,
}GsmRunGetEnum;


typedef enum
{
    GSM_INIT,
    GSM_RECEIVE,
}gsmRunStatusEnum;


extern devComType gsm;

int8_t gsm_http_download_file ( uint8_t *fineName );
void gsm_http_close( void );
void gsm_close( void );

#endif


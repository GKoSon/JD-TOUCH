#ifndef _SOCKET_H_
#define _SOCKET_H_

#include "stdint.h"
#include "cmsis_os.h"
#include "usart.h"

#define     SOCKET_CONNECT_MAX      5

typedef void    (*socket_recv_call_back)( uint8_t *data ,uint16_t length);
typedef void    (*socket_status_call_back)( uint8_t status);


typedef struct
{
    void (*recv_fun)    ( uint8_t *data ,uint16_t length);
    void (*status_fun)   ( uint8_t status);
}socketCallbackFunType;

enum NET_CONNECT_STATE
{
    NET_INITING,
    NET_CONNECT,
    NET_DISCONNECT,
};

typedef enum
{
    RUN_INIT,
    RUN_CONNECTING,
    RUN_CONNECT,
}runStatusEnum;

typedef enum
{
    SOCKET_INIT,
    SOCKET_READ,
    SOCKET_CLOSE,
    SOCKET_ALL,
}socketStatusEnum;

#define        SOCKET_OK         (0)

typedef enum
{
    SOCKERT_STATUS_ERR = -99,
    SOCKET_CONNECT_ERR,
    SOCKET_CONNECT_CLOSE,
    SOCKET_CONNECT_FULL,
    SOCKET_CONNECT_BUSY,
    SOCKET_CONNECT_NORES,//no response
    SOCKET_CONNECT_FAIL,
    SOCKET_CONNECT_TIMEOUT,
    SOCKETCLOSE_ERR,
    SOCKET_CLOSE_BUSY,
    SOCKET_SEND_ERR,
    SOCKET_SEND_NOID,
    SOCKET_SEND_NOCON, //no connect
    SOCKET_SEND_BUSY,
    SOCKET_SEND_NOREADY,
    SOCKET_SEND_NORESULT,
    SOCKET_SEND_RETURNERR,
    SOCKET_SEND_FAIL,
        SOCKER_READ_ERR,
    SOCKER_READ_NOID,
    SOCKET_READ_NOCON,
    SOCKET_READ_LENERR,
    SOCKET_READ_TIMEOUT,

}socketErr;

typedef enum
{
    SOCKET_CLOSE_STATUS,
    SOCKET_WORKING_STATUS,
}socketWorkStatusEnum;

typedef struct
{
__IO int8_t            id;
__IO int8_t                 useFlag;
socketStatusEnum            status;
char                        *msg;
uint16_t                    maxSize;
__IO uint16_t               len;
}sockeArryType;




typedef struct
{
    uint8_t     (*isOK)                 ( void );
    int8_t        (*disconnect)           ( int8_t id );
    int8_t      (*connect)              ( uint8_t *ip , uint16_t port , char *pData , uint16_t size);
    int32_t        (*read)                 ( int8_t id , uint32_t timeOut);
    int         (*read_buffer)          (int8_t id , uint8_t *recvData , int32_t recvLen , uint32_t timeout);
    int8_t        (*send)                 ( uint8_t id ,  uint8_t *sendData , uint16_t length , uint32_t timeout );
    void        (*close)                ( void );
}socketOpsType;

typedef struct
{
    void        (*init)                  ( void );
    uint8_t        (*isOK)                  ( void );
    int8_t        (*disconnect)              ( int8_t id);
    int8_t        (*connect)                 ( int8_t id , uint8_t *ip , uint16_t port);
    int8_t        (*send)                    ( uint8_t socketId , uint8_t *sendData , uint16_t length );
    void        (*close)                   ( void );
}devComType;


sockeArryType * socket_read_obj( uint8_t id);

void socket_set_id_clear( int8_t id );

void socket_set_id_use( int8_t id);

int8_t socket_find_id( void );

void socket_clear_all( void );

void socket_clear_bind( int8_t id);

void socket_set_status( socketWorkStatusEnum status);

void socket_err(int8_t err , int8_t id);

void socket_clear_buffer( uint8_t id);

void socket_compon_init( void );

void socket_close_f( void );
extern socketOpsType   socket;
extern sockeArryType   sockeArry[SOCKET_CONNECT_MAX];
#endif


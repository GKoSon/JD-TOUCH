#ifndef  _HTTPBUSS_H_
#define  _HTTPBUSS_H_

#define Elog(level, fmt, ...)     log(level , fmt, ## __VA_ARGS__) 

typedef struct _httpRecvMsgType
{
    uint8_t	rule;
} httpRecvMsgType;

char Get_HTTPStatus(void);

void Reset_HTTPStatus(void);

void creat_http_task( void );

#endif

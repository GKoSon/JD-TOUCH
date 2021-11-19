#ifndef _ESP12S_H_
#define _ESP12S_H_

#include "socket.h"


#define    wifi_assert( x , z)        do{\
    if( (x) == FALSE)\
    {\
        wifiRunStatus = WIFI_POWER_OFF;\
    }\
    else\
    {\
        wifiRunStatus = z;\
    }\
}while(0);

typedef enum
{
    WIFI_STATION_MODE = 1,
    WIFI_AP_MODE,
    WIFI_STATION_AP_MODE,
}wifiRunModeEnum;

typedef enum
{
    WIFI_RUN_INIT,
    WIFI_POWER_OFF,
    WIFI_POWER_ON,
    WIFI_SET_ECHO,
    WIFI_SET_DEFAULT,
    WIFI_WAIT_CONNECT,
    WIFI_WAIT_GOT_IP,
    WIFI_PRINTF_IP,
    WIFI_SET_MUX,
    WIFI_INIT_FINSG,
    WIFI_INIT_SUCCESS,
}wifiRunEnum;

typedef enum
{
    WIFI_COMMAND_MODE,
    WIFI_DATA_MODE,
}wifiReceiveModeEnum;


typedef enum
{
    WIFI_INIT,
    WIFI_RECEIVE,
}wifiRunStatusEnum;


typedef enum
{
    WIFI_GOT_IP_STATUS = 2,
    WIFI_CONNECT_SERVER_STATUS,
    WIFI_DISOCONECT_STATUS,
    WIFI_DISCONNECT_AP,
    WIFI_INIT_STATUS,
}esp12sConnectStatusEnum;




#define WIRI_RUN_MODE       WIFI_STATION_MODE
extern devComType    wifi;

uint8_t wifi_set_ssid( uint8_t *ssid , uint8_t *pwd );

#endif



#ifndef _W5500_CONFIG_H_
#define _W5500_CONFIG_H_
#include "socket.h"
#include "stdint.h"


#define		DHCP_SN			7
#define		DNS_SN			6


typedef enum
{
    ETH_INIT,
    ETH_RUN,
}ETHRunStatusEnum;

uint8_t w5500_line_connect( void );

extern ETHRunStatusEnum ethstatus;
extern devComType     eth;


#endif
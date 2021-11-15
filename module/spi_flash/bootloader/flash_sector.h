#ifndef _FLASH_SECTOR_H_
#define _FLASH_SECTOR_H_

#include "stdint.h"


#define     SPI_FLASH_USER_START_ADDR       (0x1000)
#define		OTA_START_ADDR					(0x180000)

extern uint32_t    sysCfgAddr;
extern uint32_t    permiListBootAddr;
extern uint32_t    permiListDataAddr;
extern uint32_t    openLogDataAddr;
extern uint32_t    passwordBootAddr;
extern uint32_t    passwordDataAddr;

#define     SYS_CFG_ADDR            sysCfgAddr
#define     PERMI_LIST_BOOT_ADDR    permiListBootAddr
#define     PERMI_LIST_DATA_ADDR    permiListDataAddr
#define     OPEN_LOG_DATA_ADDRE     openLogDataAddr
#define		PASSWORD_BOOT_ADDR		passwordBootAddr
#define		PASSWORD_DATA_ADDR		passwordDataAddr


void flash_sector_init( void );


#endif


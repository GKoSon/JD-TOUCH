#ifndef _FLASH_SECTOR_H_
#define _FLASH_SECTOR_H_

#include "stdint.h"

#define        SPI_FLASH_MAX_SIZE              0x800000//8*1024*1024 8M
#define        SPI_FLASH_USER_START_ADDR       0X0    //0x1000      //一页做cfg
#define        SPI_FLASH_USER_INFO_ADDR        0X1000//0x2000      //后面开始自己使用
#define        OTA_START_ADDR                  0x180000    //1.5M start
#define        ERR_START_ADDR                  0x300000//2M start
#define        ERR_END_ADDR                    SPI_FLASH_MAX_SIZE


extern uint32_t    sysCfgAddr;
extern uint32_t    permiListBootAddr;
extern uint32_t    permiListDataAddr;
extern uint32_t    openLogDataAddr;


#define        SYS_CNT_ADDR            sysCfgAddr
#define        PERMI_LIST_BOOT_ADDR    permiListBootAddr
#define        PERMI_LIST_DATA_ADDR    permiListDataAddr
#define        OPEN_LOG_DATA_ADDRE     openLogDataAddr



void flash_sector_init( void );


#endif


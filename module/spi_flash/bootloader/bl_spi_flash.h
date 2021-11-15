#ifndef _SPI_FLASH_H_
#define _SPI_FLASH_H_

#include <stdio.h>
#include <stdint.h>
#include "unit.h"
#include "cmsis_os.h"
#include "module\modules_init.h"
#include "flash_sector.h"


// M25P SPI Flash supported commands 
#define FLASH_CMD_WRITE			0x02  // Write to Memory instruction 
#define FLASH_CMD_WRSR			0x01  // Write Status Register instruction 
#define FLASH_CMD_WREN			0x06  // Write enable instruction 
#define FLASH_CMD_READ			0x03  // Read from Memory instruction 
#define FLASH_CMD_RDSR			0x05  // Read Status Register instruction  
#define FLASH_CMD_RDID			0x9F  // Read identification 
#define FLASH_CMD_SE			0x20  // Sector Erase instruction 
#define FLASH_CMD_BL32K			0x52  // 32K Block Erase instruction 
#define FLASH_CMD_BL64K			0xD8  // 32K Block Erase instruction 
#define FLASH_CMD_BE			0xC7  // Bulk Erase instruction 

#define FLASH_WIP_FLAG			0x01  // Write In Progress (WIP) flag 
#define FLASH_DUMMY_BYTE		0xA5
#define FLASH_SPI_PAGESIZE		0x100
#define FLASH_SPI_BLOCKSIZE     0x1000

#define FLASH_ID                0xC84016

typedef struct  _spi_flash
{
    uint8_t (*earse)           (uint32_t addr);
    uint32_t (*read_chip_id)(void);
    uint8_t (*write)    (uint32_t writeAddr,uint8_t* buffer,  uint16_t numByteToWrite);
    uint8_t (*read)     (uint32_t readAddr, uint8_t* buffer, uint16_t NumByteToRead);
}spi_flash_type;

extern spi_flash_type  flash;
extern uint8_t fb[4096];


extern SemaphoreHandle_t    xFlashSemaphore;

#endif


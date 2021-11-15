#include "flash_sector.h"
#include "spi_flash.h"
#include "sysCfg.h"
#include "tempwd.h"
#include "permi_list.h"
#include "open_log.h"
#include "unit.h"
uint32_t    sysCfgAddr = 0;
uint32_t    permiListBootAddr = 0;
uint32_t    permiListDataAddr = 0;
uint32_t    openLogDataAddr = 0;
uint32_t    passwordBootAddr = 0;
uint32_t    passwordDataAddr = 0;
uint32_t    endFlashAddr = 0;

void flash_sector_init( void )
{
    sysCfgAddr = SPI_FLASH_USER_START_ADDR;
    permiListBootAddr = sysCfgAddr + ((sizeof(SystemConfigType)/FLASH_SPI_BLOCKSIZE)+1)*FLASH_SPI_BLOCKSIZE;
    permiListDataAddr   = permiListBootAddr + ((PERMI_LIST_MAX*PERMI_LISD_INDEX_SIZE/FLASH_SPI_BLOCKSIZE)+1)*FLASH_SPI_BLOCKSIZE;
    openLogDataAddr =  permiListDataAddr+ (((PERMI_LIST_SIZE*PERMI_LIST_MAX)/FLASH_SPI_BLOCKSIZE)+1)*FLASH_SPI_BLOCKSIZE;
    passwordBootAddr = openLogDataAddr + (((LOG_SIZE*LOG_MAX)/FLASH_SPI_BLOCKSIZE)+1)*FLASH_SPI_BLOCKSIZE;
    passwordDataAddr = passwordBootAddr + + ((PWD_MAX_NUM*PWD_INDEX_SIZE/FLASH_SPI_BLOCKSIZE)+1)*FLASH_SPI_BLOCKSIZE;
    
    
    endFlashAddr = passwordDataAddr + (((PWD_SIZE*PERMI_LIST_MAX)/FLASH_SPI_BLOCKSIZE)+1)*FLASH_SPI_BLOCKSIZE;
    
    //endFlashAddr = passwordDataAddr + (((PWD_SIZE*PERMI_LIST_MAX)/FLASH_SPI_BLOCKSIZE)+1)*FLASH_SPI_BLOCKSIZE;
    //log(DEBUG,"系统日志存储起始地址 , ADDR = %x ,PAGE = %d \n" ,sysCfgAddr , sysCfgAddr/FLASH_SPI_BLOCKSIZE );
    //log(DEBUG,"黑白名单索引区存储起始地址 , ADDR = %x ,PAGE = %d \n" ,permiListBootAddr , permiListBootAddr/FLASH_SPI_BLOCKSIZE );
    //log(DEBUG,"黑白名单数据区存储起始地址 , ADDR = %x ,PAGE = %d \n" ,permiListDataAddr , permiListDataAddr/FLASH_SPI_BLOCKSIZE );
    //log(DEBUG,"开门日志存储起始地址 , ADDR = %x ,PAGE = %d \n" ,openLogDataAddr , openLogDataAddr/FLASH_SPI_BLOCKSIZE );
    //log(DEBUG,"临时密码存储起始地址 , ADDR = %x ,PAGE = %d \n" ,passwordDataAddr , passwordDataAddr/FLASH_SPI_BLOCKSIZE );
    
    if( endFlashAddr >= OTA_START_ADDR)
    {
        log(ERR,"数据存储区已越界\n");
    }
}

//INIT_EXPORT(flash_sector_init , "Flash存储地址分配");

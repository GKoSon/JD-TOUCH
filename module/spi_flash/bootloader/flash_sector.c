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
    //log(DEBUG,"ϵͳ��־�洢��ʼ��ַ , ADDR = %x ,PAGE = %d \n" ,sysCfgAddr , sysCfgAddr/FLASH_SPI_BLOCKSIZE );
    //log(DEBUG,"�ڰ������������洢��ʼ��ַ , ADDR = %x ,PAGE = %d \n" ,permiListBootAddr , permiListBootAddr/FLASH_SPI_BLOCKSIZE );
    //log(DEBUG,"�ڰ������������洢��ʼ��ַ , ADDR = %x ,PAGE = %d \n" ,permiListDataAddr , permiListDataAddr/FLASH_SPI_BLOCKSIZE );
    //log(DEBUG,"������־�洢��ʼ��ַ , ADDR = %x ,PAGE = %d \n" ,openLogDataAddr , openLogDataAddr/FLASH_SPI_BLOCKSIZE );
    //log(DEBUG,"��ʱ����洢��ʼ��ַ , ADDR = %x ,PAGE = %d \n" ,passwordDataAddr , passwordDataAddr/FLASH_SPI_BLOCKSIZE );
    
    if( endFlashAddr >= OTA_START_ADDR)
    {
        log(ERR,"���ݴ洢����Խ��\n");
    }
}

//INIT_EXPORT(flash_sector_init , "Flash�洢��ַ����");

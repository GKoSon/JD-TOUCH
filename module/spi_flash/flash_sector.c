#include "flash_sector.h"
#include "spi_flash.h"
#include "sysCfg.h"
#include "permi_list.h"
#include "open_log.h"

uint32_t    sysCfgAddr = 0;
uint32_t    permiListBootAddr = 0;
uint32_t    permiListDataAddr = 0;
uint32_t    openLogDataAddr = 0;
uint32_t    endFlashAddr = 0;

void flash_sector_init( void )
{
    
    sysCfgAddr  = SPI_FLASH_USER_START_ADDR;
    permiListBootAddr = SPI_FLASH_USER_INFO_ADDR;
    permiListDataAddr = permiListBootAddr + ((PERMI_LIST_MAX*PERMI_LISD_INDEX_SIZE/FLASH_SPI_BLOCKSIZE)+1)*FLASH_SPI_BLOCKSIZE;
    openLogDataAddr =  permiListDataAddr+ (((PERMI_LIST_SIZE*PERMI_LIST_MAX)/FLASH_SPI_BLOCKSIZE)+1)*FLASH_SPI_BLOCKSIZE; 
    endFlashAddr = openLogDataAddr + (((LOG_SIZE*LOG_MAX)/FLASH_SPI_BLOCKSIZE)+1)*FLASH_SPI_BLOCKSIZE;;
    

    log(DEBUG,"[FLASH]ϵͳ������ʼ��ַ ,           ADDR = 0X%08X ,PAGE = %d \n" ,sysCfgAddr ,  sysCfgAddr/FLASH_SPI_BLOCKSIZE );
    
    log(DEBUG,"[FLASH]�ڰ������������洢��ʼ��ַ , ADDR = 0X%08X ,PAGE = %d \n" ,permiListBootAddr , permiListBootAddr/FLASH_SPI_BLOCKSIZE );
    log(DEBUG,"[FLASH]�ڰ������������洢��ʼ��ַ , ADDR = 0X%08X ,PAGE = %d \n" ,permiListDataAddr , permiListDataAddr/FLASH_SPI_BLOCKSIZE );
    
    log(DEBUG,"[FLASH]������־�洢��ʼ��ַ ,       ADDR = 0X%08X ,PAGE = %d \n" ,openLogDataAddr , openLogDataAddr/FLASH_SPI_BLOCKSIZE );
    log(DEBUG,"[FLASH]FLASHʹ����Ϻ������е�ַ,   ADDR = 0X%08X ,PAGE = %d \n" ,endFlashAddr , endFlashAddr/FLASH_SPI_BLOCKSIZE );
    if( endFlashAddr >= OTA_START_ADDR)
    {
        log_err("[FLASH]���ݴ洢����Խ��\n");
    }
}


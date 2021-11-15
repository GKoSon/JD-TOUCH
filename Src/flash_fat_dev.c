
#include <string.h>
#include "ff_gen_drv.h"
#include "spi_flash.h"

#define SPI_FLASH_REBUILD           0    // 1:使能格式化串行Flash，0：禁用格式化串行Flash
#define SPI_FLASH_SECTOR_SIZE    4096    // 串行Flash扇区大小
#define SPI_FLASH_START_SECTOR   512    // 串行Flash文件系统FatFS偏移量
#define SPI_FLASH_SECTOR_COUNT   1536    // 串行Flash文件系统FatFS占用扇区个数



/* 私有变量 ------------------------------------------------------------------*/
static __IO DSTATUS Stat = STA_NOINIT;

/* 扩展变量 ------------------------------------------------------------------*/
/* 私有函数原形 --------------------------------------------------------------*/
DSTATUS SPIFLASH_initialize(BYTE);
DSTATUS SPIFLASH_status(BYTE);
DRESULT SPIFLASH_read(BYTE, BYTE*, DWORD, UINT);

#if _USE_WRITE == 1   // 如果允许写操作
  DRESULT SPIFLASH_write (BYTE, const BYTE*, DWORD, UINT);
#endif /* _USE_WRITE == 1 */

#if _USE_IOCTL == 1   // 如果输入输出操作控制
  DRESULT SPIFLASH_ioctl (BYTE, BYTE, void*);
#endif  /* _USE_IOCTL == 1 */

/* 定义串行FLASH接口函数 */

const Diskio_drvTypeDef  SPIFLASH_Driver =
{
  SPIFLASH_initialize,                   // 串行FLASH初始化 
  SPIFLASH_status,                       // 串行FLASH状态获取
  SPIFLASH_read,                         // 串行FLASH读数据
#if  _USE_WRITE == 1
  SPIFLASH_write,                        // 串行FLASH写数据
#endif /* _USE_WRITE == 1 */
  
#if  _USE_IOCTL == 1
  SPIFLASH_ioctl,                         // 获取串行FLASH信息
#endif /* _USE_IOCTL == 1 */
};

/* 函数体 --------------------------------------------------------------------*/
/**
  * 函数功能: 串行FLASH初始化配置
  * 输入参数: 无
  * 返 回 值: 无
  * 说    明: 无
  */
DSTATUS SPIFLASH_initialize(BYTE lun)
{
#if SPI_FLASH_REBUILD == 1
  static uint8_t startflag=1;
#endif
  Stat = STA_NOINIT;

  if(flash.read_chip_id()==FLASH_ID)
  {
#if SPI_FLASH_REBUILD == 1
      if(startflag)
      {
        flash.earse(SPI_FLASH_START_SECTOR*SPI_FLASH_SECTOR_SIZE);
        startflag=0;
      }
#endif
    Stat &=~STA_NOINIT;
  }
  return Stat;
}

/**
  * 函数功能: 串行FLASH状态获取
  * 输入参数: lun : 不用
  * 返 回 值: DSTATUS：串行FLASH状态返回值
  * 说    明: 无
  */
DSTATUS SPIFLASH_status(BYTE lun)
{
  Stat = STA_NOINIT;

  if( flash.read_chip_id()== FLASH_ID)
  {
    Stat &= ~STA_NOINIT;
  }
  return Stat;
}

/**
  * 函数功能: 从串行FLASH读取数据到缓冲区
  * 输入参数: lun : 不用
  *           buff：存放读取到数据缓冲区指针
  *           sector：扇区地址(LBA)
  *           count：扇区数目
  * 返 回 值: DSTATUS：操作结果
  * 说    明: 无
  */
DRESULT SPIFLASH_read(BYTE lun, BYTE *buff, DWORD sector, UINT count)
{ 
  sector+=SPI_FLASH_START_SECTOR;      
  flash.read(sector*SPI_FLASH_SECTOR_SIZE, buff,  count*SPI_FLASH_SECTOR_SIZE);
  return RES_OK;
}

/**
  * 函数功能: 将缓冲区数据写入到串行FLASH内
  * 输入参数: lun : 不用
  *           buff：存放待写入数据的缓冲区指针
  *           sector：扇区地址(LBA)
  *           count：扇区数目
  * 返 回 值: DSTATUS：操作结果
  * 说    明: 无
  */
#if _USE_WRITE == 1
DRESULT SPIFLASH_write(BYTE lun, const BYTE *buff, DWORD sector, UINT count)
{
  uint32_t write_addr; 
  /* 扇区偏移2MB，外部Flash文件系统空间放在SPI Flash后面6MB空间 */
  sector+=SPI_FLASH_START_SECTOR;
  write_addr = sector*SPI_FLASH_SECTOR_SIZE;    
  flash.earse(write_addr);
  flash.write(write_addr , (uint8_t *)buff,count*SPI_FLASH_SECTOR_SIZE);
  return RES_OK;
}
#endif /* _USE_WRITE == 1 */

/**
  * 函数功能: 输入输出控制操作(I/O control operation)
  * 输入参数: lun : 不用
  *           cmd：控制命令
  *           buff：存放待写入或者读取数据的缓冲区指针
  * 返 回 值: DSTATUS：操作结果
  * 说    明: 无
  */
#if _USE_IOCTL == 1
DRESULT SPIFLASH_ioctl(BYTE lun, BYTE cmd, void *buff)
{
  DRESULT res = RES_ERROR;
  
  if (Stat & STA_NOINIT) return RES_NOTRDY;
  
  switch (cmd)
  {
  /* Make sure that no pending write process */
  case CTRL_SYNC :
    res = RES_OK;
    break;
  
  /* 获取串行FLASH总扇区数目(DWORD) */
  case GET_SECTOR_COUNT :
    *(DWORD * )buff = SPI_FLASH_SECTOR_COUNT;        
    res = RES_OK;
    break;
  
  /* 获取读写扇区大小(WORD) */
  case GET_SECTOR_SIZE :
    *(WORD * )buff = SPI_FLASH_SECTOR_SIZE;
    res = RES_OK;
    break;
  
  /* 获取擦除块大小(DWORD) */
  case GET_BLOCK_SIZE :
    *(DWORD * )buff = 1;
    res = RES_OK;
    break;
  
  default:
    res = RES_PARERR;
  }
  
  return res;
}
#endif /* _USE_IOCTL == 1 */

/******************* (C) COPYRIGHT 2015-2020 硬石嵌入式开发团队 *****END OF FILE****/

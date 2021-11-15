
#include <string.h>
#include "ff_gen_drv.h"
#include "spi_flash.h"

#define SPI_FLASH_REBUILD           0    // 1:ʹ�ܸ�ʽ������Flash��0�����ø�ʽ������Flash
#define SPI_FLASH_SECTOR_SIZE    4096    // ����Flash������С
#define SPI_FLASH_START_SECTOR   512    // ����Flash�ļ�ϵͳFatFSƫ����
#define SPI_FLASH_SECTOR_COUNT   1536    // ����Flash�ļ�ϵͳFatFSռ����������



/* ˽�б��� ------------------------------------------------------------------*/
static __IO DSTATUS Stat = STA_NOINIT;

/* ��չ���� ------------------------------------------------------------------*/
/* ˽�к���ԭ�� --------------------------------------------------------------*/
DSTATUS SPIFLASH_initialize(BYTE);
DSTATUS SPIFLASH_status(BYTE);
DRESULT SPIFLASH_read(BYTE, BYTE*, DWORD, UINT);

#if _USE_WRITE == 1   // �������д����
  DRESULT SPIFLASH_write (BYTE, const BYTE*, DWORD, UINT);
#endif /* _USE_WRITE == 1 */

#if _USE_IOCTL == 1   // ������������������
  DRESULT SPIFLASH_ioctl (BYTE, BYTE, void*);
#endif  /* _USE_IOCTL == 1 */

/* ���崮��FLASH�ӿں��� */

const Diskio_drvTypeDef  SPIFLASH_Driver =
{
  SPIFLASH_initialize,                   // ����FLASH��ʼ�� 
  SPIFLASH_status,                       // ����FLASH״̬��ȡ
  SPIFLASH_read,                         // ����FLASH������
#if  _USE_WRITE == 1
  SPIFLASH_write,                        // ����FLASHд����
#endif /* _USE_WRITE == 1 */
  
#if  _USE_IOCTL == 1
  SPIFLASH_ioctl,                         // ��ȡ����FLASH��Ϣ
#endif /* _USE_IOCTL == 1 */
};

/* ������ --------------------------------------------------------------------*/
/**
  * ��������: ����FLASH��ʼ������
  * �������: ��
  * �� �� ֵ: ��
  * ˵    ��: ��
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
  * ��������: ����FLASH״̬��ȡ
  * �������: lun : ����
  * �� �� ֵ: DSTATUS������FLASH״̬����ֵ
  * ˵    ��: ��
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
  * ��������: �Ӵ���FLASH��ȡ���ݵ�������
  * �������: lun : ����
  *           buff����Ŷ�ȡ�����ݻ�����ָ��
  *           sector��������ַ(LBA)
  *           count��������Ŀ
  * �� �� ֵ: DSTATUS���������
  * ˵    ��: ��
  */
DRESULT SPIFLASH_read(BYTE lun, BYTE *buff, DWORD sector, UINT count)
{ 
  sector+=SPI_FLASH_START_SECTOR;      
  flash.read(sector*SPI_FLASH_SECTOR_SIZE, buff,  count*SPI_FLASH_SECTOR_SIZE);
  return RES_OK;
}

/**
  * ��������: ������������д�뵽����FLASH��
  * �������: lun : ����
  *           buff����Ŵ�д�����ݵĻ�����ָ��
  *           sector��������ַ(LBA)
  *           count��������Ŀ
  * �� �� ֵ: DSTATUS���������
  * ˵    ��: ��
  */
#if _USE_WRITE == 1
DRESULT SPIFLASH_write(BYTE lun, const BYTE *buff, DWORD sector, UINT count)
{
  uint32_t write_addr; 
  /* ����ƫ��2MB���ⲿFlash�ļ�ϵͳ�ռ����SPI Flash����6MB�ռ� */
  sector+=SPI_FLASH_START_SECTOR;
  write_addr = sector*SPI_FLASH_SECTOR_SIZE;    
  flash.earse(write_addr);
  flash.write(write_addr , (uint8_t *)buff,count*SPI_FLASH_SECTOR_SIZE);
  return RES_OK;
}
#endif /* _USE_WRITE == 1 */

/**
  * ��������: ����������Ʋ���(I/O control operation)
  * �������: lun : ����
  *           cmd����������
  *           buff����Ŵ�д����߶�ȡ���ݵĻ�����ָ��
  * �� �� ֵ: DSTATUS���������
  * ˵    ��: ��
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
  
  /* ��ȡ����FLASH��������Ŀ(DWORD) */
  case GET_SECTOR_COUNT :
    *(DWORD * )buff = SPI_FLASH_SECTOR_COUNT;        
    res = RES_OK;
    break;
  
  /* ��ȡ��д������С(WORD) */
  case GET_SECTOR_SIZE :
    *(WORD * )buff = SPI_FLASH_SECTOR_SIZE;
    res = RES_OK;
    break;
  
  /* ��ȡ�������С(DWORD) */
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

/******************* (C) COPYRIGHT 2015-2020 ӲʯǶ��ʽ�����Ŷ� *****END OF FILE****/

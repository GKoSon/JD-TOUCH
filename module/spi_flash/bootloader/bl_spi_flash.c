#include "spi_flash.h"
#include "bsp.h"


uint8_t fb[4096];


static void *spiFlashPort = NULL;



static void spi_flash_write_enable(void)
{
	halSpi.chip_select(spiFlashPort , ENABLE);
	halSpi.send_reveive_byte(spiFlashPort ,FLASH_CMD_WREN);
	halSpi.chip_select(spiFlashPort , DISABLE);
}

static void spi_flash_wait_for_write_end(void)
{
	uint8_t flashstatus = 0;
	halSpi.chip_select(spiFlashPort , ENABLE);
	halSpi.send_reveive_byte(spiFlashPort ,FLASH_CMD_RDSR);
	do
    {
        flashstatus = halSpi.send_reveive_byte(spiFlashPort ,FLASH_DUMMY_BYTE);
	}
	while ((flashstatus & FLASH_WIP_FLAG) == SET); 
    
	halSpi.chip_select(spiFlashPort , DISABLE);
}

static void spi_flash_erase_sector(uint32_t sectorAddr)
{
    uint8_t cmd[4] ={0x00};
     
    spi_flash_write_enable();
        
    cmd[0] = FLASH_CMD_SE;
    cmd[1] = (sectorAddr & 0xFF0000) >> 16;
    cmd[2] = (sectorAddr& 0xFF00) >> 8;
    cmd[3] = sectorAddr & 0xFF;
    
    halSpi.chip_select(spiFlashPort , ENABLE);
    
    HAL_SPI_Transmit(spiFlashPort, cmd, 4, 1000); 
    
	halSpi.chip_select(spiFlashPort , DISABLE);
    
	spi_flash_wait_for_write_end();
    

}


static void spi_flash_write_page(uint8_t* buffer, uint32_t writeAddr, uint16_t numByteToWrite)
{
    uint8_t cmd[4] ={0x00};

    
    spi_flash_write_enable();
        
    cmd[0] = FLASH_CMD_WRITE;
    cmd[1] = (writeAddr & 0xFF0000) >> 16;
    cmd[2] = (writeAddr& 0xFF00) >> 8;
    cmd[3] = writeAddr & 0xFF;
    
    halSpi.chip_select(spiFlashPort , ENABLE);
 
    HAL_SPI_Transmit(spiFlashPort, cmd, 4, 1000); 
    
    HAL_SPI_Transmit(spiFlashPort, buffer,numByteToWrite, 1000);
    
	halSpi.chip_select(spiFlashPort , DISABLE);
    
	spi_flash_wait_for_write_end();
    

}

static void spi_flash_write_buffer(uint32_t writeAddr, uint8_t* buffer, uint16_t numByteToWrite)
{

	uint8_t NumOfPage = 0, NumOfSingle = 0, Addr = 0, count = 0, temp = 0;
  
	Addr = writeAddr % FLASH_SPI_PAGESIZE;
	count = FLASH_SPI_PAGESIZE - Addr;
	NumOfPage =  numByteToWrite / FLASH_SPI_PAGESIZE;
	NumOfSingle = numByteToWrite % FLASH_SPI_PAGESIZE;

   
	if (Addr == 0)
    {
		if (NumOfPage == 0)
        {
			spi_flash_write_page(buffer, writeAddr, numByteToWrite);
		}
		else
        {
			while (NumOfPage--)
            {
				spi_flash_write_page(buffer, writeAddr, FLASH_SPI_PAGESIZE);
				writeAddr +=  FLASH_SPI_PAGESIZE;
				buffer += FLASH_SPI_PAGESIZE;
			}
			spi_flash_write_page(buffer, writeAddr, NumOfSingle);
		}
	}
	else
    {
		if (NumOfPage == 0)
        {
			if (NumOfSingle > count)
            {
				temp = NumOfSingle - count;
				spi_flash_write_page(buffer, writeAddr, count);
				writeAddr +=  count;
				buffer += count;
				spi_flash_write_page(buffer, writeAddr, temp);
			}
			else
            {
				spi_flash_write_page(buffer, writeAddr, numByteToWrite);
			}
		}
		else
        {
			numByteToWrite -= count;
			NumOfPage =  numByteToWrite / FLASH_SPI_PAGESIZE;
			NumOfSingle = numByteToWrite % FLASH_SPI_PAGESIZE;
			spi_flash_write_page(buffer, writeAddr, count);
			writeAddr +=  count;
			buffer += count;
			while (NumOfPage--)
            {
				spi_flash_write_page(buffer, writeAddr, FLASH_SPI_PAGESIZE);
				writeAddr +=  FLASH_SPI_PAGESIZE;
				buffer += FLASH_SPI_PAGESIZE;
			}
			if (NumOfSingle != 0)
            {
				spi_flash_write_page(buffer, writeAddr, NumOfSingle);
			}
		}
	}

}

static void spi_flash_read_buffer(uint32_t readAddr,uint8_t* buffer,  uint16_t NumByteToRead)
{
    uint8_t cmd[4] ={0x00};
    

    cmd[0] = FLASH_CMD_READ;
    cmd[1] = (readAddr & 0xFF0000) >> 16;
    cmd[2] = (readAddr& 0xFF00) >> 8;
    cmd[3] = readAddr & 0xFF;
    
    halSpi.chip_select(spiFlashPort , ENABLE);
    
    HAL_SPI_Transmit(spiFlashPort, cmd, 4, 1000); 

    if( HAL_SPI_Receive(spiFlashPort, buffer,NumByteToRead, 1000) != HAL_OK)
    {
        cmd[0] = 0x01 ;
    }

	halSpi.chip_select(spiFlashPort , DISABLE);
    

}

static uint32_t spi_flash_read_chip_id(void)
{
	
	uint32_t Temp = 0, Temp0 = 0, Temp1 = 0, Temp2 = 0;

    halSpi.chip_select(spiFlashPort , ENABLE);
    halSpi.send_reveive_byte(spiFlashPort ,0x9F );
    
	Temp0 = halSpi.send_reveive_byte(spiFlashPort ,FLASH_DUMMY_BYTE);
	Temp1 = halSpi.send_reveive_byte(spiFlashPort ,FLASH_DUMMY_BYTE);
	Temp2 = halSpi.send_reveive_byte(spiFlashPort ,FLASH_DUMMY_BYTE);
    
	halSpi.chip_select(spiFlashPort , DISABLE);
    
	Temp = (Temp0 << 16) | (Temp1 << 8) | Temp2;

    log(DEBUG,"falsh id = %d\n" , Temp);
	return Temp;
}


static uint8_t spi_flash_read(uint32_t addr,uint8_t* buffer,  uint16_t length)
{
    uint8_t result = TRUE;
    

    spi_flash_read_buffer(addr , buffer , length);

    
    return result;
}

static uint8_t spi_flash_write(uint32_t addr,uint8_t* buffer,  uint16_t length)
{
    uint8_t result = TRUE;
    

    spi_flash_write_buffer(addr , buffer , length);

    
    return result;
}


static uint8_t spi_flash_erase(uint32_t sectorAddr)
{
    uint8_t result = TRUE;
    

    spi_flash_erase_sector(sectorAddr);

    
    return result;
}


spi_flash_type  flash=
{
    .earse = spi_flash_erase,
    .read_chip_id = spi_flash_read_chip_id,
    .write = spi_flash_write,
    .read = spi_flash_read,
};


void spi_flash_init( void )
{
    spiFlashPort = halSpi.open("spi1");
    if( spiFlashPort == NULL)
    {
        return ;
    }
    
    flash_sector_init();

    halSpi.init(spiFlashPort);

   
}

MODULES_INIT_EXPORT(spi_flash_init , "spi flash");



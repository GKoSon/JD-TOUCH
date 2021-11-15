#include "string.h"
#include "stdint.h"
#include "err_log.h"
#include "unit.h"
#include "spi_flash.h"
#include "component.h"



errInfoType	errInfo;

static uint8_t dev_err_log_read_flash(uint32_t addr,uint8_t* buffer,  uint16_t length)
{
	if( flash.get_lock() == TRUE )
	{
		flash.read(addr , buffer , length);

		flash.release_lock();
	}

	 return TRUE;
}

static uint8_t dev_err_log_write_flash(uint32_t addr,uint8_t* buffer,  uint16_t length)
{
	if( flash.get_lock() == TRUE )
	{
		flash.write(addr , buffer , length);

		flash.release_lock();
	}

	return TRUE;

}


static uint8_t dev_err_log_erase_flash(uint32_t sectorAddr)
{
	if( flash.get_lock() == TRUE )
	{
		flash.earse(sectorAddr);

		flash.release_lock();
	}

	return TRUE;

}


void err_log_info_save( void )
{
	uint32_t addr = 0;
	
	
	if( (errInfo.cnt % (FLASH_SPI_BLOCKSIZE/sizeof( errInfoType ))) == 0)
	{
		dev_err_log_erase_flash(ERR_LOG_INFO_ADDR);
	}
	
	addr = ERR_LOG_INFO_ADDR + (errInfo.cnt % (FLASH_SPI_BLOCKSIZE/sizeof( errInfoType )))*sizeof( errInfoType );
	
	dev_err_log_write_flash( addr , (uint8_t *)&errInfo , sizeof( errInfoType ) );	
}



void err_log_format( void )
{

    log(INFO,"开始清空错误日志\n");

    errInfo.cnt = 0;
    errInfo.pos = 0;

    err_log_info_save();

    log(INFO,"清空错误日志完成\n");

}


	
void err_log_init( void )
{
	uint32_t cnt = 0 ; 
	uint32_t pos = 0;
	uint32_t addr = ERR_LOG_INFO_ADDR;
	errInfoType	errInfoTemp;
	
	for( uint32_t i = 0 ; i < FLASH_SPI_BLOCKSIZE/sizeof( errInfoType ); i++)
	{
		dev_err_log_read_flash( addr , (uint8_t *)&errInfoTemp , sizeof( errInfoType ) );	
		
		if( errInfoTemp.cnt != 0xFFFFFFFF)
		{
			if( cnt < errInfoTemp.cnt )
			{
				pos = errInfoTemp.pos;
				cnt = errInfoTemp.cnt;
			}
		}
		
		addr +=  sizeof( errInfoType );
	}
		
	errInfo.cnt = cnt;
	errInfo.pos = pos;
	
	log(DEBUG,"当前存储错误日志的flash擦写次数 = %u，当前存储位置 POS=%d \n" ,  errInfo.cnt,errInfo.pos); 
	    
}

void err_log(errCodeEnum errCode,uint8_t *errData , uint32_t errLen)
{
	errMsgType	err;
	uint32_t addr = 0;
	uint32_t numTotle = (ERR_END_ADDR - ERR_LOG_DATA_ADDR)/sizeof(errMsgType);
	
	
	memset((uint8_t *)&err , 0x00 , sizeof(errMsgType));
	
	if( errLen > sizeof(err.buffer) )
	{
		log(WARN,"错误日志存储长度过长, len = %d\n" , errLen);
		return ;
	}
	err.time  = rtc.read_stamp();
	memcpy(err.buffer , errData , errLen );

    //log(DEBUG,"在%d写入错误日志， 当前擦写次数=%d\n", errInfo.cnt ,errInfo.pos); 

	addr = ERR_LOG_DATA_ADDR + errInfo.pos*sizeof(errMsgType);
	if( addr % FLASH_SPI_BLOCKSIZE == 0)
	{
		dev_err_log_erase_flash(addr);
	}
	
	dev_err_log_write_flash( addr , (uint8_t *)&err ,  sizeof(errMsgType));
	
	errInfo.cnt++;
    errInfo.pos++;
    
    if( errInfo.pos >= numTotle)
    {
        errInfo.pos = 0;
    }
	
	err_log_info_save();
}



void err_log_read(void)
{
	errMsgType	errMsg;
	uint32_t numTotle = (ERR_END_ADDR - ERR_LOG_DATA_ADDR)/sizeof(errMsgType) , printfNum=0;
	uint32_t addr =  ERR_LOG_DATA_ADDR;

	log(DEBUG,"调试日志打印:\n");

	if( errInfo.cnt > numTotle )
	{
		printfNum = numTotle;
	}
	else
	{
		printfNum = errInfo.pos;
	}
	
	for( uint32_t i = 0 ; i <  printfNum ; i++)
	{
		memset(&errMsg , 0x00 , sizeof(errMsgType));
		
		dev_err_log_read_flash(addr , (uint8_t *)&errMsg , sizeof(errMsgType));
		addr += sizeof(errMsgType);
		
		if(errMsg.time != 0xFFFFFFFF)
		{
			rtcTimeType tim;
    
    		rtc.stamp_to_time(&tim ,errMsg.time );
    
    		printf("[%02d-%02d-%02d %02d:%02d:%02d]" , tim.year,tim.mon,tim.day,tim.hour , tim.min , tim.sec);
			printf("%s" ,errMsg.buffer);
		}
        HAL_IWDG_Refresh(&hiwdg);
	}

	log(DEBUG,"调试日志打印结束\n");
}



//INIT_EXPORT(err_log_init  , "err log");






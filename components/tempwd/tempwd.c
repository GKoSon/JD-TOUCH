#include <string.h>
#include "tempwd.h"
#include "unit.h"
#include "bsp_rtc.h"
#include "crc32.h"
#include "syscfg.h"

/*
uint8_t dev_pwd_read_flash(uint32_t addr,uint8_t* buffer,  uint16_t length)
{
	if( flash.get_lock() == TRUE )
	{
		flash.read(addr , buffer , length);

		flash.release_lock();
	}

	 return TRUE;
}

static uint8_t dev_pwd_write_flash(uint32_t addr,uint8_t* buffer,  uint16_t length)
{
	if( flash.get_lock() == TRUE )
	{

		flash.write(addr , buffer , length);

		flash.release_lock();
	}

	return TRUE;

}


static uint8_t dev_pwd_erase_flash(uint32_t sectorAddr)
{
	if( flash.get_lock() == TRUE )
	{
		flash.earse(sectorAddr);

		flash.release_lock();
	}

	return TRUE;

}
*/

void pwd_clear_all( void )
{
    uint32_t addr = 0;
    
    uint32_t page = PWD_MAX_NUM*PWD_INDEX_SIZE/FLASH_SPI_BLOCKSIZE;

    log(INFO,"开始清空临时密码\n");
    
    if( (PWD_MAX_NUM*PWD_INDEX_SIZE)%FLASH_SPI_BLOCKSIZE !=0)
    {
         page = PWD_MAX_NUM*PWD_INDEX_SIZE/FLASH_SPI_BLOCKSIZE+1;
    }
    else
    {
        page = PWD_MAX_NUM*PWD_INDEX_SIZE/FLASH_SPI_BLOCKSIZE;
    }
    
    flash.get_lock();

    for(uint8_t i = 0 ; i < page ; i++)
    {
        addr = PASSWORD_BOOT_ADDR + i*FLASH_SPI_BLOCKSIZE;

        //log(INFO,"清楚临时密码 address = %x ,page = %d\n" , addr ,addr/FLASH_SPI_BLOCKSIZE);
        printf("...");
        flash.earse(addr);
        
        HAL_IWDG_Refresh(&hiwdg);
    }
    flash.release_lock();
    log(INFO,"清空临时密码完成\n");
}


void pwd_read_data(uint32_t index, tempwdType *pwd)
{
    flash.read( PASSWORD_DATA_ADDR+index*PWD_SIZE , (uint8_t *)pwd , PWD_SIZE);
}


void pwd_del_index( uint32_t index)
{
    uint32_t addr = 0 , page =0 ;

    addr  = PASSWORD_BOOT_ADDR+index*PWD_INDEX_SIZE;

    page = addr/FLASH_SPI_BLOCKSIZE;

    flash.get_lock();
    
    flash.read( page*FLASH_SPI_BLOCKSIZE , fb , FLASH_SPI_BLOCKSIZE);

    *(uint32_t *)(fb + (index*PWD_INDEX_SIZE)%FLASH_SPI_BLOCKSIZE) = EMPTY_ID;

    flash.earse(page*FLASH_SPI_BLOCKSIZE);

    flash.write( page*FLASH_SPI_BLOCKSIZE, fb, FLASH_SPI_BLOCKSIZE);
    
    flash.release_lock();
}

void pwd_del_index_no_lock( uint32_t index)
{
    uint32_t addr = 0 , page =0 ;

    addr  = PASSWORD_BOOT_ADDR+index*PWD_INDEX_SIZE;

    page = addr/FLASH_SPI_BLOCKSIZE;

    flash.read( page*FLASH_SPI_BLOCKSIZE , fb , FLASH_SPI_BLOCKSIZE);

    *(uint32_t *)(fb + (index*PWD_INDEX_SIZE)%FLASH_SPI_BLOCKSIZE) = EMPTY_ID;

    flash.earse(page*FLASH_SPI_BLOCKSIZE);

    flash.write( page*FLASH_SPI_BLOCKSIZE, fb, FLASH_SPI_BLOCKSIZE);

}

void pwd_add_index( uint32_t index , uint32_t pwd)
{
    uint32_t addr = 0 , page =0;

    addr  = PASSWORD_BOOT_ADDR+index*PWD_INDEX_SIZE;

    page = addr/FLASH_SPI_BLOCKSIZE;

    flash.get_lock();
    
    flash.read( page*FLASH_SPI_BLOCKSIZE , fb , FLASH_SPI_BLOCKSIZE);

    *(uint32_t *)(fb + (index*PWD_INDEX_SIZE)%FLASH_SPI_BLOCKSIZE) = pwd;

    flash.earse(page*FLASH_SPI_BLOCKSIZE);

    flash.write( page*FLASH_SPI_BLOCKSIZE, fb, FLASH_SPI_BLOCKSIZE);
    
    flash.release_lock();

}

int32_t pwd_find_index( uint32_t ID )
{
    uint32_t tempID;
    flash.get_lock();
    for(uint32_t i = 0 ; i < PWD_MAX_NUM ; i++)
    {
        flash.read(PASSWORD_BOOT_ADDR+i*PWD_INDEX_SIZE , (uint8_t *)&tempID , 4);
        if( tempID == ID)
        {
            flash.release_lock();
            return i;
        }
    }
    flash.release_lock();
    return PWD_NULL_ID;
}

void pwd_add_data( uint32_t index , tempwdType *pwd)
{
    uint32_t addr = 0 , page =0;

    addr  = PASSWORD_DATA_ADDR+index*PWD_SIZE;

    page = addr/FLASH_SPI_BLOCKSIZE;

    flash.get_lock();
    
    flash.read( page*FLASH_SPI_BLOCKSIZE , fb , FLASH_SPI_BLOCKSIZE);

    memcpy(fb + (index*PWD_SIZE)%FLASH_SPI_BLOCKSIZE , pwd , PWD_SIZE);

    flash.earse(page*FLASH_SPI_BLOCKSIZE);

    flash.write( page*FLASH_SPI_BLOCKSIZE, fb, FLASH_SPI_BLOCKSIZE);
    
    flash.release_lock();
}


int32_t pwd_find(uint32_t pwd, tempwdType *outPwd)
{
    uint32_t tempPwd = 0;
    uint32_t tick = 0;
    
    tick = HAL_GetTick();
    
    flash.get_lock();
      
    for(uint32_t i = 0 ; i < PWD_MAX_NUM ; i++)
    {
        flash.read(PASSWORD_BOOT_ADDR+i*PWD_INDEX_SIZE , (uint8_t *)&tempPwd , 4);
        if( tempPwd == pwd)
        {
            pwd_read_data( i , outPwd);
            if( pwd == outPwd->pwd)
            {
                log(DEBUG,"在 %d 位置查询到临时密码 PWD = %x , 查询时间=%d\n" , i, 
                    outPwd->pwd , HAL_GetTick()-tick);
                
                flash.release_lock();
                
                return i;
            }
        }
    }

    flash.release_lock();
    return PWD_NULL_ID;
}

int32_t pwd_add(tempwdType *pwd)
{
    uint32_t index =0;
    tempwdType   pwdTemp;

    //uint32_t tick = 0;
    
    //tick = HAL_GetTick();
    
    if( (index = pwd_find(pwd->pwd ,&pwdTemp )) == PWD_NULL_ID)
    {
        log(DEBUG,"没有找到同样的临时密码 ， 新增密码\n");

        if( (index = pwd_find_index(EMPTY_ID)) == PWD_NULL_ID)
        {
            log(WARN,"临时密码存储已满。\n");
            return PWD_FULL;
        }
        //log(DEBUG,"找到一个空闲位置，位置:%d,时间=%d\n" , index , HAL_GetTick()-tick);
        pwd_add_index(index , pwd->pwd);
        
    }
    else
    {
        log(DEBUG,"存在同样的临时密码，位置：%d \n" , index);
        
        if( pwd->time == pwdTemp.time)
        {
            log(DEBUG,"写入的数据和存储的数据一样，不用重新写入\n");
            return PWD_EXIST;
        }
    
    }

    pwd_add_data(index , pwd);
    
    return PWD_SUCCESS;
}

int32_t pwd_del( uint32_t pwd)
{
    uint32_t index =0;
    tempwdType   pwdTemp;

    if( (index = pwd_find(pwd,&pwdTemp )) == PWD_NULL_ID)
    {
        log(DEBUG,"没有找到临时密码,删除失败 password = %x \n" ,pwd);
        return PWD_NULL_ID;
    }
    log(DEBUG," 在%d 位置删除 PWD = %x \n" ,index ,pwd);
    pwd_del_index(index);

    return PWD_SUCCESS;

}


void pwd_clear_overdue( void )
{
    tempwdType pwd;
    int32_t tempPwd = 0;
    
    uint32_t stamp = rtc.read_stamp();
    if( config.read(CFG_SYS_UPDATA_TIME , NULL) == TRUE )
    {
        flash.get_lock();
        
        for(uint32_t i = 0 ; i <  PWD_MAX_NUM ; i++)
        {
            flash.read(PASSWORD_BOOT_ADDR+i*PWD_INDEX_SIZE , (uint8_t *)&tempPwd , 4);
            if( tempPwd != EMPTY_ID)
            {
                pwd_read_data( i , &pwd);
                if( pwd.time < stamp)
                {
                    //pwd_del(pwd.pwd);
                    pwd_del_index_no_lock(i);
                    log(DEBUG,"删除过期一次性密码，POS=%d , pwd=%04x , TIME:%u\n" , i , pwd.pwd, pwd.time);
                }
            }
             HAL_IWDG_Refresh(&hiwdg);
        }
        flash.release_lock();
    }
}

void pwd_printf_msg( void )
{
    int32_t  tempPwd=0 , cnt =0;

    pwd_clear_overdue();
    
    flash.get_lock();
    
    for(uint32_t i = 0 ; i < PWD_MAX_NUM ; i++)
    {
        flash.read(PASSWORD_BOOT_ADDR+i*PWD_INDEX_SIZE , (uint8_t *)&tempPwd , 4);
        if(tempPwd != 0xFFFFFFFF)
        {
            cnt++;
        }
    }
    flash.release_lock();
    log(DEBUG,"设备目前临时密码数量:%d\n" , cnt);
    
}


tempwdOpstype tempwd =
{
    .find = pwd_find,
    .add = pwd_add,
    .del = pwd_del,
    .clear = pwd_clear_all,
    .clear_overdue = pwd_clear_overdue,
    .show = pwd_printf_msg,
};




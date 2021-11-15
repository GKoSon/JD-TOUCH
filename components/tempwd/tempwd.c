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

    log(INFO,"��ʼ�����ʱ����\n");
    
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

        //log(INFO,"�����ʱ���� address = %x ,page = %d\n" , addr ,addr/FLASH_SPI_BLOCKSIZE);
        printf("...");
        flash.earse(addr);
        
        HAL_IWDG_Refresh(&hiwdg);
    }
    flash.release_lock();
    log(INFO,"�����ʱ�������\n");
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
                log(DEBUG,"�� %d λ�ò�ѯ����ʱ���� PWD = %x , ��ѯʱ��=%d\n" , i, 
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
        log(DEBUG,"û���ҵ�ͬ������ʱ���� �� ��������\n");

        if( (index = pwd_find_index(EMPTY_ID)) == PWD_NULL_ID)
        {
            log(WARN,"��ʱ����洢������\n");
            return PWD_FULL;
        }
        //log(DEBUG,"�ҵ�һ������λ�ã�λ��:%d,ʱ��=%d\n" , index , HAL_GetTick()-tick);
        pwd_add_index(index , pwd->pwd);
        
    }
    else
    {
        log(DEBUG,"����ͬ������ʱ���룬λ�ã�%d \n" , index);
        
        if( pwd->time == pwdTemp.time)
        {
            log(DEBUG,"д������ݺʹ洢������һ������������д��\n");
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
        log(DEBUG,"û���ҵ���ʱ����,ɾ��ʧ�� password = %x \n" ,pwd);
        return PWD_NULL_ID;
    }
    log(DEBUG," ��%d λ��ɾ�� PWD = %x \n" ,index ,pwd);
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
                    log(DEBUG,"ɾ������һ�������룬POS=%d , pwd=%04x , TIME:%u\n" , i , pwd.pwd, pwd.time);
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
    log(DEBUG,"�豸Ŀǰ��ʱ��������:%d\n" , cnt);
    
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




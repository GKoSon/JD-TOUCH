#include "open_log.h"
#include "unit.h"
#include "component.h"
#include "spi_flash.h"
#include "bsp_rtc.h"
#include "net.h"
#include "timer.h"
#include "sysCfg.h"
#include "swipeTag.h"
#include "beep.h"
#include "mqtt_task.h"
#include "tsl_mqtt.h"
#include "sysCntSave.h"

sysCntSaveType  sysCnt;
uint32_t writePos = 0;

static uint8_t syscnt_read_flash(uint32_t addr,uint8_t* buffer,  uint16_t length)
{
    if( flash.get_lock() == TRUE )
    {
        flash.read(addr , buffer , length);

        flash.release_lock();
    }

     return TRUE;
}

static uint8_t syscnt_write_flash(uint32_t addr,uint8_t* buffer,  uint16_t length)
{
    if( flash.get_lock() == TRUE )
    {
        flash.write(addr , buffer , length);
        flash.release_lock();
    }

    return TRUE;

}


static uint8_t syscnt_erase_flash(uint32_t sectorAddr)
{
    if( flash.get_lock() == TRUE )
    {
        flash.earse(sectorAddr);

        flash.release_lock();
    }

    return TRUE;

}


void syscnt_write_list_time( uint32_t time)
{
    if( writePos == 0 )
    {
        syscnt_erase_flash(SYS_CNT_ADDR);
    }
    sysCnt.listUpdataTime = time;
    syscnt_write_flash(SYS_CNT_ADDR+writePos*sizeof(sysCntSaveType) , (uint8_t *)&sysCnt , sizeof(sysCntSaveType));
    
    if( ++writePos == FLASH_SPI_BLOCKSIZE/sizeof(sysCntSaveType) )
    {
        writePos = 0;
    }
}


void syscnt_write_oncepwd_time( uint32_t time)
{
    if( writePos == 0 )
    {
        syscnt_erase_flash(SYS_CNT_ADDR);
    }
    sysCnt.oncePwdTime = time;
    syscnt_write_flash(SYS_CNT_ADDR+writePos*sizeof(sysCntSaveType) , (uint8_t *)&sysCnt , sizeof(sysCntSaveType));
    
    if( ++writePos == FLASH_SPI_BLOCKSIZE/sizeof(sysCntSaveType) )
    {
        writePos = 0;
    }
}




void syscnt_write_user_time( uint32_t time)
{
    if( writePos == 0 )
    {
        syscnt_erase_flash(SYS_CNT_ADDR);
    }
    sysCnt.listRainbowUseTime = time;
    syscnt_write_flash(SYS_CNT_ADDR+writePos*sizeof(sysCntSaveType) , (uint8_t *)&sysCnt , sizeof(sysCntSaveType));
    
    if( ++writePos == FLASH_SPI_BLOCKSIZE/sizeof(sysCntSaveType) )
    {
        writePos = 0;
    }
}


void syscnt_clear_all( void )
{
    syscnt_erase_flash(SYS_CNT_ADDR);
    
    sysCnt.listRainbowUseTime = 1;
    sysCnt.listUpdataTime = 1;
    sysCnt.oncePwdTime = 1;
    
    syscnt_write_flash(SYS_CNT_ADDR+writePos*sizeof(sysCntSaveType) , (uint8_t *)&sysCnt , sizeof(sysCntSaveType));
    
    writePos++;
    
    log(INFO,"list updata time=%u\n" , sysCnt.listUpdataTime);
}


void syscnt_init( void )
{
    sysCntSaveType  sysCntTemp;
    
    uint32_t maxCnt = FLASH_SPI_BLOCKSIZE/sizeof(sysCntSaveType);
    
    memset(&sysCnt , 0 , sizeof(sysCntSaveType));
    
    for(uint32_t i = 0 ; i < maxCnt ; i++ )
    {
        syscnt_read_flash(SYS_CNT_ADDR+i*sizeof(sysCntSaveType) , (uint8_t *)&sysCntTemp , sizeof(sysCntSaveType));
        if( sysCntTemp.listUpdataTime != 0xFFFFFFFF )
        {
            sysCnt.listUpdataTime = MAX(sysCnt.listUpdataTime , sysCntTemp.listUpdataTime);
        }
        
        if( sysCntTemp.listRainbowUseTime != 0xFFFFFFFF )
        {
            sysCnt.listRainbowUseTime = MAX(sysCnt.listRainbowUseTime , sysCntTemp.listRainbowUseTime);
        }
        
        if( sysCntTemp.oncePwdTime != 0xFFFFFFFF )
        {
            sysCnt.oncePwdTime = MAX(sysCnt.oncePwdTime , sysCntTemp.oncePwdTime);
        }
        
    }
    //syscnt_clear_all();
    log(DEBUG,"黑白名单更新时间为:%d , rainbow 用户列表更新时间为:%d , 一次性密码时间:%d\n" , sysCnt.listUpdataTime , sysCnt.listRainbowUseTime , sysCnt.oncePwdTime);
}


INIT_EXPORT(syscnt_init , "syscnt");
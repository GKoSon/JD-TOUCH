#include <string.h>
#include "permi_list.h"
#include "spi_flash.h"
#include "beep.h"
#include "unit.h"
#include "crc32.h"
#include "sysCfg.h"
#include "sysCntSave.h"

/*20000个int
地址的计算再 flash_sector_init 里面已经做了 
可以当时flash_sector_init就做一个数组保存下面 
每个部分的起始地址和该地址延续多少页 
方便这里使用 */
void permi_list_clear_all( void )
{
    uint32_t addr = 0;
    uint32_t page = PERMI_LIST_MAX*PERMI_LISD_INDEX_SIZE/FLASH_SPI_BLOCKSIZE;
    
    
    log(INFO,"开始清空黑白名单\n");
    
    if( (PERMI_LIST_MAX*PERMI_LISD_INDEX_SIZE)%FLASH_SPI_BLOCKSIZE !=0)
    {
         page = PERMI_LIST_MAX*PERMI_LISD_INDEX_SIZE/FLASH_SPI_BLOCKSIZE+1;
    }
    else
    {
         page = PERMI_LIST_MAX*PERMI_LISD_INDEX_SIZE/FLASH_SPI_BLOCKSIZE;
    }
    flash.get_lock();
    for(uint8_t i = 0 ; i < page ; i++)
    {
        addr = PERMI_LIST_BOOT_ADDR + i*FLASH_SPI_BLOCKSIZE;
        
        //log(INFO,"Clear permission address = %x ,page = %d\n" , addr ,addr/FLASH_SPI_BLOCKSIZE);
        printf("... ");
        flash.earse(addr);
        HAL_IWDG_Refresh(&hiwdg);
    }
    flash.release_lock();
    log(INFO,"[PERMI]清空黑白名单完成\n");
    
    syscnt_clear_all();
}

/*再索引区  通过index读出内容返回*/
static uint32_t permiList_read_id( uint32_t index)
{
    uint32_t tempID = 0;
    flash.read(PERMI_LIST_BOOT_ADDR+index*PERMI_LISD_INDEX_SIZE , (uint8_t *)&tempID , PERMI_LISD_INDEX_SIZE);

    return tempID;
}

/*再数据区  通过index读出内容返回*/
static void permiList_read_data(uint32_t index, permiListType *list)
{
    flash.read( PERMI_LIST_DATA_ADDR+index*PERMI_LIST_SIZE , (uint8_t *)list , PERMI_LIST_SIZE);
}

/*再索引区  通过index删除指定内容 所谓删除是写int为0XFFFF FFFF*/
static void permiList_del_index( uint32_t index)
{
    uint32_t addr = 0 , page =0 ;
    
    addr  = PERMI_LIST_BOOT_ADDR+index*PERMI_LISD_INDEX_SIZE;
    
    page = addr/FLASH_SPI_BLOCKSIZE;

    flash.get_lock();
    
    flash.read( page*FLASH_SPI_BLOCKSIZE , fb , FLASH_SPI_BLOCKSIZE);

    *(uint32_t *)(fb + (index*PERMI_LISD_INDEX_SIZE)%FLASH_SPI_BLOCKSIZE) = EMPTY_ID;
    
    flash.earse(page*FLASH_SPI_BLOCKSIZE);
    
    flash.write( page*FLASH_SPI_BLOCKSIZE, fb, FLASH_SPI_BLOCKSIZE);

    flash.release_lock();
}


static void permiList_del_index_no_lock( uint32_t index)
{
    uint32_t addr = 0 , page =0 ;
    
    addr  = PERMI_LIST_BOOT_ADDR+index*PERMI_LISD_INDEX_SIZE;
    
    page = addr/FLASH_SPI_BLOCKSIZE;

    flash.read( page*FLASH_SPI_BLOCKSIZE , fb , FLASH_SPI_BLOCKSIZE);

    *(uint32_t *)(fb + (index*PERMI_LISD_INDEX_SIZE)%FLASH_SPI_BLOCKSIZE) = EMPTY_ID;
    
    flash.earse(page*FLASH_SPI_BLOCKSIZE);
    
    flash.write( page*FLASH_SPI_BLOCKSIZE, fb, FLASH_SPI_BLOCKSIZE);

}

/*写一个数据进来 本质保存的是uint32_t */
static void permiList_add_index( uint32_t index , uint64_t cardNumber)
{
    uint32_t addr = 0 , page =0 , ID =0;
    
    addr  = PERMI_LIST_BOOT_ADDR+index*PERMI_LISD_INDEX_SIZE;
    
    page = addr/FLASH_SPI_BLOCKSIZE;

    ID = crc32((uint8_t *)&cardNumber , 8);

    flash.get_lock();
    
    flash.read( page*FLASH_SPI_BLOCKSIZE , fb , FLASH_SPI_BLOCKSIZE);

    *(uint32_t *)(fb + (index*PERMI_LISD_INDEX_SIZE)%FLASH_SPI_BLOCKSIZE) = ID;
    
    flash.earse(page*FLASH_SPI_BLOCKSIZE);
    
    flash.write( page*FLASH_SPI_BLOCKSIZE, fb, FLASH_SPI_BLOCKSIZE);

    flash.release_lock();
    
}

/*根据内容找到索引*/
static int32_t permiList_find_index( uint32_t data )
{
    uint32_t tempdata;

    flash.get_lock();
    for(uint32_t i = 0 ; i < PERMI_LIST_MAX ; i++)
    {
        flash.read(PERMI_LIST_BOOT_ADDR+i*PERMI_LISD_INDEX_SIZE , (uint8_t *)&tempdata , 4);
        
        if( tempdata == data)
        {
            flash.release_lock();
            return i;
        }
    }
    flash.release_lock();
    return FIND_NULL_ID;
}

/*再数据区域 根据给定的id写内容*/
static void permiList_add_data( uint32_t index , permiListType *list)
{
    uint32_t addr = 0 , page =0;
    
    addr  = PERMI_LIST_DATA_ADDR+index*PERMI_LIST_SIZE;
    
    page = addr/FLASH_SPI_BLOCKSIZE;

    flash.get_lock();
    
    flash.read( page*FLASH_SPI_BLOCKSIZE , fb , FLASH_SPI_BLOCKSIZE);
    
    memcpy(fb + (index*PERMI_LIST_SIZE)%FLASH_SPI_BLOCKSIZE , list , PERMI_LIST_SIZE);
    
    flash.earse(page*FLASH_SPI_BLOCKSIZE);
    
    flash.write( page*FLASH_SPI_BLOCKSIZE, fb, FLASH_SPI_BLOCKSIZE);

    flash.release_lock();
}

/*先在索引区寻找 根据给定的cardNumber一个一个比较 找到一样的 继续去数据区查找 返回pos  你可以假设第一个就是*/
static int32_t list_find_index( uint64_t cardNumber , permiListType *list)
{
    uint32_t tem = 0;
    uint32_t pos = 0;
    uint32_t ID =0 ;
    
    ID = crc32((uint8_t *)&cardNumber , 8);

    flash.get_lock();
    for(uint32_t addr = PERMI_LIST_BOOT_ADDR ; addr < PERMI_LIST_BOOT_ADDR+PERMI_LIST_MAX*PERMI_LISD_INDEX_SIZE; addr += 4096)
    {
        flash.read(addr , fb , 4096);
        for( uint16_t i = 0 ; i < 4096 / PERMI_LISD_INDEX_SIZE ; i++)
        {
            tem = 0;
            memcpy(&tem , fb+i*PERMI_LISD_INDEX_SIZE , PERMI_LISD_INDEX_SIZE);
            if(tem!=EMPTY_ID)log(INFO,"ID = %x ,tem = %x\n" , ID ,tem);
            if( tem == ID)
            {
                pos =  ((addr+i*PERMI_LISD_INDEX_SIZE)-PERMI_LIST_BOOT_ADDR)/PERMI_LISD_INDEX_SIZE;
                
                permiList_read_data( pos , list);
                if( cardNumber == list->ID)
                {
                    flash.release_lock();
                    return (pos);
                }
            }
        }
    }
    flash.release_lock();
    return FIND_NULL_ID;
}









/*先在索引区寻找 根据给定的cardNumber一个一个比较 找到一样的 继续去数据区查找 返回pos  你可以假设第一个就是*/
int32_t permiList_find(uint64_t cardNumber, permiListType *list)
{
    uint32_t pos =0;
    uint32_t tick = HAL_GetTick();
    
    if( (pos =list_find_index(cardNumber , list)) != FIND_NULL_ID)
    {        
        log(DEBUG,"[PERMI]在 %d 位置查询到卡片ID:%llx , 查询时间:%d\n" , pos, list->ID ,HAL_GetTick()-tick);
        return pos;
    }
    
    log(DEBUG,"[PERMI]该卡号不在黑白名单中，ID=%llx , 查询时间:%d\n",cardNumber,HAL_GetTick()-tick);
    
    return FIND_NULL_ID;
}

/*先找 再写 再索引区 数据区 都要写的*/
int32_t periList_add(permiListType *list)
{
    uint32_t index =0;
    permiListType   listTemp;
    
    if( (index = permiList_find(list->ID ,&listTemp )) == FIND_NULL_ID)
    {
        log(DEBUG,"[PERMI]当前黑白名单列表没有找到对应卡号，新增名单\n");
        
        if( (index = permiList_find_index(EMPTY_ID)) == FIND_NULL_ID)
        {
            log(WARN,"[PERMI]黑白名单数据已经满了。\n");
            return LIST_FULL;
        }
        log(DEBUG,"[PERMI]找到一个空闲位置，位置:%d\n" , index);
        permiList_add_index(index , list->ID);
    }
    else
    {
        log(DEBUG,"[PERMI]当前黑白名单列表可以找到对应卡号，位置：%d \n" , index);
    }
    
    memset( list->temp , 0x00 , 3);
    memset( listTemp.temp , 0x00 , 3);
    if( aiot_strcmp((uint8_t *)list , (uint8_t *)&listTemp , PERMI_LIST_SIZE) == TRUE )
    {
        log(DEBUG,"[PERMI]写入的数据和存储的数据一样，不用重新写入\n");
    }
    else
    {
        log(DEBUG , "[PERMI]数据将写入到 %d 位置中\n" , index);
        permiList_add_data(index , list);
    }
    return LIST_SUCCESS;
}

/*先找再删 只是操作索引的删除*/
int32_t permiList_del( uint64_t cardNumber)
{
    uint32_t index =0;
    permiListType   listTemp;
    
    if( (index = permiList_find(cardNumber,&listTemp )) == FIND_NULL_ID)
    {
        log(DEBUG,"[PERMI]列表中没有改名单,无需删除 ID = %llx \n" ,cardNumber);
    }
    else
    {
        log(DEBUG," [PERMI]在%d 位置删除 ID = %llx \n" ,index ,cardNumber);
        permiList_del_index(index);
    }
    return LIST_SUCCESS;
    
}

void permiList_show( void )
{
    uint32_t cnt = 0;
    uint32_t tempID = 0;
    flash.get_lock();
    for(uint32_t i = 0 ; i < PERMI_LIST_MAX ; i++)
    {
        tempID = permiList_read_id(i);
        if(tempID != EMPTY_ID)
        {
            log(DEBUG,"[PERMI]当前设备黑白名单第 %d 条是0X%08X\n" , cnt,tempID);
            cnt++;
        }
    }
    flash.release_lock();
    log(DEBUG,"[PERMI]当前设备黑白名单总条数:%d\n" , cnt);
}

permi_list_type permi = 
{
    .find = permiList_find, 
    .add = periList_add,
    .del = permiList_del,
    .clear = permi_list_clear_all,
    .show = permiList_show,
};





void permi_list_init( void )
{
    //char* cardNo = "40D1BB0133CE6498";//丁博身份证
    char* cardNo = "723F170A8F2102E0";//特斯连NFC
    permiListType list;

    
    //list.ID = 0x40D1BB0133CE6498;
    list.ID = atol64(cardNo);
    printf("[PERMI]permi_list_init 0x%llx \r\n",list.ID);
    list.time = 1900000000;
    list.status = LIST_WRITE;   
    periList_add(&list);
    
}

void permiList_clear_overdue( void )
{
    permiListType list;
    uint32_t deviceTime;
            
    if( config.read(CFG_SYS_UPDATA_TIME , NULL) == TRUE )
    {
        deviceTime = rtc.read_stamp();
        
        log(DEBUG,"[PERMI]当前设备时间戳:%d\n" , deviceTime);
        flash.get_lock();
        for(uint32_t i = 0 ; i < PERMI_LIST_MAX ; i++)
        {
            if(  permiList_read_id(i) != EMPTY_ID)
            {
                permiList_read_data(i, &list);
                if(list.time <  deviceTime)
                {
                    log(DEBUG,"[PERMI]删除过期黑白名单，POS=%d , ID=%llx , TIME:%u\n" , i , list.ID , list.time);
                    permiList_del_index_no_lock(i);
                }                

                if((list.status != LIST_BLACK)&&(list.status != LIST_WRITE))
                {
                    log(INFO,"[PERMI]黑白名单状态错误,status=%d , 删除POS=%d\n" , list.status ,i);
                    permiList_del_index_no_lock(i);
                }
            }
            HAL_IWDG_Refresh(&hiwdg);
        }
        flash.release_lock();
    }
    else
    {
        log(INFO,"[PERMI]设备还没有同步时间\n");
    }
}
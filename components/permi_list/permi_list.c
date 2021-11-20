#include <string.h>
#include "permi_list.h"
#include "spi_flash.h"
#include "beep.h"
#include "unit.h"
#include "crc32.h"
#include "sysCfg.h"
#include "sysCntSave.h"

/*20000��int
��ַ�ļ����� flash_sector_init �����Ѿ����� 
���Ե�ʱflash_sector_init����һ�����鱣������ 
ÿ�����ֵ���ʼ��ַ�͸õ�ַ��������ҳ 
��������ʹ�� */
void permi_list_clear_all( void )
{
    uint32_t addr = 0;
    uint32_t page = PERMI_LIST_MAX*PERMI_LISD_INDEX_SIZE/FLASH_SPI_BLOCKSIZE;
    
    
    log(INFO,"��ʼ��պڰ�����\n");
    
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
    log(INFO,"[PERMI]��պڰ��������\n");
    
    syscnt_clear_all();
}

/*��������  ͨ��index�������ݷ���*/
static uint32_t permiList_read_id( uint32_t index)
{
    uint32_t tempID = 0;
    flash.read(PERMI_LIST_BOOT_ADDR+index*PERMI_LISD_INDEX_SIZE , (uint8_t *)&tempID , PERMI_LISD_INDEX_SIZE);

    return tempID;
}

/*��������  ͨ��index�������ݷ���*/
static void permiList_read_data(uint32_t index, permiListType *list)
{
    flash.read( PERMI_LIST_DATA_ADDR+index*PERMI_LIST_SIZE , (uint8_t *)list , PERMI_LIST_SIZE);
}

/*��������  ͨ��indexɾ��ָ������ ��νɾ����дintΪ0XFFFF FFFF*/
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

/*дһ�����ݽ��� ���ʱ������uint32_t */
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

/*���������ҵ�����*/
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

/*���������� ���ݸ�����idд����*/
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

/*����������Ѱ�� ���ݸ�����cardNumberһ��һ���Ƚ� �ҵ�һ���� ����ȥ���������� ����pos  ����Լ����һ������*/
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









/*����������Ѱ�� ���ݸ�����cardNumberһ��һ���Ƚ� �ҵ�һ���� ����ȥ���������� ����pos  ����Լ����һ������*/
int32_t permiList_find(uint64_t cardNumber, permiListType *list)
{
    uint32_t pos =0;
    uint32_t tick = HAL_GetTick();
    
    if( (pos =list_find_index(cardNumber , list)) != FIND_NULL_ID)
    {        
        log(DEBUG,"[PERMI]�� %d λ�ò�ѯ����ƬID:%llx , ��ѯʱ��:%d\n" , pos, list->ID ,HAL_GetTick()-tick);
        return pos;
    }
    
    log(DEBUG,"[PERMI]�ÿ��Ų��ںڰ������У�ID=%llx , ��ѯʱ��:%d\n",cardNumber,HAL_GetTick()-tick);
    
    return FIND_NULL_ID;
}

/*���� ��д �������� ������ ��Ҫд��*/
int32_t periList_add(permiListType *list)
{
    uint32_t index =0;
    permiListType   listTemp;
    
    if( (index = permiList_find(list->ID ,&listTemp )) == FIND_NULL_ID)
    {
        log(DEBUG,"[PERMI]��ǰ�ڰ������б�û���ҵ���Ӧ���ţ���������\n");
        
        if( (index = permiList_find_index(EMPTY_ID)) == FIND_NULL_ID)
        {
            log(WARN,"[PERMI]�ڰ����������Ѿ����ˡ�\n");
            return LIST_FULL;
        }
        log(DEBUG,"[PERMI]�ҵ�һ������λ�ã�λ��:%d\n" , index);
        permiList_add_index(index , list->ID);
    }
    else
    {
        log(DEBUG,"[PERMI]��ǰ�ڰ������б�����ҵ���Ӧ���ţ�λ�ã�%d \n" , index);
    }
    
    memset( list->temp , 0x00 , 3);
    memset( listTemp.temp , 0x00 , 3);
    if( aiot_strcmp((uint8_t *)list , (uint8_t *)&listTemp , PERMI_LIST_SIZE) == TRUE )
    {
        log(DEBUG,"[PERMI]д������ݺʹ洢������һ������������д��\n");
    }
    else
    {
        log(DEBUG , "[PERMI]���ݽ�д�뵽 %d λ����\n" , index);
        permiList_add_data(index , list);
    }
    return LIST_SUCCESS;
}

/*������ɾ ֻ�ǲ���������ɾ��*/
int32_t permiList_del( uint64_t cardNumber)
{
    uint32_t index =0;
    permiListType   listTemp;
    
    if( (index = permiList_find(cardNumber,&listTemp )) == FIND_NULL_ID)
    {
        log(DEBUG,"[PERMI]�б���û�и�����,����ɾ�� ID = %llx \n" ,cardNumber);
    }
    else
    {
        log(DEBUG," [PERMI]��%d λ��ɾ�� ID = %llx \n" ,index ,cardNumber);
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
            log(DEBUG,"[PERMI]��ǰ�豸�ڰ������� %d ����0X%08X\n" , cnt,tempID);
            cnt++;
        }
    }
    flash.release_lock();
    log(DEBUG,"[PERMI]��ǰ�豸�ڰ�����������:%d\n" , cnt);
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
    //char* cardNo = "40D1BB0133CE6498";//�������֤
    char* cardNo = "723F170A8F2102E0";//��˹��NFC
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
        
        log(DEBUG,"[PERMI]��ǰ�豸ʱ���:%d\n" , deviceTime);
        flash.get_lock();
        for(uint32_t i = 0 ; i < PERMI_LIST_MAX ; i++)
        {
            if(  permiList_read_id(i) != EMPTY_ID)
            {
                permiList_read_data(i, &list);
                if(list.time <  deviceTime)
                {
                    log(DEBUG,"[PERMI]ɾ�����ںڰ�������POS=%d , ID=%llx , TIME:%u\n" , i , list.ID , list.time);
                    permiList_del_index_no_lock(i);
                }                

                if((list.status != LIST_BLACK)&&(list.status != LIST_WRITE))
                {
                    log(INFO,"[PERMI]�ڰ�����״̬����,status=%d , ɾ��POS=%d\n" , list.status ,i);
                    permiList_del_index_no_lock(i);
                }
            }
            HAL_IWDG_Refresh(&hiwdg);
        }
        flash.release_lock();
    }
    else
    {
        log(INFO,"[PERMI]�豸��û��ͬ��ʱ��\n");
    }
}
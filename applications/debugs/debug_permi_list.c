#include <stdlib.h>
#include "components_ins.h"
#include "unit.h"
#include "permi_list.h"
#include "bsp_rtc.h"

         
void debug_show_list( uint8_t type )
{
    uint32_t tempID =0;
    permiListType list;
    rtcTimeType time;
    
    flash.get_lock();
    
    for(uint32_t i = 0 ; i < 8000 ; i++)
    {
        if( ( tempID = permiList_read_id(i)) != EMPTY_ID)
        {
            permiList_read_data(i, &list);
            if(list.status & type )
            {
                rtc.stamp_to_time(&time , list.time);
                log(DEBUG,"[%d] ID = %x, number = %llx ,time =20%02d-%02d-%02d %02d:%02d:%02d , status = %s.\n",
                    i,
                    tempID,
                    list.ID,
                    time.year,time.mon,time.day,time.hour,time.min,time.sec,
                    (list.status ==LIST_BLACK)?"black":"write");
            }
        }
        HAL_IWDG_Refresh(&hiwdg);
    }
    flash.release_lock();
}


void debug_permi_find_number( uint64_t number)
{
    permiListType list;
    rtcTimeType time;

    if( permi.find(number , &list) != FIND_NULL_ID)
    {
        rtc.stamp_to_time(&time , list.time);
            
        log(DEBUG,"number = %llx ,time =20%02d-%02d-%02d %02d:%02d:%02d , status = %s.\n",
                    list.ID,
                    time.year,time.mon,time.day,time.hour,time.min,time.sec,
                    (list.status ==LIST_BLACK)?"black":"write");
    }
    else
    {
        log(DEBUG,"没有找到相应卡号\n");
    }
}

int debug_permi_list (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{

    if( strcmp("-s" , argv[1]) == 0)
    {
        if( strcmp("-a" , argv[2]) == 0)
        {
            debug_show_list( LIST_BLACK|LIST_WRITE);
        }
        else if( strcmp("-b" , argv[2]) == 0)
        {
            debug_show_list(LIST_BLACK );
        }
        else  if( strcmp("-w" , argv[2]) == 0)
        {
            debug_show_list(LIST_WRITE);
        }
    }
    else if( strcmp("-f" , argv[1]) == 0)
    {
        uint64_t number = 0;

        number = atol64(argv[2]);
        log(DEBUG,"查找卡号:%llx ,是否为黑白名单\n" ,number);
        
        debug_permi_find_number(number);
        
    }
    else if( strcmp("-a" , argv[1]) == 0)
    {
        permiListType list;
        rtcTimeType time;
        
        if( (list.ID = atol64(argv[2])) == 0 )
        {
            log(WARN,"输入的参数错误：%s , 只能是0-F的字符串\n" ,argv[2]);
            return -1;
        }
        if( strcmp("-b" , argv[3]) == 0)
        {
            list.status = LIST_BLACK;
        }
        else if( strcmp("-w" , argv[3]) == 0)
        {
            list.status = LIST_WRITE;
        }
        else
        {
            log(WARN,"输入的参数错误：%s , 正确参数 \"-b:黑名单 -w:白名单\" \n" ,argv[3]);
            return -1;
        }
        
        
        list.time =  atoll(argv[4]);
        
        rtc.stamp_to_time(&time , list.time);
            
        log(DEBUG,"新增名单 number = %llx ,time =20%02d-%02d-%02d %02d:%02d:%02d , status = %s.\n",
                    list.ID,
                    time.year,time.mon,time.day,time.hour,time.min,time.sec,
                    (list.status ==LIST_BLACK)?"black":"write");
        
        //for(uint16_t i = 0 ; i < 100 ; i++)
        {
            //list.ID++;
            permi.add(&list);
            //HAL_IWDG_Refresh(&hiwdg);
        }
    }
    else if( strcmp("-d" , argv[1]) == 0)
    {
        if( strcmp("-n" , argv[2]) == 0)
        {
            uint64_t number = 0;

            number = atol64(argv[3]);
            
            permi.del(number);
        }
        else if( strcmp("-s" , argv[2]) == 0)
        {
            uint32_t index =0;
            
            index = atoll(argv[3]);
            
            log(DEBUG,"删除 %d 位置的黑白名单\n" ,index);
            
            permiList_del_index(index);
            
        }
        else
        {
          log(WARN,"输入的参数错误：%s ,帮助请输入: help list \n" ,argv[2]);
        }
    }
    else if( strcmp("-clear all" , argv[1]) == 0)
    {
        log(WARN,"删除全部黑白名单数据\n删除全部黑白名单数据\n删除全部黑白名单数据\n");
        permi.clear();
    }
    
    
    return 0;
}


U_BOOT_CMD(
    list,     6,     1,      debug_permi_list,
	"list\t-黑白名单调试\n",
	"\t-NFC黑白名单调试\n\n\
list -s\t显示黑白名单信息\n\t\
     -a:显示所有名单信息 , 事例：list -s -a \n\t\
     -b:显示所有黑名单 , 事例：list -s -b \n\t\
     -w:显示所有白名单 , 事例：list -s -w \n\n\
list -f\t查询当前卡号是否为黑白名单，卡号不足8字节后补0,例如:list -f 1122334400000000\n\n\
list -a\t增加黑白名单,用法：list -a [number] [-b/-w] [unix stamp]\n\
         事例:list -a 1122334400000000 -b 1508234407\n\n\
list -d\t删除黑白名单\n\t\
     -n:使用卡号删除 ,用法： list -d -n [number]\n\t\
     -s:使用序号删除 ,用法： list -d -s [sn]\n\t\
        可以先使用命令 \"list -s -a\" 查看想要删除名单的序号\n\
list -clear all: 清空所有黑白名单\n"
);

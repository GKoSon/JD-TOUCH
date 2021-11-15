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
        log(DEBUG,"û���ҵ���Ӧ����\n");
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
        log(DEBUG,"���ҿ���:%llx ,�Ƿ�Ϊ�ڰ�����\n" ,number);
        
        debug_permi_find_number(number);
        
    }
    else if( strcmp("-a" , argv[1]) == 0)
    {
        permiListType list;
        rtcTimeType time;
        
        if( (list.ID = atol64(argv[2])) == 0 )
        {
            log(WARN,"����Ĳ�������%s , ֻ����0-F���ַ���\n" ,argv[2]);
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
            log(WARN,"����Ĳ�������%s , ��ȷ���� \"-b:������ -w:������\" \n" ,argv[3]);
            return -1;
        }
        
        
        list.time =  atoll(argv[4]);
        
        rtc.stamp_to_time(&time , list.time);
            
        log(DEBUG,"�������� number = %llx ,time =20%02d-%02d-%02d %02d:%02d:%02d , status = %s.\n",
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
            
            log(DEBUG,"ɾ�� %d λ�õĺڰ�����\n" ,index);
            
            permiList_del_index(index);
            
        }
        else
        {
          log(WARN,"����Ĳ�������%s ,����������: help list \n" ,argv[2]);
        }
    }
    else if( strcmp("-clear all" , argv[1]) == 0)
    {
        log(WARN,"ɾ��ȫ���ڰ���������\nɾ��ȫ���ڰ���������\nɾ��ȫ���ڰ���������\n");
        permi.clear();
    }
    
    
    return 0;
}


U_BOOT_CMD(
    list,     6,     1,      debug_permi_list,
	"list\t-�ڰ���������\n",
	"\t-NFC�ڰ���������\n\n\
list -s\t��ʾ�ڰ�������Ϣ\n\t\
     -a:��ʾ����������Ϣ , ������list -s -a \n\t\
     -b:��ʾ���к����� , ������list -s -b \n\t\
     -w:��ʾ���а����� , ������list -s -w \n\n\
list -f\t��ѯ��ǰ�����Ƿ�Ϊ�ڰ����������Ų���8�ֽں�0,����:list -f 1122334400000000\n\n\
list -a\t���Ӻڰ�����,�÷���list -a [number] [-b/-w] [unix stamp]\n\
         ����:list -a 1122334400000000 -b 1508234407\n\n\
list -d\tɾ���ڰ�����\n\t\
     -n:ʹ�ÿ���ɾ�� ,�÷��� list -d -n [number]\n\t\
     -s:ʹ�����ɾ�� ,�÷��� list -d -s [sn]\n\t\
        ������ʹ������ \"list -s -a\" �鿴��Ҫɾ�����������\n\
list -clear all: ������кڰ�����\n"
);

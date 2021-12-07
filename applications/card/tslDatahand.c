#include "tslDatahand.h"
#include "apdu_phone.h"
#include "open_log.h"
#include "open_door.h"
#include "magnet.h"
#include "bsp_rtc.h"
#include "unit.h"
#include "beep.h"

extern _SHType SHType; 

uint8_t tag_verify_group1(shanghaicardtype *p)
{
    uint8_t i,j,level=0;

    for(i=0;i<5;i++)
	{
		if( p->body[i].crc)
		{
			//level = p->body[i].power;
			//printf("level = p->p->body[%d].power=%d\r\n",i,level);		
			
			for(j=0;j<SHType.gup.cnt;j++)
			{
				if(aiot_strcmp(SHType.gup.code[j],p->body[i].group,11))
                                {
                                   log_err("【CARD】YES!\r\n");
                                   return TAG_SUCESS;  
                                }
				log_err("【CARD】(card %d vs device %d)",i,j);
                                
                                log_arry(DEBUG,"设备同行组"  ,SHType.gup.code[j],11);
			} 
                        printf("\r\n");
			
		}
	}
  return  TAG_COMM_ERR;

}

uint8_t tag_verify_group2(shanghaicardtype *p)
{
    uint8_t i,j,devicegupcode[22],cardgupcode[22];

    for(i=0;i<5;i++)
	{
		if( p->body[i].crc)
		{
			
			for(j=0;j<SHType.gup.cnt;j++)
			{
                                memcpy_up(devicegupcode,SHType.gup.code[j],11);  
                                memcpy_up(cardgupcode,p->body[i].group,11);
				if(aiot_strcmp(devicegupcode,cardgupcode,15))
                                {
                                   log_err("【CARD】YES!\r\n");
                                   return TAG_SUCESS;  
                                }
				log_err("【CARD】(card %d vs device %d)",i,j);
                                
                                log_arry(DEBUG,"设备同行组"  ,SHType.gup.code[j],11);
			} 
			printf("\r\n");
		}
	}
  return  TAG_COMM_ERR;

}


uint8_t tag_verify_time(shanghaicardtype *p)
{
    uint8_t t[3]={0},i;
    uint32_t DateInCard,DateInSys;
    rtcTimeType time;  
    rtc.read_time(&time);
    
    DateInSys=(time.year<<16)|(time.mon<<8)|time.day; 
   
    for(i=0;i<5;i++)
    {   
      if(p->body[i].endtime[0]!=0)
      {
	  memcpy(t,p->body[i].endtime,3);

	  DateInCard=( bcd_to_bin(t[0])<<16 | bcd_to_bin(t[1])<<8 | bcd_to_bin(t[2]));
          /*make a flag*/
	  p->body[i].crc = (DateInCard >= DateInSys )?1:0;
	  
         printf("sys :0X%X:%X:%X        card:0x%X:%X:%X\n",time.year,time.mon,time.day,p->body[i].endtime[0],p->body[i].endtime[1],p->body[i].endtime[2]);
         printf("DateInSys:%X            DateInCard:%X\n",DateInSys,DateInCard);
      }
    }
    
    p->head.crc = 0;
    for(i=0;i<5;i++)
	if( p->body[i].crc)
	   p->head.crc++;
    
    if(p->head.crc ==0)
    {
           log_err("【CARD】All time FAILED\r\n");
	   return 1;
    }
    log_err("【CARD】time ok ! card  have %d gup\r\n", p->head.crc);
    return 0;
}

uint8_t tag_shanghai_user_process( tagBufferType *tag)
{
  
    SHOWME
    shanghaicardtype *p = (shanghaicardtype *)tag->buffer;    
    if(tag_verify_time(p))
        return TAG_TIME_ERR;
    if(tag->tagPower==MANAGENT_TAG)
    {
        log_err("【CARD】This is man card [15same]!\r\n");
        return tag_verify_group2(p);
    }
    uint8_t deviceLockMode = config.read(CFG_SYS_LOCK_MODE , NULL);
    if(deviceLockMode==0)//0:单元机  1：围墙机 2：多围墙 
    {
        log_err("【CARD】This is menkouji [22same]!\r\n");
        return tag_verify_group1(p);
    } 
    else if(deviceLockMode==1)
    {

        log_err("【CARD】This is weiqiangji [15same]!\r\n");
        return tag_verify_group2(p);
    } 
  return  TAG_COMM_ERR;
}

uint8_t tag_shanghai_card_process( tagBufferType *tag)
{
    log_err("tag_shanghai_card_process, POWER = %d(临时-0x00 居民-0x02 管理员-0x03)\r\n" , tag->tagPower);
    
    
    switch(tag->tagPower)
    {
        case TEMP_TAG:      return (tag_shanghai_user_process(tag));
        case USER_TAG:      return (tag_shanghai_user_process(tag));
        case MANAGENT_TAG:  return (tag_shanghai_user_process(tag));

        default:           return TAG_NO_SUPPORT;
    }
}


#include "tslDatahand.h"
#include "apdu_phone.h"
#include "open_log.h"
#include "open_door.h"
#include "magnet.h"
#include "bsp_rtc.h"
#include "unit.h"

#include "beep.h"



#include "BleDataHandle.h"

//extern _SHType SHType; 

uint8_t tag_verify_group(shanghaicardtype *p)
{
#if 0
  
  
  
  
  uint16_t cardshort[5];
  memset(cardshort,0,5);
  for(i=0;i<5;i++)
    cardshort = CRC16_CCITT(p->body[i].group,11);
  
  
  
    uint8_t i,j,level=0;
    uint8_t *defgup=NULL;


   // config.read(CFG_SYS_DEFGUP_CODE , (void **)&defgup);

    for(i=0;i<5;i++)
	{
		if( p->body[i].crc)
		{
			level = p->body[i].power;
			printf("level = p->p->body[i].power=%d\r\n",level);		

			if(is_arr_same(defgup,p->body[i].group,11))
                        {printf("??¨¨?¡Á¨¦¡À¨¨??3¨¦1|\n"); return TAG_SUCESS;}  

			printf("??¨¨?¡Á¨¦¡À¨¨??¨º¡ì¡ã¨¹\n");
			
			
		}
	}  

    //printf("%d\r\n",SHType.gup.cnt);
    for(i=0;i<5;i++)
	{
		if( p->body[i].crc)
		{
			level = p->body[i].power;
			printf("level = p->p->body[i].power=%d\r\n",level);		
			
			for(j=0;j<SHType.gup.cnt;j++)/*????¦Ì?gid*/ 
			{
				if(is_arr_same(SHType.gup.code[j],p->body[i].group,11))
                                {
                                   log_err("¨ª?DD¡Á¨¦¡À¨¨??OK ?a??\r\n");
                                   return TAG_SUCESS;  
                                }
				printf("¡À?¡ä?¡À¨¨??[?¡§¡ä?%d]#####[¨ª¡§DD¡Á¨¦¡ä?%d]",i,j);
			} 
			
		}
	}
    printf("¨ª?DD¡Á¨¦¡À¨¨??¨º¡ì¡ã¨¹\n");
  return  TAG_COMM_ERR;
#endif
}
uint8_t Gtag_verify_time(shanghaicardtype *p)
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

	  p->body[i].crc = (DateInCard >= DateInSys )?1:0;
	  
        printf("sys :%X--%X--%X  <-pk->  card:%X--%X--%X\n",time.year,time.mon,time.day,p->body[i].endtime[0],p->body[i].endtime[1],p->body[i].endtime[2]);
        printf("DateInCard::%X\tDateInSys::%X\n",DateInCard,DateInSys);
      }
    }
    
    p->head.crc = 0;
    for(i=0;i<5;i++)
	if( p->body[i].crc)
	   p->head.crc++;
    
    if(p->head.crc ==0)
    {
           printf("?¨´¡À???¨®D¨º¡À??OK¦Ì?¡Á¨¦\r\n");
	   return 1;
    }
    printf("¨®D¨º¡À??OK¦Ì?¡Á¨¦ ¨°??e¨®D%d??\r\n", p->head.crc);
    return 0;
}


uint8_t tag_shanghai_user_process( tagBufferType *tag)
{
  
    SHOWME
    shanghaicardtype *p = (shanghaicardtype *)tag->buffer;
    
    if(Gtag_verify_time(p ))
    {
        return TAG_TIME_ERR;
    }
    
    return tag_verify_group(p);

}



uint8_t tag_shanghai_card_process( tagBufferType *tag)
{
    log_err("tag_shanghai_card_process, POWER = %02X\n ??2-USER_TAG 3-MANAGENT_TAG 4-TEMP_TAG 8-CONFIG_TAG??" , tag->tagPower);
    
    
    switch(tag->tagPower)
    {
        case TEMP_TAG:      return (tag_shanghai_user_process(tag));//2?¨°a return 0;?¨¹?¡À?¨®?a??¨¤2¡ê?
        case USER_TAG:      return (tag_shanghai_user_process(tag));
        case MANAGENT_TAG:  return (tag_shanghai_user_process(tag));//2?¨°a return 0;?¨¹?¡À?¨®?a??¨¤2¡ê?

        default:            printf("tag->tagPower ERR\r\n");return TAG_NO_SUPPORT;
    }
}


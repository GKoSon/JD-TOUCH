#include "swipeTag.h"
#include "unit.h"
#include "string.h"
#include "beep.h"
#include "tagComponent.h"
#include "EncryDecry.h"
#include "stdbool.h"
#include "permi_list.h"
#include "bsp_rtc.h"
#include "open_door.h"
#include "open_log.h"
#include "tslDataHand.h"

#define    UID_SIZE 16    
uint8_t beforeTagUID[UID_SIZE];
uint8_t readFishFlag = FALSE;
uint8_t readType = TRACK_NONE;
uint8_t detectTagCount = 0;
uint8_t detectTagFlag = 0;

blinkParmType beepTagNullErr=
{
    .mode = BLINK_OPEN_NOW,
    .openCnt = 4,
    .openTime = 40,
    .closeTime = 40,
    .delayTime = 0,
};

blinkParmType beepTagTimeErr=
{
    .mode = BLINK_OPEN_NOW,
    .openCnt = 2,
    .openTime = 40,
    .closeTime = 40,
    .delayTime = 0,
};

blinkParmType beepBlackListErr=
{
    .mode = BLINK_OPEN_NOW,
    .openCnt = 5,
    .openTime = 40,
    .closeTime = 40,
    .delayTime = 0,
};

uint8_t ReadCardTypeArry[]=
{
    TRACK_NFCTYPE5,
    TRACK_NFCTYPE4A,
    TRACK_NFCTYPE4B,
};

uint8_t ReadCardTypeCnt;

const uint8_t ReadCardTypeMax = sizeof(ReadCardTypeArry);

void tag_updata_beforeUid(  tagBufferType *tag )
{
    memset(beforeTagUID , 0x00 , UID_SIZE);
    
    memcpy(beforeTagUID , tag->UID , tag->UIDLength);
    
    readFishFlag = TRUE;
}

void tag_clear_beforeUid( void )
{
    memset(beforeTagUID , 0x00 , UID_SIZE);
    
    readFishFlag = FALSE;
}

uint8_t tag_compare_uid(uint8_t *old_uid , tagBufferType *tag)
{
  
    for(uint8_t i = 0 ; i < tag->UIDLength ; i++)
    {
        if( old_uid[i] != tag->UID[i])
        {
            return FALSE;
        }
    }
    
    return TRUE;
}

uint64_t assemble_id( tagBufferType *tag)
{
    uint64_t idTemp[16]={0};
    uint64_t id;
    for(uint8_t i = 0 ; i < tag->UIDLength ; i++)
    {
        idTemp[i] = tag->UID[i];
    }
    
    id=idTemp[0]<<56|idTemp[1]<<48|idTemp[2]<<40|idTemp[3]<<32|
             idTemp[4]<<24|idTemp[5]<<16|idTemp[6]<<8|idTemp[7];
    log_arry(DEBUG,"[CARD]UID" ,tag->UID , 8);
    printf("[CARD]assemble_id 0x%llx \r\n",id);
    return id;
    
}

void tag_calc_crc16(uint8_t ucdata,uint16_t *pwcrc)
{
      uint16_t x=*pwcrc;
      x=(uint16_t)((uint8_t)(x>>8)|(x<<8));
      x^=(uint8_t)ucdata;
      x^=(uint8_t)(x&0xff)>>4;
      x^=(x<<8)<<4;
      x^=((x&0xff)<<4)<<1;
      *pwcrc=x;
}

uint8_t GetReadCardType( void )
{
    uint8_t rt = TRACK_NOTHING;
    
    rt = ReadCardTypeArry[ReadCardTypeCnt++];
    if( ReadCardTypeCnt >= ReadCardTypeMax )
    {
        ReadCardTypeCnt = 0;
    }
    
    return rt;
}



/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/


uint8_t tag_hunting ( uint8_t tagsToFind ,tagBufferType *card)
{    
    uint8_t ret = TRACK_NONE;
    
    if (tagsToFind&TRACK_NFCTYPE5)
    {
        if( (ret = tagComp->iso15693_get_uid( card )) != TRACK_NONE)
        {
            return ret;
        }
    }
    
    if (tagsToFind&TRACK_NFCTYPE4A)
    {
        if( (ret = tagComp->iso14443a_get_uid( card )) != TRACK_NONE)
        {
            return ret;
        }
    }
    
    if (tagsToFind&TRACK_NFCTYPE4B)
    {
        if( (ret = tagComp->iso14443b_get_uid( card )) != TRACK_NONE)
        {
            return ret;
        }
    }
    
    return TRACK_NONE;
}

uint8_t tag_find_class( tagBufferType *tag)
{

    if(readType == TRACK_NONE)
    {
        readType = GetReadCardType();
    }
    
    tag->cardType = tag_hunting(readType , tag);
    
    if( tag->cardType == TRACK_NOTHING)
    {   
        if( detectTagFlag)
        {
            if(++detectTagCount >= 3)
            {
                detectTagFlag = 0;
                detectTagCount =0;
                tag_clear_beforeUid();
                beep.write(BEEP_CLEAR);
                readType = TRACK_NONE;
                log(DEBUG, "[CARD]card leaved away\n");
            }
        }
        else
        {
            readType = TRACK_NONE;
        }
        return  TAG_NONE;
    }    
    
    log(DEBUG,"[CARD]tag->cardType=0X%02X[ALL 3(0X20-15693)(0X10-1443B)(0X20-1443A)]\n" , tag->cardType );
    log_arry(DEBUG,"[CARD]tag->UID" , tag->UID , tag->UIDLength   );
    detectTagFlag =1;
    return TAG_SUCESS;
}

static void SHOW_card(shanghaicardtype *p)
{
    char i,j;
    printf("p->head.class = %02X[00--A901 02--A902  03--rainbow]\r\n",p->head.class);
    printf("p->head.type = %02X[临时-0x00 居民-0x02 管理员-0x03]\r\n",p->head.type);
    printf("p->uid:");
    for(i=0;i<4;i++) printf("%02X ",p->head.uid[i]);
    printf("\r\n");
    printf("p->head.ID:");
    for(i=0;i<9;i++) printf("%02X ",p->head.ID[i]);
    printf("\r\n");
    printf("p->head.crc = %02X\r\n",p->head.crc);
    
    for(i=0;i<5;i++)
    {
      printf("p->body[i].group:");
      for(j=0;j<11;j++) printf("%02X ",p->body[i].group[j]);
      printf("\r\n");

      printf("p->body[i].endtime:");
      for(j=0;j<3;j++) printf("%02X ",p->body[i].endtime[j]);
      printf("\r\n");

      printf("p->body[i].crc = %02X\r\n",p->body[i].crc);
      printf("p->body[i].power = %02X\r\n",p->body[i].power);
    }


     printf("p->crc.crc16 = %02X\r\n",p->crc.crc16);
     printf("p->crc.crc = %02X--%02X\r\n",p->crc.crc[0],p->crc.crc[1]);
}

uint8_t checkpowerinfo(shanghaicardtype *p)
{
    switch(p->head.type)
    {
        case 0X00:return TEMP_TAG;
        case 0X02:return USER_TAG;
        case 0X03:return MANAGENT_TAG;       
    }
    return INIT_TAG;
}

uint8_t checkcrcerr(shanghaicardtype *p)
{
    uint8_t *data = (uint8_t *)p;
    uint16_t crc16=0;
    if(p->head.crc       != mycrc8(&data[0] ,15))
        return 1;
    
    if(p->body[0].endtime[0]!=0  && p->body[0].crc    != mycrc8(&data[16],14))
        return 1;
    if(p->body[1].endtime[0]!=0  && p->body[1].crc    != mycrc8(&data[31],15))
        return 1;
    if(p->body[2].endtime[0]!=0  && p->body[2].crc    != mycrc8(&data[47],15))
        return 1;
    if(p->body[3].endtime[0]!=0  && p->body[3].crc    != mycrc8(&data[63],15))
        return 1;
    if(p->body[4].endtime[0]!=0  && p->body[4].crc    != mycrc8(&data[79],15))
        return 1;
    printf("[CARD]CRC8 OK\r\n");
    for(char i=0; i<96; i++)
          tag_calc_crc16(data[i],&crc16);
        printf("[CARD]CARDCRC16=%02X  MYCRC16=%02X\r\n",p->crc.crc16,crc16);     
        if(crc16 != p->crc.crc16)
        return 1;
    printf("[CARD]CRC16 OK\r\n");
    return 0;    
}

uint8_t checkuiderr(shanghaicardtype *p,tagBufferType *tag )
{
    log_arry(DEBUG,"[CARD]p->head.uid" ,p->head.uid , 4 );    
    uint8_t tem[4];
    tem[0]=tag->UID[0] ^tag->UID[4];
    tem[1]=tag->UID[1] ^tag->UID[5];
    tem[2]=tag->UID[2] ^tag->UID[6];
    tem[3]=tag->UID[3] ^tag->UID[7];
    log_arry(DEBUG,"[CARD]uid" ,tem , 4 );    
    for(char i=0;i<4;i++)
          if(tem[i] != p->head.uid[i])
            return 1;
    return 0;        
}

uint8_t checkclasserr(shanghaicardtype *p)
{

    if(p->head.class == 3 ||p->head.class == 2 ||p->head.class == 1)
        return 0;
    else
        return 1;
}

uint8_t tag_read_15693_data( tagBufferType *tag ) 
{
    uint8_t tag_buffer[128];
    shanghaicardtype *p=NULL;
    memset(tag_buffer , 0x00 , 128);

    if( tagComp->iso15693_read_data(0 , (98+13),tag_buffer) == FALSE)
    {   

      memcpy(tag->buffer , tag_buffer , 98);
      
      log_arry(1,"tag->buffer" ,tag->buffer , 98 );

      Decryptionr(tag->buffer,tag->UID,tag->buffer);

      log_arry(0,"tag->buffer" ,tag->buffer , 98 ); 

      p = (shanghaicardtype *)tag->buffer;

      SHOW_card(p); 

      if(checkclasserr(p))
      {
        log(ERR,"[CARD]CLASS RETURN\n");
        return TAG_NULL;
      }

      if(checkuiderr(p,tag))
      {
        log(ERR,"[CARD]UID RETURN\n");
        return TAG_NULL;
      }

      if(checkcrcerr(p))
      {
        log(ERR,"[CARD]CRC RETURN\n");
        return TAG_CRC_ERR;
      }    


      tag->tagPower = (tagPowerEnum)checkpowerinfo(p);
      tag->type = TAG_SHANGHAI_CARD;

      log(ERR,"TAG_SHANGHAI_CARD TAG_SUCESS\n");
      return TAG_SUCESS;
 
    }
    
    log_err("[CARD]tag_read_15693_data FAIL\n");
    return TAG_NONE;
}


uint8_t tag_read_fk_card( tagBufferType *tag ) 
{
    uint8_t tag_buffer[98];//16*6+2=98 ALL DATA
    shanghaicardtype *p=NULL;
    memset(tag_buffer , 0x00 , 98);

    if(tagComp->read_m1_data(tag))
    {   

      log_arry(DEBUG,"tag_buffer" ,tag_buffer , 98 );

      Decryptionr(tag_buffer,tag->UID,tag->buffer);

      memcpy(&tag->buffer[64],&tag_buffer[64],98-64);

      log_arry(DEBUG,"tag->buffer" ,tag->buffer , 98 ); 

      p = (shanghaicardtype *)tag->buffer;

      SHOW_card(p);        


      if(checkclasserr(p))
      {
        log(ERR,"[CARD]CLASS\n");
        return TAG_NULL;
      }

      if(checkuiderr(p,tag))
      {
        log(ERR,"[CARD]UID\n");
        return TAG_NULL;
      }

      if(checkcrcerr(p))
      {
        log(ERR,"[CARD]CRC\n");
        return TAG_CRC_ERR;
      }    


      tag->tagPower = (tagPowerEnum)checkpowerinfo(p);
      tag->type = TAG_SHANGHAI_CARD;

      log(ERR,"TAG_SHANGHAI_CARD TAG_SUCESS\n");
      return TAG_SUCESS;
 
    }
    
    log_err("[CARD]read_m1_data FAIL\n");
    return TAG_NONE;
}


uint8_t tag_read_data( tagBufferType *tag)
{
    switch(tag->cardType)
    {
/*每个case 都是一次性 设置2个flag tag->type  tag->tagPower*/
        case TRACK_NFCTYPE4A: 
        {
            log(DEBUG,"[CARD]TRACK_NFCTYPE4A:tag->sak:%02X\n",tag->sak);        
                                
            if(tag->sak == 0x28)
            {
              log(DEBUG,"[CARD]FM1208 CUP card");
            }

            return tag_read_fk_card(tag);            
        }break;
        
        case TRACK_NFCTYPE4B: 
        {
            log_arry(DEBUG,"[CARD]TRACK_NFCTYPE4B,UID" , tag->UID , tag->UIDLength);    
            tag->type = TAG_ID_CARD;
            tag->tagPower = ID_TAG;
            return TAG_TYPE4B_ERR;
        }break;    
        
        case TRACK_NFCTYPE5: 
        {
              return (tag_read_15693_data(tag));
        }break;
        
        default: 
        {
            NEVERSHOW
            return TAG_NONE;
        }break;
    }

}

uint8_t read_tag( tagBufferType *tag)
{
    uint8_t rt  = TAG_SUCESS;
    
    if( ( rt = tag_find_class(tag)) != TAG_SUCESS)  
    { 
        return rt;
    }

    if(tag_compare_uid(beforeTagUID , tag))
    {
        return TAG_SAME_ID_ERR;
    }
    
    if( ( rt = tag_read_data(tag)) != TAG_SUCESS)  
    {   
        return rt;    
        
    }
 
    return TAG_SUCESS;
}

uint8_t read_list_name(tagBufferType *tag)
{
    uint64_t ID;
    uint32_t stamp;
    permiListType list;

    ID = assemble_id(tag);
    
    log_err("[CARD]read_list_name ID:%llx\n",ID);
    
    if(permi.find(ID , &list) != FIND_NULL_ID)
    {
        stamp = rtc.read_stamp();
        if(( list.status == LIST_WRITE) )
        {
            log(ERR,"[CARD]list.status == LIST_WRITE\n");
            if(  stamp < list.time)
            {
                return TAG_WRITE_LIST;
            }
            else
            {
                log(ERR,"[CARD]list.status == LIST_WRITE but timeout %d :%d\n" ,list.time,stamp);
                return TAG_TIME_ERR;
            }
        }
        else if( list.status == LIST_BLACK)
        {
            log(ERR,"[CARD]list.status == LIST_BLACK\n");
            if( stamp < list.time )
            {
                return  TAG_BALCK_LIST_ERR;
            }
            else
            {
                log(ERR,"[CARD]list.status == LIST_BLACK but timeout %d :%d\n" ,list.time,stamp);
            }
        }
    }
    else
      log(ERR,"[CARD]NEW CARD WITH NO LIST IN TOUCH [TAG_LIST_NULL]\n");
    
    return TAG_LIST_NULL;
}

uint8_t tag_data_process( tagBufferType *tag)
{
 
    log(DEBUG,"[CARD]tag->type=%02X\n" ,  tag->type );
    log_arry(DEBUG,"[CARD]tag->UID:" , tag->UID , tag->UIDLength   );
    
    switch(tag->type)
    {

        case TAG_SHANGHAI_CARD:
        {
            log(ERR,"[CARD]TAG_SHANGHAI_CARD\n");
            return (tag_shanghai_card_process(tag) );
        }break;
        
        
        default:
        {
            log_err("[CARD]No this tag type = %d.\r\n", tag->type);
        }break;
    }

    return TAG_ERR;
}


void tag_interaction_buzzer( tagBufferType *tag , uint8_t *result)
{
   openlogDataType    logData;
   uint8_t sendLogFlag = FALSE , openResult = FALSE;
    
    if(*result== TAG_NONE)
      return;
    
    log(WARN,"[CARD-RET]result=%d\n" ,*result);
    
    switch(*result)
    {
        case TAG_NULL:
        {
            beep.write_base(&beepTagNullErr);
            tag_updata_beforeUid(tag);
        }break;
        
        case TAG_WRITE_LIST:
        case TAG_SUCESS:
        {
            beep.write(BEEP_NORMAL);
            open_door();
            tag_updata_beforeUid(tag);
            sendLogFlag = TRUE;
            openResult = TRUE;
        }break;
       
        case TAG_CRC_ERR:
        {
            beep.write(BEEP_DEALY);
        }break;
        
        case TAG_SAME_ID_ERR:
        {
            tag_updata_beforeUid(tag);
        }break;
        
       
        case TAG_BALCK_LIST_ERR:
        {
          beep.write_base(&beepBlackListErr);
          tag_updata_beforeUid(tag);
          sendLogFlag = TRUE;
        }break;

        case TAG_TYPE4B_ERR:log(WARN,"[CARD-RET]TAG_TYPE4B_ERR A STRANGER ID CARD \n");
        case TAG_NO_SUPPORT:    
        case TAG_ERR:           
        case TAG_COMM_ERR:        
        {
          beep.write(BEEP_ALARM);
          tag_updata_beforeUid(tag);
          sendLogFlag = TRUE;
        }break;
        case TAG_TIME_ERR: 
        {
          beep.write_base(&beepTagTimeErr);
          tag_updata_beforeUid(tag);
          sendLogFlag = TRUE;
        }break;
        
        default :
        {
            log(WARN,"[CARD-RET]No this mode = %d\n" , *result);
        }break;
    }

    
    if( sendLogFlag == TRUE )
    {    
            memset(&logData , 0x00 , sizeof(openlogDataType));
            logData.type = OPENLOG_FORM_CARD;
            logData.length = openResult;
            memcpy(logData.data , (uint8_t *)tag , sizeof(tagBufferType));
            journal.save_log(&logData);
    }
}


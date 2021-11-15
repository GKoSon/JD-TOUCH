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
#include "fkDataHand.h"


#define	UID_SIZE 16	
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
    //TRACK_NFCTYPE5,
    //TRACK_NFCTYPE3,
    //TRACK_NFCTYPE4A,
    //TRACK_NFCTYPE5,
    //TRACK_NFCTYPE1,
    //TRACK_NFCTYPE4B,
    //TRACK_NFCTYPE5,
    //TRACK_NFCTYPE2,
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
	//printf("\r\n%s\r\n",__FUNCTION__);

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
uint8_t tag_compare_uid_common(tagBufferType *tag)
{
    if(tag_compare_uid(beforeTagUID , tag)) 
	{
	  printf("雷同卡\n");
	  return 1;
	}
	return 0;
	  
}
uint64_t assemble_id( tagBufferType *tag)
{
    uint64_t idTemp[16]={0};
    
    for(uint8_t i = 0 ; i < tag->UIDLength ; i++)
    {
        idTemp[i] = tag->UID[i];
    }
    
    return( idTemp[0]<<56|idTemp[1]<<48|idTemp[2]<<40|idTemp[3]<<32|
            idTemp[4]<<24|idTemp[5]<<16|idTemp[6]<<8|idTemp[7]);
    
}


uint8_t comm_bcd_to_bin(uint8_t bcd)
{
	return (bcd>>4)*10 + (bcd&0x0F);
}

uint8_t tag_is_null( uint8_t *buffer , uint8_t length)
{
    uint32_t sum = 0 ; 
  
    for(uint8_t i = 0 ; i < length ; i++)
    {
        sum += buffer[i];
    }
    
    if( sum < 0xFF)
    {
        return TRUE;
    }
    
    return FALSE;
}

//CRC8 效验
uint8_t tag_calc_crc8(uint8_t *ps1,uint8_t uLen)
{
  uint8_t i_crc=0,i;
  
  for(i=0;i<uLen;i++)
  {
        i_crc ^=*(ps1++);
  }
  return i_crc;
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

bool tag_check_crc(uint8_t u1,uint8_t *crc2 , uint8_t *uRd)
{
    uint8_t i,uLen=(u1==1)?29:30;
    uint16_t wD=0,wD1=(u1==1)?(((*(uRd+30))<<8)|(*(uRd+29))):(((*(uRd+31))<<8)|(*(uRd+30)));
    uint16_t wD2=0,wD21=((*(uRd+88))<<8)|(*(uRd+87));
  
    for(i=0;i<uLen;i++)
    {
        tag_calc_crc16(*(uRd+i),&wD);
    }
  
    for(i=0;i<87;i++)
    {
        tag_calc_crc16(*(uRd+i),&wD2);
    }
  
    ///log(INFO,"crc leng = %d ,card crc : %x , %x , calc crc : %x %x\n",uLen ,wD1 ,wD21 , wD , wD2);
  
    *crc2 = (wD2==wD21)?TRUE:FALSE;

    if(wD==wD1)
    {
        return FALSE;
    }
  
    return TRUE;

}

uint8_t tag_check_data_crc(tagBufferType *tag)
{
    if((tag->buffer[28]==0x12)||(0x80==tag->buffer[16]))
    {
        if(tag_check_crc((0x80==tag->buffer[16])?1:0, &tag->allDataCrc , tag->buffer))
        {
            log_err("user data crc check is error.\n");                        
            return TAG_CRC_ERR;
        }
        if((0x80==tag->buffer[16])&&(tag->allDataCrc == FALSE))
        {
            log_err("all data crc check is error.\n");                        
            return TAG_CRC_ERR; 
        }
        
        return TAG_SUCESS;
    } 
    
    return TAG_SUCESS;
}


tagPowerEnum tagAnalyticalClass(tagBufferType *tag)
{
    tagPowerEnum tagPwoer = INIT_TAG;
    
    if(0x81==(tag->buffer[16]&0x81))
    {
        tagPwoer = UINT_ADMIN_TAG;
    }
    else if(0x80==tag->buffer[16])
    {
        tagPwoer = CONFIG_TAG;
    }
    else
    {
        uint8_t m_temp = 0;
        m_temp=(tag->buffer[16]>>2)&0x03;
        if(2>m_temp)
        {
            tagPwoer = USER_TAG;
        }
        else if(m_temp==0x02)
        {
            tagPwoer = MANAGENT_TAG;
        }
        else if(m_temp==0x03)
        {
            tagPwoer = UINT_MANAGENT_TAG;
        }
    }

    printf("tagPwoer=%d$$$$$$$$$$$$$【2用户卡】【3管理员卡】[8安装卡清空设备]\r\n",tagPwoer);
    return tagPwoer;
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
                log(DEBUG, "检测到卡片已经离开\n");
            }
        }
        else
        {
            readType = TRACK_NONE;
        }
        return  TAG_NONE;
    }    
    //log(DEBUG,"TYPE=%d\n" ,  tag->cardType );
    //log_arry(DEBUG,"CARD ID:" , tag->UID , tag->UIDLength   );
    detectTagFlag =1;
    return TAG_SUCESS;
}



uint8_t tag_read_15693_data( tagBufferType *tag )
{
	uint8_t tag_buffer[128];
	memset(tag_buffer , 0x00 , 128);
	
	if(  tagComp->iso15693_read_data(0 , 110 ,tag_buffer) == FALSE)
	{
		memcpy(tag->buffer , tag_buffer+13 , 100);
		
		if( tag_is_null(tag_buffer,110))
		{
			log(INFO,"卡片数据为空\n");
			return TAG_NULL;
		}               
		log_arry(DEBUG,"Read data:" ,tag->buffer , 110);
		Decryptionr(tag->buffer,tag->UID,tag->buffer);
		log_arry(DEBUG,"Decryptionr data:" ,tag->buffer , 110 );
		if( tag_calc_crc8(tag->buffer , 24) !=  0)
		{                    
			log_err("24字节的CRC校验错误，卡片数据读取失败.\n");
			return TAG_CRC_ERR;
		}
		
		if( tag_check_data_crc(tag) != TAG_SUCESS)
		{
			log_err("所有字节的CRC校验错误，只能做单小区权限判断.\n");
			return TAG_CRC_ERR;
		}
		tag->tagPower = tagAnalyticalClass(tag);
		tag->type = TAG_TERMINUS_CARD;
		return TAG_SUCESS;
	}
	
	return TAG_NONE;
}


uint8_t tag_read_data( tagBufferType *tag)
{
  
    printf("%d--tag->cardType=%d",__LINE__,tag->cardType);
    
    switch(tag->cardType)
    {

        case TRACK_NFCTYPE4B: 
	{
                  log_arry(DEBUG,"检测到身份证,UID" , tag->UID , tag->UIDLength);    
                  tag->type = TAG_ID_CARD;
                  tag->tagPower = ID_TAG;
                  return TAG_TYPE4B_ERR;
        }break;    
        
        case TRACK_NFCTYPE5: 
	{
          	return (tag_read_15693_data(tag));
        }break;
        
        case TRACK_NFCTYPE1: 
        case TRACK_NFCTYPE2: 
        case TRACK_NFCTYPE3: 
        case TRACK_NFCTYPE4A:
        default: 
            log(ERR ,"不识别的卡片类型 type =%d.\r\n" , tag->cardType);
            return TAG_NO_SUPPORT;

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

uint8_t read_list_name( tagBufferType *tag)
{
    SHOWME
    uint64_t ID;
    uint32_t stamp;
    permiListType list;

    ID = assemble_id(tag);
    
    if( permi.find(ID , &list) != FIND_NULL_ID)
    {
        stamp = rtc.read_stamp();
        if(( list.status == LIST_WRITE) )
        {
            log(DEBUG,"白名单卡\n");
            if(  stamp < list.time)
            {
                return TAG_WRITE_LIST;
            }
            else
            {
                log(DEBUG,"过期卡 , 卡片时间:%d , 系统时间:%d\n" ,list.time,stamp);
                return TAG_TIME_ERR;
            }
        }
        else if( list.status == LIST_BLACK)
        {
            log(DEBUG,"黑名单卡\n");
            if( stamp < list.time )
            {
                return  TAG_BALCK_LIST_ERR;
            }
            else
            {
                log(DEBUG,"过期卡 , 卡片时间:%d , 系统时间:%d\n" ,list.time,stamp);               
            }
        }
    }
    
    return TAG_LIST_NULL;
}



uint8_t tag_data_process( tagBufferType *tag)
{
 
    log(DEBUG,"TYPE=0x%X\n" ,  tag->cardType );
    log_arry(DEBUG,"CARD ID:" , tag->UID , tag->UIDLength   );
    
    switch(tag->type)
    {

        case TAG_TERMINUS_CARD://特斯联卡片ISO15693
        {
            log(DEBUG,"进入特斯联卡片数据解析\n");
            return (tag_terminus_card_process(tag) );
        }break;
       
        default:
        {
            log_err("[%s]No this tag type = %d.\r\n", __func__ , tag->type);
        }break;
    }

    return TAG_ERR;
}


void tag_interaction_buzzer( tagBufferType *tag , uint8_t *result)
{
    openlogDataType	logData;
    uint8_t sendLogFlag = FALSE , openResult = FALSE;
	
    switch(*result)
    {
        case TAG_NONE: break;
	case TAG_WRITE_LIST:
        case TAG_SUCESS://成功
        {
            beep.write(BEEP_NORMAL);
            open_door();
            tag_updata_beforeUid(tag);
	    sendLogFlag = TRUE;
	    openResult  = TRUE;
        }break;
        case TAG_NULL://空卡
        case TAG_CRC_ERR://CRC校验错误，重新读取数据，延迟2秒报警
        case TAG_NO_SUPPORT:	//不支持该卡片类型
        case TAG_ERR:			//卡片读取错误
        case TAG_COMM_ERR:		//不是本小区卡片         
        {
          beep.write(BEEP_ALARM);
          tag_updata_beforeUid(tag);
          sendLogFlag = FALSE;
          openResult =  FALSE;
        }break;   
        case TAG_TIME_ERR:	//临时卡过期卡片
        case TAG_BALCK_LIST_ERR:
        {
            beep.write_base(&beepBlackListErr);
            tag_updata_beforeUid(tag);
	    sendLogFlag = TRUE;
	    openResult = FALSE;
        }break;
        case TAG_SAME_ID_ERR://监测到同一张卡片
        case TAG_PHONE_ERR:	               //PHONE检测错误，重新和手机建立连接，延时2秒
        case TAG_PHONE_PWD_ERR:		//密码错误或者没有钥匙
        case TAG_PHONE_NO_KEY_ERR:
        case TAG_TYPE4A_ERR:
        case TAG_TYPE4B_ERR:
        default :
        {
            log(WARN,"[%s]No this mode = %d\n" , __func__, *result);
        }break;
    }

	
	if( sendLogFlag == TRUE )
	{	
          sys_delay(10);
          memset(&logData , 0x00 , sizeof(openlogDataType));
          logData.type = OPENLOG_FORM_CARD;
          logData.length = openResult;
          memcpy(logData.data , (uint8_t *)tag , sizeof(tagBufferType));
          journal.save_log(&logData);
	}
	
    *result = TAG_NONE;
    
    
}


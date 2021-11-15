#include "tslDatahand.h"
#include "open_log.h"
#include "open_door.h"
#include "magnet.h"
#include "bsp_rtc.h"
#include "unit.h"
#include "ladder.h"
#include "beep.h"
#include "chipFlash.h"

volatile uint8_t configTagEnableFlag = FALSE;

void tag_config_enable( uint8_t status)
{
    configTagEnableFlag = status;
}



uint8_t community_data_is_zero(TSLCommunityModel *tagCommunity)
{
    if( ( tagCommunity->building_id ==0)&&
        ( tagCommunity->floor_id ==0)&&
        ( tagCommunity->house_id ==0)&&
        ( tagCommunity->village_id ==0))
    {
        return TRUE;
    }
    
    return FALSE;
}


uint8_t tag_verify_village(SystemCommunitiesType   *deviceCommunity , TSLCommunityModel *community)
{
 
    if( community_data_is_zero(community) == TRUE )
    {
        log(WARN,"��Ƭ�ڴ洢��С����Ԫ¥�����Ϊ��\n");
        return FALSE;
    }
    
    if( deviceCommunity->super_code ==  community->village_id)
    {
    	log(DEBUG,"��������ԱȨ��ƥ����ȷ ,Super code = %X\n" , deviceCommunity->super_code);
        return TRUE;
    }
    
    for(uint8_t i = 0 ; i< 5 ; i++)
    {
        if( community->village_id == deviceCommunity->communities[i].village_id)
        {
        	log(DEBUG,"�ڵ�%d���洢λ��ƥ����ȷ ,Code = %X\n" , i+1,community->village_id);
            return TRUE;
        }
    }
    
    return FALSE;
}

uint8_t tag_terminus_unit_admin_process( tagBufferType *tag)
{
    tagUnitAdminDataType    tagUnitAdmin = {0};
    rtcTimeType time;
    TSLCommunityModel       communityTemp;
    SystemCommunitiesType   *deviceCommunity;
    uint32_t DateInCard,DateInSys;
   
    log(DEBUG,"����Ա��\n");
    memcpy((uint8_t*)&tagUnitAdmin,tag->buffer,sizeof(tagUnitAdminDataType));
    
    rtc.read_time(&time);
    DateInSys=(time.year<<24)|(time.mon<<16)|(time.day<<8)|(time.hour<<3)|((time.min/10)&0x07);            

    DateInCard=(comm_bcd_to_bin(tagUnitAdmin.stop[0])<<16)|(comm_bcd_to_bin(tagUnitAdmin.stop[1])<<8)|comm_bcd_to_bin(tagUnitAdmin.stop[2]);
    DateInCard=DateInCard<<8;
    if(tagUnitAdmin.start[2]==0xFC)
        DateInCard |=((comm_bcd_to_bin(tagUnitAdmin.start[0])<<3)|((comm_bcd_to_bin(tagUnitAdmin.start[1])/10)&0x07));
    else 
        DateInCard=0x65000000;
    
    log(DEBUG,"��Ƭ�洢ʱ�� = %x , �豸��ǰʱ�� =%x\n" ,DateInCard , DateInSys);
    
    if(DateInCard>DateInSys)
    {
        memset(&communityTemp , 0x00 , sizeof(TSLCommunityModel));
        config.read(CFG_SYS_COMMUN_CODE , (void **)&deviceCommunity);
        communityTemp.village_id = (tagUnitAdmin.CommunityID[0]<<24)|(tagUnitAdmin.CommunityID[1]<<16)|
                                    (tagUnitAdmin.CommunityID[2]<<8)|(tagUnitAdmin.CommunityID[3]);
        communityTemp.building_id = tagUnitAdmin.buildingUnitID[0]<<8|tagUnitAdmin.buildingUnitID[1];
        log(DEBUG,"��Ƭ�洢, С��ID =%x ,¥��ID=%x ,��ԪID=%x ,����� =%x\n" ,
                    communityTemp.village_id,communityTemp.building_id ,
                    communityTemp.floor_id,communityTemp.house_id);
        
        if( tag_verify_village(deviceCommunity , &communityTemp) == TRUE)
        {
			ladder_set_form_card((uint8_t *)tag , LADDER_ADMIN_POWER);
            return TAG_SUCESS;
        }
        
        if(tag->allDataCrc == FALSE )
        {
            log(WARN,"CRCУ��������¶�ȡ��Ƭ����(��Ƭ�洢�ĵ�һ��С����Ϣƥ�������Ҫƥ��ȫ����Ϣ��CRC��Ҫȫ��У��)\n");
            return TAG_CRC_ERR;
        }
    
        for( uint8_t i = 0 ; i < 4 ; i++)
        {
            memset(&communityTemp , 0x00 , sizeof(TSLCommunityModel));
            communityTemp.village_id = (tagUnitAdmin.communitys[i].communityID[0]<<24)|(tagUnitAdmin.communitys[i].communityID[1]<<16)|
                                    (tagUnitAdmin.communitys[i].communityID[2]<<8)|(tagUnitAdmin.communitys[i].communityID[3]);
            communityTemp.building_id = tagUnitAdmin.communitys[i].buildingID[0]<<8| tagUnitAdmin.communitys[i].buildingID[1];
            log(DEBUG,"��Ƭ�洢 (%d), С��ID =%x ,¥��ID=%x ,��ԪID=%x ,����� =%x\n" ,
                    i+1,
                    communityTemp.village_id,communityTemp.building_id ,
                    communityTemp.floor_id,communityTemp.house_id);

            if( tag_verify_village(deviceCommunity , &communityTemp) == TRUE)
            {
                return TAG_SUCESS;
            }
        
        }
        log(DEBUG,"�Ǳ�С����Ƭ\n");
        return TAG_COMM_ERR;
        
    }
    
    
    return TAG_TIME_ERR;
}


uint8_t tag_verify_community(TSLCommunityModel *tagCommunity)
{
    SystemCommunitiesType   *deviceCommunity;
    uint8_t  i = 0 , lock_mode= 0;
    
    config.read(CFG_SYS_COMMUN_CODE , (void **)&deviceCommunity);
    lock_mode = config.read(CFG_SYS_LOCK_MODE , NULL);
    log(ERR,"�豸����lock_mode = %d(0:��Ԫ�Ž� , 1:¥���Ž� ,2:С���Ž�)\n" , lock_mode);//1

    


  /*   00B9F06B B70A0000*/
//deviceCommunity->communities[0].village_id=0XB9F06B;
//deviceCommunity->communities[0].building_id=0XA00;


    if( community_data_is_zero(tagCommunity) == TRUE )
    {
    	log(WARN,"��Ƭ�ڴ洢��С����Ԫ¥�����Ϊ��\n");
        return TAG_COMM_ERR;
    }
    
    for( i = 0 ; i < 5 ; i++)
    {

        switch(lock_mode)
        {
            case VILLAGE_LOCK_MODE://2
            {
                log(ERR,"�����豸����Χǽ�� VILLAGE_LOCK_MODE \n");
                if( tagCommunity->village_id == deviceCommunity->communities[i].village_id)
                {
                    log(DEBUG,"��Ƭ�洢 , С��ID =%x ,¥��ID=%x ,��ԪID=%x ,����� =%x\n" ,
                        deviceCommunity->communities[i].village_id,deviceCommunity->communities[i].building_id ,
                        deviceCommunity->communities[i].floor_id,deviceCommunity->communities[i].house_id);
                    
                    return TAG_SUCESS;
                }            
            }break;
            case BULID_LOCK_MODE://1-----
            {
                if(( tagCommunity->village_id == deviceCommunity->communities[i].village_id)&&
               ( tagCommunity->building_id == deviceCommunity->communities[i].building_id))
                {
                    log(DEBUG,"��Ƭ�洢 , С��ID =%x ,¥��ID=%x ,��ԪID=%x ,����� =%x\n" ,
                        deviceCommunity->communities[i].village_id,deviceCommunity->communities[i].building_id ,
                        deviceCommunity->communities[i].floor_id,deviceCommunity->communities[i].house_id);
                    
                    return TAG_SUCESS;
                }
            }break;
            case UNIT_LOCK_MODE://0
            {
                if(( tagCommunity->village_id == deviceCommunity->communities[i].village_id)&&
                   ( tagCommunity->building_id == deviceCommunity->communities[i].building_id)&&
                   ( tagCommunity->floor_id == deviceCommunity->communities[i].floor_id) )
                {
                    log(DEBUG,"��Ƭ�洢 , С��ID =%x ,¥��ID=%x ,��ԪID=%x ,����� =%x\n" ,
                        deviceCommunity->communities[i].village_id,deviceCommunity->communities[i].building_id ,
                        deviceCommunity->communities[i].floor_id,deviceCommunity->communities[i].house_id);
                    
                    return TAG_SUCESS;
                } 
            }break;
            default:
            {
                log_err("�豸��֧�ִ�ģʽ\n");
            }break;
        }
    }

    return TAG_COMM_ERR;
}

uint8_t  tag_verify_base_information(tagBufferType *card , tagUserDataType *tag)
{
    TSLCommunityModel       communityTemp;
    

    uint8_t *communityID = NULL,*buildingID = NULL ,*house = NULL;

    log(DEBUG,"�豸���� = %d(0:��Ԫ�Ž� , 1:¥���Ž� ,2:С���Ž�)\n" , config.read(CFG_SYS_LOCK_MODE , NULL));
    
    communityID = tag->community.communityID;
    buildingID = tag->community.buildingID;
    house = tag->house0;
    
    communityTemp.village_id = (communityID[0]<<24)|(communityID[1]<<16)|(communityID[2]<<8)|(communityID[3]);
    communityTemp.building_id = (buildingID[0]<<8)|(buildingID[1]);
    communityTemp.floor_id = tag->community.unitID;
    communityTemp.house_id = (house[0]<<8)|house[1];
    
    if( tag_verify_community(&communityTemp) == TAG_SUCESS)
    {
        log(INFO ,"��С����Ƭ\n");
		ladder_set_form_card((uint8_t *)tag , LADDER_USER_POWER);
        return TAG_SUCESS;
    }
    else
    {
        log(DEBUG,"��ƬС����� =%x ,¥�����=%x ,��Ԫ���=%x ,����� =%x\n" ,
                        communityTemp.village_id,communityTemp.building_id ,
                        communityTemp.floor_id,communityTemp.house_id);

        log(DEBUG,"��С�����ʧ�ܣ���⿨Ƭ���д洢С���Ƿ���ƥ��\n");
    }

    if(card->allDataCrc == FALSE )
    {
        log(WARN,"С��ID2~5����Ϊ�գ�ֻ������С��Ȩ���ж�.\n");
        return TAG_COMM_ERR;
    }
    
    for(uint8_t i = 0 ; i < 4 ; i++)
    {
        memset(&communityTemp , 0x00 , sizeof(TSLCommunityModel));

        communityID = tag->communitys[i].communityID;
        buildingID = tag->communitys[i].buildingID;
        
        communityTemp.village_id = (communityID[0]<<24)|(communityID[1]<<16)|(communityID[2]<<8)|(communityID[3]);
        communityTemp.building_id = (buildingID[0]<<8)|(buildingID[1]);
        communityTemp.floor_id = tag->communitys[i].unitID;
        
        switch(i)
        {
            case 0: communityTemp.house_id = (tag->house1[0]<<8)|tag->house1[1]; break;
            case 1: communityTemp.house_id = (tag->house2[0]<<8)|tag->house2[1]; break;
            case 2: communityTemp.house_id = (tag->house3[0]<<8)|tag->house3[1]; break;
            case 3: communityTemp.house_id = (tag->house4[0]<<8)|tag->house4[1]; break;
            default: log_err("Get tag other house, no this case.\n");
        }
        
        log(DEBUG,"��Ƭ���� [%d] ,С��ID = %04X ,¥��ID = %02X ,��ԪID = %X ,house id =%02X\n" ,
                    i+1,
                    communityTemp.village_id,
                    communityTemp.building_id ,
                    communityTemp.floor_id,
                    communityTemp.house_id);
        
        if( tag_verify_community(&communityTemp) == TAG_SUCESS)
        {
            log(INFO ,"��С����\n");
			ladder_set_form_card((uint8_t *)tag , LADDER_USER_POWER);
            return TAG_SUCESS;
        }
    }
    
    log(DEBUG,"�Ǳ�С����Ƭ\n");
    return TAG_COMM_ERR;
    
}

uint8_t  tag_verify_time(tagBufferType *card , tagUserDataType *tag)
{
    uint32_t DateInCard,DateInSys;
    rtcTimeType time;
    
    rtc.read_time(&time);
    
    DateInCard=(comm_bcd_to_bin(tag->stop[0])<<24)|(comm_bcd_to_bin(tag->stop[1])<<16)|(comm_bcd_to_bin(tag->stop[2])<<8);
    DateInSys=(time.year<<24)|(time.mon<<16)|(time.day<<8)|(time.hour<<3)|((time.min/10)&0x07);            
        
    if(tag->cardType==0x03)
    {
        if((tag->start[2]==0xFF)||(tag->start[2]==0xFC))
        {
            DateInCard |=((comm_bcd_to_bin(tag->start[0])<<3)|((comm_bcd_to_bin(tag->start[1])/10)&0x07));
        }
        else
        {   
            DateInCard += ((23<<3)|0x06);
        }
        card->tagPower = TEMP_TAG;
        
        log(DEBUG,"��ʱ��\n");
        log(DEBUG,"��Ƭ�洢ʱ�� = %x , �豸��ǰʱ�� =%x\n" ,DateInCard , DateInSys);
    }
    else
    {
    	log(DEBUG,"ס����\n");
        if(tag->start[2]==0xFC)
        {
            DateInCard |=((comm_bcd_to_bin(tag->start[0])<<3)|((comm_bcd_to_bin(tag->start[1])/10)&0x07));
        }
        else 
        {
            DateInCard=0x65000000;
        }
        log(DEBUG,"��Ƭ�洢ʱ�� = %x , �豸��ǰʱ�� =%x\n" ,DateInCard , DateInSys);
    }
       
    
    if(DateInCard<DateInSys )
    {
        log(DEBUG,"���ڿ�\n");
        return TAG_TIME_ERR;
    }
    
    
    return TAG_SUCESS;
}


uint8_t tag_terminus_user_process( tagBufferType *tag)
{
    tagUserDataType         tagUser;
    uint8_t rt = TAG_SUCESS;
    
    memcpy((uint8_t*)&tagUser,tag->buffer,sizeof(tagUserDataType));
    

    if(tag_verify_time(tag ,  &tagUser))
    {
        return TAG_TIME_ERR;
    }
    
    
    if( (rt=tag_verify_base_information(tag ,  &tagUser)))
    {
        return rt;
    }

    return rt;
}
#include "httpbuss.h"
uint8_t tag_terminus_card_process( tagBufferType *tag)
{
    switch(tag->tagPower)
    {
        case USER_TAG:	
        {
            return (tag_terminus_user_process(tag));
        }break;

        case MANAGENT_TAG:
        {
            return (tag_terminus_unit_admin_process(tag));
        }break;
        case CONFIG_TAG:
          {
              Reset_HTTPStatus();
              log_err("��װ���ĺ��� ǿ���豸\n");
              httpRecvMsgType p;
              p.rule = 'B';
              xQueueSend(xHttpRecvQueue, (void*)&p, NULL);
          }break;
        case UINT_ADMIN_TAG:
        case UINT_MANAGENT_TAG:
        default:
        {
              log_err("��˾��Ƭ���ͣ���֧�ִ�Ȩ�޽���, POWER = %d.\n" , tag->tagPower);
              return TAG_NO_SUPPORT;
        }break;
    }
     return TAG_SUCESS;
}

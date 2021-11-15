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
        log(WARN,"卡片内存储的小区单元楼栋编号为空\n");
        return FALSE;
    }
    
    if( deviceCommunity->super_code ==  community->village_id)
    {
    	log(DEBUG,"超级管理员权限匹配正确 ,Super code = %X\n" , deviceCommunity->super_code);
        return TRUE;
    }
    
    for(uint8_t i = 0 ; i< 5 ; i++)
    {
        if( community->village_id == deviceCommunity->communities[i].village_id)
        {
        	log(DEBUG,"在第%d个存储位置匹配正确 ,Code = %X\n" , i+1,community->village_id);
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
   
    log(DEBUG,"管理员卡\n");
    memcpy((uint8_t*)&tagUnitAdmin,tag->buffer,sizeof(tagUnitAdminDataType));
    
    rtc.read_time(&time);
    DateInSys=(time.year<<24)|(time.mon<<16)|(time.day<<8)|(time.hour<<3)|((time.min/10)&0x07);            

    DateInCard=(comm_bcd_to_bin(tagUnitAdmin.stop[0])<<16)|(comm_bcd_to_bin(tagUnitAdmin.stop[1])<<8)|comm_bcd_to_bin(tagUnitAdmin.stop[2]);
    DateInCard=DateInCard<<8;
    if(tagUnitAdmin.start[2]==0xFC)
        DateInCard |=((comm_bcd_to_bin(tagUnitAdmin.start[0])<<3)|((comm_bcd_to_bin(tagUnitAdmin.start[1])/10)&0x07));
    else 
        DateInCard=0x65000000;
    
    log(DEBUG,"卡片存储时间 = %x , 设备当前时间 =%x\n" ,DateInCard , DateInSys);
    
    if(DateInCard>DateInSys)
    {
        memset(&communityTemp , 0x00 , sizeof(TSLCommunityModel));
        config.read(CFG_SYS_COMMUN_CODE , (void **)&deviceCommunity);
        communityTemp.village_id = (tagUnitAdmin.CommunityID[0]<<24)|(tagUnitAdmin.CommunityID[1]<<16)|
                                    (tagUnitAdmin.CommunityID[2]<<8)|(tagUnitAdmin.CommunityID[3]);
        communityTemp.building_id = tagUnitAdmin.buildingUnitID[0]<<8|tagUnitAdmin.buildingUnitID[1];
        log(DEBUG,"卡片存储, 小区ID =%x ,楼栋ID=%x ,单元ID=%x ,房间号 =%x\n" ,
                    communityTemp.village_id,communityTemp.building_id ,
                    communityTemp.floor_id,communityTemp.house_id);
        
        if( tag_verify_village(deviceCommunity , &communityTemp) == TRUE)
        {
			ladder_set_form_card((uint8_t *)tag , LADDER_ADMIN_POWER);
            return TAG_SUCESS;
        }
        
        if(tag->allDataCrc == FALSE )
        {
            log(WARN,"CRC校验错误，重新读取卡片数据(卡片存储的第一个小区信息匹配错误，需要匹配全部信息，CRC需要全部校验)\n");
            return TAG_CRC_ERR;
        }
    
        for( uint8_t i = 0 ; i < 4 ; i++)
        {
            memset(&communityTemp , 0x00 , sizeof(TSLCommunityModel));
            communityTemp.village_id = (tagUnitAdmin.communitys[i].communityID[0]<<24)|(tagUnitAdmin.communitys[i].communityID[1]<<16)|
                                    (tagUnitAdmin.communitys[i].communityID[2]<<8)|(tagUnitAdmin.communitys[i].communityID[3]);
            communityTemp.building_id = tagUnitAdmin.communitys[i].buildingID[0]<<8| tagUnitAdmin.communitys[i].buildingID[1];
            log(DEBUG,"卡片存储 (%d), 小区ID =%x ,楼栋ID=%x ,单元ID=%x ,房间号 =%x\n" ,
                    i+1,
                    communityTemp.village_id,communityTemp.building_id ,
                    communityTemp.floor_id,communityTemp.house_id);

            if( tag_verify_village(deviceCommunity , &communityTemp) == TRUE)
            {
                return TAG_SUCESS;
            }
        
        }
        log(DEBUG,"非本小区卡片\n");
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
    log(ERR,"设备类型lock_mode = %d(0:单元门禁 , 1:楼栋门禁 ,2:小区门禁)\n" , lock_mode);//1

    


  /*   00B9F06B B70A0000*/
//deviceCommunity->communities[0].village_id=0XB9F06B;
//deviceCommunity->communities[0].building_id=0XA00;


    if( community_data_is_zero(tagCommunity) == TRUE )
    {
    	log(WARN,"卡片内存储的小区单元楼栋编号为空\n");
        return TAG_COMM_ERR;
    }
    
    for( i = 0 ; i < 5 ; i++)
    {

        switch(lock_mode)
        {
            case VILLAGE_LOCK_MODE://2
            {
                log(ERR,"蓝加设备就是围墙机 VILLAGE_LOCK_MODE \n");
                if( tagCommunity->village_id == deviceCommunity->communities[i].village_id)
                {
                    log(DEBUG,"卡片存储 , 小区ID =%x ,楼栋ID=%x ,单元ID=%x ,房间号 =%x\n" ,
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
                    log(DEBUG,"卡片存储 , 小区ID =%x ,楼栋ID=%x ,单元ID=%x ,房间号 =%x\n" ,
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
                    log(DEBUG,"卡片存储 , 小区ID =%x ,楼栋ID=%x ,单元ID=%x ,房间号 =%x\n" ,
                        deviceCommunity->communities[i].village_id,deviceCommunity->communities[i].building_id ,
                        deviceCommunity->communities[i].floor_id,deviceCommunity->communities[i].house_id);
                    
                    return TAG_SUCESS;
                } 
            }break;
            default:
            {
                log_err("设备不支持此模式\n");
            }break;
        }
    }

    return TAG_COMM_ERR;
}

uint8_t  tag_verify_base_information(tagBufferType *card , tagUserDataType *tag)
{
    TSLCommunityModel       communityTemp;
    

    uint8_t *communityID = NULL,*buildingID = NULL ,*house = NULL;

    log(DEBUG,"设备类型 = %d(0:单元门禁 , 1:楼栋门禁 ,2:小区门禁)\n" , config.read(CFG_SYS_LOCK_MODE , NULL));
    
    communityID = tag->community.communityID;
    buildingID = tag->community.buildingID;
    house = tag->house0;
    
    communityTemp.village_id = (communityID[0]<<24)|(communityID[1]<<16)|(communityID[2]<<8)|(communityID[3]);
    communityTemp.building_id = (buildingID[0]<<8)|(buildingID[1]);
    communityTemp.floor_id = tag->community.unitID;
    communityTemp.house_id = (house[0]<<8)|house[1];
    
    if( tag_verify_community(&communityTemp) == TAG_SUCESS)
    {
        log(INFO ,"本小区卡片\n");
		ladder_set_form_card((uint8_t *)tag , LADDER_USER_POWER);
        return TAG_SUCESS;
    }
    else
    {
        log(DEBUG,"卡片小区编号 =%x ,楼栋编号=%x ,单元编号=%x ,房间号 =%x\n" ,
                        communityTemp.village_id,communityTemp.building_id ,
                        communityTemp.floor_id,communityTemp.house_id);

        log(DEBUG,"单小区检测失败，检测卡片所有存储小区是否有匹配\n");
    }

    if(card->allDataCrc == FALSE )
    {
        log(WARN,"小区ID2~5数据为空，只能做单小区权限判断.\n");
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
        
        log(DEBUG,"卡片数据 [%d] ,小区ID = %04X ,楼栋ID = %02X ,单元ID = %X ,house id =%02X\n" ,
                    i+1,
                    communityTemp.village_id,
                    communityTemp.building_id ,
                    communityTemp.floor_id,
                    communityTemp.house_id);
        
        if( tag_verify_community(&communityTemp) == TAG_SUCESS)
        {
            log(INFO ,"本小区卡\n");
			ladder_set_form_card((uint8_t *)tag , LADDER_USER_POWER);
            return TAG_SUCESS;
        }
    }
    
    log(DEBUG,"非本小区卡片\n");
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
        
        log(DEBUG,"临时卡\n");
        log(DEBUG,"卡片存储时间 = %x , 设备当前时间 =%x\n" ,DateInCard , DateInSys);
    }
    else
    {
    	log(DEBUG,"住户卡\n");
        if(tag->start[2]==0xFC)
        {
            DateInCard |=((comm_bcd_to_bin(tag->start[0])<<3)|((comm_bcd_to_bin(tag->start[1])/10)&0x07));
        }
        else 
        {
            DateInCard=0x65000000;
        }
        log(DEBUG,"卡片存储时间 = %x , 设备当前时间 =%x\n" ,DateInCard , DateInSys);
    }
       
    
    if(DateInCard<DateInSys )
    {
        log(DEBUG,"过期卡\n");
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
              log_err("安装卡的后门 强清设备\n");
              httpRecvMsgType p;
              p.rule = 'B';
              xQueueSend(xHttpRecvQueue, (void*)&p, NULL);
          }break;
        case UINT_ADMIN_TAG:
        case UINT_MANAGENT_TAG:
        default:
        {
              log_err("公司卡片类型，不支持此权限解析, POWER = %d.\n" , tag->tagPower);
              return TAG_NO_SUPPORT;
        }break;
    }
     return TAG_SUCESS;
}


#include "ladder.h"
#include "rs485.h"
#include "unit.h"
#include "open_door.h"


int floorOffset	 = 0;
uint16_t deviceFloor = 0;


void get_floor_offset(uint8_t *getFloor , uint8_t *userFloor , uint8_t offset)
{
#if USE_LADDER
    int  m =0;
    long long floor = 0;
    long long F =1;
    
    printf("Floor offser is %d.\r\n" , offset);
    for( uint8_t i = 0 ; i < 8 ; i++ )
	{
		for(uint8_t j = 0 ; j < 8 ; j++)
		{
            m++;
			if( (userFloor[i]&(0x01<<j) ) )
				floor |= F<< (64-m);
		}
	}
    
    floor <<= offset;
    m=0;
    for( uint8_t i = 0 ; i < 8 ; i++ )
	{
		for(uint8_t j = 0 ; j < 8 ; j++)
		{
            m++;
			if(floor &( F<< (64-m)))
				getFloor[i] |= 1<<j;
		}
	}
#endif
}

void ladder_set_form_app( uint8_t *msg)
{
#if USE_LADDER
	LadderUserMsgType  userMsg;
	uint8_t floor[8] ,floorTemp[8] , controlData[100] , controlLen =0;
	
	char	openType =0 ,userFloor[4]={0x00}, userRoom[5]={0x00} ,floorAscii[17] = {0x00};
	
	openType = msg[DATA_TYPE_OFFSET];	//get data type， if 0: user data. if 1:admin data.
	
	//get user floor
	memcpy(userFloor  , msg+USER_FLOOR_OFFSET ,USER_FLOOR_SIZE);
	userMsg.userFloor  = ((userFloor[0]<<8) | (userFloor[1]<<4) | (userFloor[2] ));
	
	//get user room
	memcpy(userRoom  , msg+USER_ROOM_OFFSET ,USER_ROOM_SIZE);
	userMsg.userRoom   = ((userRoom[0]<<12)|(userRoom[1]<<8) | (userRoom[2]<<4) | (userRoom[3] ));
	
	memcpy(floor      , msg+LIMIT_FLOOR_OFFSET ,LIMIT_FLOOR_SIZE);	
	for(uint8_t cnt = 0 ,i = 0 ; i < 16 ; i+=2 , cnt++)
	{
		char tmp[2]={0x00};

		memcpy(tmp , floorAscii+i , 2);
		floorTemp[cnt] = (tmp[0]<<4 |tmp[1]);
	}
	
	memset(floor , 0x00 , 8);
	get_floor_offset(floor ,floorTemp , floorOffset);
    memcpy((uint8_t *)&userMsg.floor , floor , 8);
	
	if(openType != 0x00)
    {
        log(DEBUG,"Use admin keys...\r\n");
        memset(userMsg.floor , 0xff , 8);
    }
	
	log(DEBUG,"Revc app data, converted data:\r\n");
	log(DEBUG,"User floor : %x \r\n" , userMsg.userFloor);
	log(DEBUG,"User_room : %x \r\n" , userMsg.userRoom);
	log(DEBUG,"Floor : 0x%x ,0x%x ,0x%x ,0x%x ,0x%x ,0x%x ,0x%x ,0x%x. \r\n" , userMsg.floor[0],userMsg.floor[1],userMsg.floor[2],userMsg.floor[3],userMsg.floor[4],userMsg.floor[5],userMsg.floor[6],userMsg.floor[7]);


	memset(controlData , 0x00 , 100);
	controlLen = get_control_data( &userMsg , controlData);
	
	rs485_puts(controlData , controlLen);
#endif
}

void ladder_set_card_user( uint8_t *data)
{
#if USE_LADDER
	LadderUserMsgType  userMsg;
	uint8_t  floor[8] , controlData[100] , controlLen =0;
	tagUserDataType *cardData = (tagUserDataType*)data;
	
	userMsg.userRoom = cardData->house0[0];
    userMsg.userFloor = cardData->house0[1];
    memcpy(userMsg.cardId , cardData->cardID , 4);
    memset((uint8_t *)&userMsg.floor , 0x00 , 8);
	
	if(cardData->id[0] == 0xFE)
	{
		memset(floor , 0x00 , 8);
		
		get_floor_offset(floor ,cardData->id+1 , floorOffset);
        log(DEBUG,"Nfc card floor : 0x%x ,0x%x ,0x%x ,0x%x ,0x%x ,0x%x ,0x%x ,0x%x. \r\n" , 
                 cardData->id[1],cardData->id[2],
                 cardData->id[3],cardData->id[4],
                 cardData->id[5],cardData->id[6],
                 cardData->id[7],cardData->id[8]);
        memcpy((uint8_t *)&userMsg.floor , floor , 8);
        log(DEBUG,"DeviceFloor = %d , NFC card user floor = %d.\r\n" , deviceFloor , cardData->userFloor);
        if( cardData->userFloor != deviceFloor)
        {
			log(INFO,"梯控楼层不是第一个楼层，清空梯控楼层数据(备注：目前梯控只允许第一个小区进行梯控控制).\n");
            memset((uint8_t *)&userMsg.floor , 0x00 , 8);
        }
		
		log(DEBUG,"Revc app data, converted data:\r\n");
		log(DEBUG,"User floor : %x \r\n" , userMsg.userFloor);
		log(DEBUG,"User_room : %x \r\n" , userMsg.userRoom);
		log(DEBUG,"Floor : 0x%x ,0x%x ,0x%x ,0x%x ,0x%x ,0x%x ,0x%x ,0x%x. \r\n" , userMsg.floor[0],userMsg.floor[1],userMsg.floor[2],userMsg.floor[3],userMsg.floor[4],userMsg.floor[5],userMsg.floor[6],userMsg.floor[7]);

		memset(controlData , 0x00 , 100);
		controlLen = get_control_data( &userMsg , controlData);

		rs485_puts(controlData , controlLen);

	}
	else
	{
		log(DEBUG,"该卡不支持梯控刷卡\n");
	}
#endif
}


void ladder_set_card_admin( void )
{
#if USE_LADDER
	uint8_t controlData[100] , controlLen =0;
	LadderUserMsgType  userMsg;
	
	log(DEBUG,"管理员卡，楼层全开\n");
	
	memset(&userMsg , 0xff , sizeof(LadderUserMsgType));
	memset(userMsg.floor , 0xff , 8);
	memset(controlData , 0x00 , 100);
	controlLen = get_control_data( &userMsg , controlData);

	rs485_puts(controlData , controlLen);
	
#endif	
}


void ladder_set_form_card( uint8_t* data  , ladderPowerEnum powerType )
{
#if USE_LADDER
	if(powerType == LADDER_ADMIN_POWER)
	{
		ladder_set_card_admin();
	}
	else
	{
		ladder_set_card_user(data);
	}
#endif	
}



void ladder_init( void )
{
#if USE_LADDER

	SystemCommunitiesType *communityTemp=NULL;
    config.read(CFG_SYS_COMMUN_CODE , (void **)&communityTemp);
	
	
	floorOffset	 = communityTemp->install_floor - communityTemp->begin_floor;
	deviceFloor =  communityTemp->communities[0].floor_id;
	
	log(DEBUG,"设备梯控偏移楼层=%d,设备安装楼栋=%d\n" , floorOffset , deviceFloor);
	
	open_ladder();
	
#endif
	
}

#if USE_LADDER
INIT_EXPORT(ladder_init  , "ladder");
#endif





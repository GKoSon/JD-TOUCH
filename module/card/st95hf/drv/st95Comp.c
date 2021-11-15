#include "st95Comp.h"
#include "hwconfig.h"
#include "lib_iso14443AType4.h"
#include "apdu_phone.h"
#include "mifare.h"

#define chartonumber(x) (x-'0')
#define numbertochar(x) (x+'0')
#define ST95HF_CMD_END_DATA    0x28

extern ISO14443A_CARD ISO14443A_Card;

DeviceMode_t devicemode = UNDEFINED_MODE;
TagType_t nfc_tagtype = UNDEFINED_TAG_TYPE;

uint8_t uCmdHeadData = 0x02;


/**
 *	@brief  This function set a variable to inform Manager that a task is on going
 *  @param  none
 *  @retval none
 */
void ConfigManager_Start( void )
{
	//StopProcess = false;
	//uAppliTimeOut = false;
	devicemode = UNDEFINED_MODE;
	nfc_tagtype = UNDEFINED_TAG_TYPE;
	
}


void st95FieldOff( tagBufferType *tag  )
{
	//PCD_FieldOff();
	sys_delay(10);
}



uint8_t st95Iso14443a_read_uuid(tagBufferType *card)
{
	uint8_t TagUID[UID_SIZE];

    memset(TagUID , 0x00 , UID_SIZE);
	// Start the config manager
	ConfigManager_Start();
	
	PCD_FieldOff();
	sys_delay(5);
	
	ISO14443A_Init( );
	if(ISO14443A_IsPresent() == RESULTOK)
	{			
		if(ISO14443A_Anticollision() == RESULTOK)
		{	
			memcpy(card->UID , ISO14443A_Card.UID , ISO14443A_Card.UIDsize);
			card->UIDLength = ISO14443A_Card.UIDsize;
			card->sak = ISO14443A_Card.SAK;
			return TRACK_NFCTYPE4A;		   
		}
	}
	
	return TRACK_NONE;
}

uint8_t st95Iso14443bReadUid( tagBufferType *card )
{
	uint8_t TagUID[UID_SIZE];

    memset(TagUID , 0x00 , UID_SIZE);
	// Start the config manager
	ConfigManager_Start();
	PCD_FieldOff();
	sys_delay(5);
	
	if(ISO14443B_IsPresent() == RESULTOK )
	{
		if(ISO14443B_Anticollision() == RESULTOK)
		{
			if (ISO14443B_GetUID(TagUID) == RESULTOK )
			{
				memcpy(card->UID , TagUID , ISO14443B_SIZE_UID);
				card->UIDLength = ISO14443B_SIZE_UID;
			}                
			return TRACK_NFCTYPE4B;
		}
	}
	
	return TRACK_NONE;
	
}

uint8_t st95ReadISO15693TagUid(tagBufferType *card)
{
	uint8_t TagUID[UID_SIZE];

    memset(TagUID , 0x00 , UID_SIZE);
	// Start the config manager
	ConfigManager_Start();
	PCD_FieldOff();
	sys_delay(5);
	
	if(ISO15693_GetUID (TagUID) == RESULTOK)	
	{
		memcpy(card->UID , TagUID , ISO15693_NBBYTE_UID);
		card->UIDLength = ISO15693_NBBYTE_UID;
		return TRACK_NFCTYPE5;
	}
	
	return TRACK_NONE;
}

uint8_t st95ReadISO15693Data(uint8_t Address , uint8_t Length ,uint8_t *Respone)
{
    uint8_t ReadDataTemp[128];
    uint8_t Ret;
    uint8_t tagDensity = ISO15693_LOW_DENSITY;
    
    if ( Ret = ISO15693_ReadBytesTagData(ISO15693_LOW_DENSITY, ISO15693_LRiS64K, ReadDataTemp, 16, 0) != ISO15693_SUCCESSCODE)
    {
        if ( Ret = ISO15693_ReadBytesTagData(ISO15693_STLEGLR_HIGH_DENSITY, ISO15693_LRiS64K, ReadDataTemp, 16, 0) != ISO15693_SUCCESSCODE)
        {
            if(Ret = ISO15693_ReadBytesTagData(ISO15693_LOW_DENSITY, ISO15693_LRiS64K, ReadDataTemp, 16, 0) != ISO15693_SUCCESSCODE)
            {
                return Ret;
            }
            else
            {
                tagDensity = ISO15693_LOW_DENSITY;
            }
						
        }
        else
        {
            tagDensity = ISO15693_STLEGLR_HIGH_DENSITY;
        }
    }
    
    Ret = ISO15693_ReadBytesTagData(tagDensity, ISO15693_LRiS64K, Respone, Length, Address);
    
    return Ret;
}


uint8_t st95ReadChongQingBusCardData(uint8_t *data)
{
	bool tls;
	
	if( chong_qing_bus_card( data , &tls) )
	{
		return TRUE;
	}
	
	return FALSE;
}

uint8_t st95ApduSelectApp( void )
{
	tagBufferType	tag;
	
	if( readFishFlag )
	{
		return FALSE;
	}
	uint8_t select_app_name[] ={0x02,0X00,0XA4,0X04,0X00,0X08,'t','e','r','m','i','n','u','s',0x28};

	if( st95Iso14443a_read_uuid(&tag) == TRACK_NONE)
	{
		log(DEBUG,"select uid is error\n");
	}
	
	if(  SelectApplicationFormApdu(select_app_name) == SELECT_APP_SUCCESS )
	{
		return TRUE;
	}
	
	return FALSE;
}

uint8_t  read_fk_card( tagBufferType *tag )
{
    return (read_fukai_card_data(tag->buffer,tag));
}


/******************************************************************************
*函数名 fm1208_send_recv
*函数功能描述 ： 向FM1208卡发送命令并返回数据
*函数参数1 ：uLen        命令长度
*函数参数2 ：pCmd        命令内容
*函数参数3 ：pResponse  返回数据
*函数返回值 ：返回状态
*作者 ： 汪林海
*函数创建日期 ：2018年01月09日
*函数修改日期 ：无
*修改人 ： 无
*修改原因 ： 无
*版本 ：1.0
*历史版本 ：无
*******************************************************************************/
uint8_t fm1208_send_recv(uint8_t uLen,uint8_t *pCmd,uint8_t *pResponse,uint8_t *oLen)
{
     uint8_t i;
     uint8_t DataToSend[80]={0}; 
     uint8_t DataRead[80]={0};
     
     DataToSend[0] = uCmdHeadData;           //命令开头要添加0x02或0x03
     memcpy(&(DataToSend[1]),pCmd,uLen);
     DataToSend[uLen+1] = ST95HF_CMD_END_DATA;   //命令结束要添加0x028
     if(PCD_SendRecv(uLen+2,(const uint8_t*) DataToSend, DataRead)!=ISO14443A_SUCCESSCODE)
            return ISO14443A_ERRORCODE_DEFAULT;
     
     *oLen = DataRead[1]-6;
       
     for(i=0;i<DataRead[1]-6;i++)*(pResponse+i)=*(DataRead+3+i);
     
     if(uCmdHeadData == 0x02)
     {
        uCmdHeadData = 0x03;
     }  
     else
     {
        uCmdHeadData = 0x02;
     }  
     return ISO14443A_SUCCESSCODE;
}

/******************************************************************************
*函数名 read_fm1208_card_data
*函数功能描述 ： 读取FM1208卡内文件数据
*函数返回值 ：返回读状态
*作者 ： 汪林海
*函数创建日期 ：2018年01月09日
*函数修改日期 ：无
*修改人 ： 无
*修改原因 ： 无
*版本 ：1.0
*历史版本 ：无
*******************************************************************************/
uint8_t st95ReadFm1208Data(uint8_t *keydata,uint8_t *readdata)
{
	uint8_t uSelectProperty[]={0x00,0xA4,0x00,0x00,0x02,0x3F,0x01};//选择文件目录3F01
	uint8_t uSelectReadBinFile[]={0x00,0xA4,0x00,0x00,0x02,0x00,0x04};//选择文件0004
	//uint8_t uSelectReadBinpin[21]={0x00,0x20,0x00,0x00,0x03,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,};//pin密钥提权
	uint8_t uSelectReadBinpin[]={0x00,0x20,0x00,0x00,0x03,0x12,0x34,0x56};
	uint8_t uSelectReadBinContent[]={0x00,0xB0,0x00,0x00,0x00,0x10};//读取文件0x10个字节
	uint8_t pDataRead[64]={0};
	uint8_t uDataLen=0,i=0;
	uint8_t keydatatmp[6]={0};
	uint8_t data[3]={0};
	// uint8_t readdatatmp[32]={0};
	tagBufferType	tag;
	uint8_t rx[256];
	
	if( st95Iso14443a_read_uuid(&tag) == TRACK_NONE)
	{
		log(DEBUG,"select uid is error\n");
	}
	
	if( ISO14443A_PPS( rx ) != ISO14443A_SUCCESSCODE )
	{
		log(DEBUG,"set pps is error\n");
	}
	
	uCmdHeadData = 0x02;

	for(i=0;i<6;i++)
	{  
		keydatatmp[i] = chartonumber(*(keydata+i));//+//chartonumber(*(keydata+i+1))<<4;
	}
								 
	data[0] = (keydatatmp[0]<<4) + keydatatmp[1];
	data[1] = (keydatatmp[2]<<4) + keydatatmp[3];
	data[2] = (keydatatmp[4]<<4) + keydatatmp[5];  

	memcpy(uSelectReadBinpin+5,data,3);
	memset(pDataRead,0,64);
	if(fm1208_send_recv(sizeof(uSelectProperty),uSelectProperty, pDataRead,&uDataLen)!=ISO14443A_SUCCESSCODE)
	{
		return ISO14443A_ERRORCODE_DEFAULT;
	}
	if(pDataRead[uDataLen-2]!= 0x90 || pDataRead[uDataLen-1]!= 0x00)
	{
		return ISO14443A_ERRORCODE_DEFAULT;
	}
	
	memset(pDataRead,0,64);
	if(fm1208_send_recv(sizeof(uSelectReadBinFile),uSelectReadBinFile, pDataRead,&uDataLen)!=ISO14443A_SUCCESSCODE)
	{
		return ISO14443A_ERRORCODE_DEFAULT;
	}
	if(pDataRead[uDataLen-2]!= 0x90 || pDataRead[uDataLen-1]!= 0x00) 
	{
		return ISO14443A_ERRORCODE_DEFAULT;
	}
	
	memset(pDataRead,0,64);
	if(fm1208_send_recv(sizeof(uSelectReadBinpin),uSelectReadBinpin, pDataRead,&uDataLen)!=ISO14443A_SUCCESSCODE)
	{
		return ISO14443A_ERRORCODE_DEFAULT;
	}
	if(pDataRead[uDataLen-2]!= 0x90 || pDataRead[uDataLen-1]!= 0x00) 
	{
		return ISO14443A_ERRORCODE_DEFAULT;
	}
	memset(pDataRead,0,64);
	if(fm1208_send_recv(sizeof(uSelectReadBinContent),uSelectReadBinContent, pDataRead,&uDataLen)!=ISO14443A_SUCCESSCODE)
	{
		return ISO14443A_ERRORCODE_DEFAULT;
	}
	
	if(pDataRead[uDataLen-2]!= 0x90 || pDataRead[uDataLen-1]!= 0x00) 
	{
		return ISO14443A_ERRORCODE_DEFAULT;
	}
	/*
	for(i=0;i<16;i++)
	{  
	readdatatmp[2*i] = (pDataRead[i]>>4)+0x30;//numbertochar(pDataRead[i]>>4);
	readdatatmp[2*i+1] = (pDataRead[i]&0x0f)+0x30;//numbertochar(pDataRead[i]&0x0f);
	}
	memcpy(readdata,readdatatmp,32);
	*/

	for(i=0;i<16;i++)
	{  
		readdata[2*i] = (pDataRead[i]>>4)+0x30;//numbertochar(pDataRead[i]>>4);
		readdata[2*i+1] = (pDataRead[i]&0x0f)+0x30;//numbertochar(pDataRead[i]&0x0f);
	}


	return ISO14443A_SUCCESSCODE;	
}

uint8_t st95ReadLikingData( uint8_t *keydata,uint8_t *readdata )
{
	if( st95ReadFm1208Data( keydata , readdata) == ISO14443A_SUCCESSCODE )
	{
		return TRUE;
	}
	
	return FALSE;
}

tagObjType	st95Tag = 
{
	.init	= st95hf_hal_init,
	.iso15693_get_uid = st95ReadISO15693TagUid,
	.iso15693_read_data = st95ReadISO15693Data,
	.iso14443a_get_uid = st95Iso14443a_read_uuid,
	.iso14443b_get_uid = st95Iso14443bReadUid,
	.read_m1_data = read_fk_card,
	.cqBus_read_data = st95ReadChongQingBusCardData,
	.apdu_select_app = st95ApduSelectApp,
	.write_and_read_app = st95WriteMessageToPhone,
	.fm1208_read_data = st95ReadLikingData,
	.turnOffField  = st95FieldOff,
};
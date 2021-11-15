#include "EncryDecry.h"
#include "stdint.h"
#include "rfal_rf.h"
#include "rfal_nfca.h"
#include "rfal_nfcb.h"
#include "rfal_nfcf.h"
#include "rfal_nfcv.h"
#include "rfal_st25tb.h"
#include "rfal_nfcDep.h"
#include "rfal_iso15693_2.h"
#include "iso15693_3.h"
#include "iso14443a.h"
#include "rfal_isoDep.h"
#include "appPhone.h"
#include "cqBus.h"
#include "syscfg.h"
#include "open_door.h"
#include "iso14443a_3.h"






uint8_t TaskID=0;
uint8_t ApduTimeHandle =0;
uint8_t ApduReadFlag = FALSE;
              


uint8_t tag_read_phone_nfc( void );



uint8_t checkConnectResponse( uint8_t *data , uint16_t len )
{
	uint8_t rsData[]={0x0A,0X0B,0X0C,0x0D,0x0E,0x0F};
	
	if( len == data[2]+3 )
	{
		
		//log_arry(DEBUG,"data" , data, len );
		if( aiot_strcmp(data+3 , rsData , 6) == TRUE )
		{
			return TRUE;
		}
	}
	
	//log(WARN,"返回数据长度有问题,rx len = %d , data len = %d\n" , len , data[2]);
	
	return FALSE;
	
}

uint8_t st25SelectApplication( void )
{
	uint16_t err = 0;
	uint8_t rx[256];
	uint16_t rxLen = 0;
	uint8_t fsdi = RFAL_ISODEP_FSXI_64;
	uint8_t cid = 0;
	uint8_t pss1 = RFAL_BR_106;
	uint8_t fscid = (cid&0xf) | ((fsdi&0xf)<<4);
	uint8_t selectAppname[] ={0x02,0X00,0XA4,0X04,0X00,0X08,'t','e','r','m','i','n','u','s'};

	memset(rx , 0x00 , 256);
	rxLen= 0 ;
	err = iso14443AEnterProtocolMode(fscid, rx, 128, &rxLen);
	if( err == ERR_NONE )
	{
		//log(DEBUG,"set rats success\n");
	}
	else
	{
		//log(INFO,"[%s] rats err = %d\n" , __func__ , err);
		return FALSE;
	}
        
    err = iso14443ASendProtocolAndParameterSelection(0, pss1);
	if( err == ERR_NONE )
	{
		//log(DEBUG,"set pps success\n");
	}
	else
	{
		//log(INFO,"[%s] pps err = %d\n" , __func__ , err);
		return FALSE;
	}
	
	err_chk(st25WriteAndReadIso14443aData(selectAppname , sizeof(selectAppname) , rx , &rxLen));
	
	if( checkConnectResponse( rx , rxLen ) == FALSE)
	{
		return FALSE;
	}
	
	
	//log_arry(DEBUG,"Response" , rx , rxLen);
	
	return TRUE;
}

uint8_t apduSelectApp( void )
{
	uint8_t rCnt = 10;
	tagBufferType tag;
		
	while(rCnt--)
	{
		if( st25SelectApplication() == TRUE )
		{
			return TRUE;
		}
		rfalFieldOff();
		sys_delay(10);
		iso14443a_read_uuid( &tag);
	}
		 
	return FALSE;
	
}



//发送数据至手机APP，并且接受返回的数据
uint8_t st25WriteMessageToPhone(uint8_t ucCmd,uint8_t ucType,uint8_t *pucSendBUff ,uint8_t ucBuffLength, ApduRecvDataType *pucReadBuff)
{
    ApduType SendApdu;
    uint8_t pucDataRead[256]={0x00};
    uint8_t ucSendLength = 0;
	uint16_t rxLen = 0;
	
	memset(&SendApdu , 0x00 , sizeof(ApduType));
    if(( ucBuffLength  > 0xF0) || (pucSendBUff==NULL))
    {
        log(DEBUG,"Get send message is to long or send buff is null.\r\n");
        goto error;
    }
    
    if( gucWriteOrReadCmd == CMD_WRITE_MODE)
    {
        gucWriteOrReadCmd = CMD_READ_MODE;
        memcpy(&SendApdu.pHdr , &gucWriteHeard , sizeof(ApduAgreementHeardType));
    }
    else
    {
        gucWriteOrReadCmd = CMD_WRITE_MODE;
        memcpy(&SendApdu.pHdr , &gucReadHeard , sizeof(ApduAgreementHeardType));
    }
    SendApdu.ucCmd = ucCmd;
    SendApdu.ucType = ucType;
    SendApdu.ucDataLength = ucBuffLength;
    memcpy(SendApdu.ucData , pucSendBUff , ucBuffLength);
    ucSendLength = ucBuffLength+8;
    log(DEBUG,"Send Leng = %d\n" , ucSendLength);
    if(st25WriteAndReadIso14443aData((uint8_t *)&SendApdu ,ucSendLength,pucDataRead ,&rxLen) == TRUE)
    {
        memset(pucReadBuff , 0x00 , sizeof (ApduRecvDataType));
        pucReadBuff->ucLength =rxLen -1;
        if( pucReadBuff->ucLength > 0xF0)
        {
            log(DEBUG,"Get select application message is to long ,data length = %d .\r\n" , pucReadBuff->ucLength);
            goto error;
        }
        memcpy(pucReadBuff ,pucDataRead ,rxLen);
        if(pucReadBuff->ucType == 130 )
        {
            log_arry(DEBUG,"Return Data" , pucDataRead , pucReadBuff->ucLength );
        }
        return WRITE_MSG_SUCCESS;
    }
    
    log_arry(DEBUG,"Return Data" , pucDataRead , pucReadBuff->ucLength );
error:

    return WRITE_MSG_ERR;
}





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
#include "cqBus.h"
#include "iso14443a_3.h"





uint8_t st25ReadChongQingBusCardData(uint8_t *respone)
{
	uint16_t err = 0;
	uint8_t rx[256];
	uint16_t rxLen = 0;
	uint16_t i = 0;
	uint8_t fsdi = RFAL_ISODEP_FSXI_64;
	uint8_t cid = 0;
	uint8_t pss1 = RFAL_BR_106;
	uint8_t fscid = (cid&0xf) | ((fsdi&0xf)<<4);
	
	uint8_t selectDir[] ={0x02,0x00,0xA4,0x00,0x00,0x02,0x3F,0x00};//读通卡卡号
	uint8_t readCard[]={0x03,0x00,0xB0,0x85,0x0C,0x08};//读通卡卡号 返回 0x80 0x10 0x3 0x40 0x0 0x0 0x2 0x12 0x0 0x39 0x63 0x90 0x0 0xEB 0xCC 0x8 0x0 0x0 
	
	uint8_t uSelectProperty[]={0x02,0x00,0xA4,0x00,0x00,0x02,0xDC,0x06};//选择地产物业应用目录
	uint8_t uSelectReadBinFile[]={0x03,0x00,0xA4,0x00,0x00,0x02,0x00,0x15};//读取其他二进制文件（0015）前48字节 0x30
	uint8_t uSelectReadBinContent[]={0x02,0x00,0xB0,0x00,0x00,0x30};//读取文件内容
	uint8_t uSelectReadBinFile1[]={0x03,0x00,0xA4,0x00,0x00,0x02,0x00,0x15};//读取其他二进制文件（0015）从0x30 后的 48字节
	uint8_t uSelectReadBinContent1[]={0x02,0x00,0xB0,0x00,0x30,0x35};//读取文件内容
	
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
	
	
	err_chk(st25WriteAndReadIso14443aData(selectDir , sizeof(selectDir) , rx , &rxLen));
	err_chk(st25WriteAndReadIso14443aData(readCard , sizeof(readCard), rx , &rxLen));
	
	err_chk(st25WriteAndReadIso14443aData(uSelectProperty , sizeof(uSelectProperty) , rx , &rxLen));
	err_chk(st25WriteAndReadIso14443aData(uSelectReadBinFile , sizeof(uSelectReadBinFile) , rx , &rxLen));
	
	err_chk(st25WriteAndReadIso14443aData(uSelectReadBinContent , sizeof(uSelectReadBinContent) , rx , &rxLen));
	for( i = 0; i < 48; i++)
	{
		*(respone+i)=*(rx+i);
	}
	
	err_chk(st25WriteAndReadIso14443aData(uSelectReadBinFile1 , sizeof(uSelectReadBinFile1) , rx , &rxLen));
	err_chk(st25WriteAndReadIso14443aData(uSelectReadBinContent1 , sizeof(uSelectReadBinContent1) , rx , &rxLen));
	if((*(rx+48)!='T')||(*(rx+49)!='S')||(*(rx+50)!='L'))
	{
		log(DEBUG,"验证TSL字段错误\n");
  		return FALSE;
	}
	for( i = 0; i < 48; i++)
	{
		*(respone+48+i)=*(rx+i);
	}
	
	return TRUE;
	
}


#include "lib_iso14443Apcd.h"
#include "lib_iso14443AType4.h"
#include "Drv95HF.h"

extern ISO14443A_CARD ISO14443A_Card;
uint8_t TransBuffer[256]={0};

uint8_t ISO14443AType4_Init(  )
{
    uint8_t ProtocolSelect[]={0x02,0x04,0x02,0x00,0x01,0xA0};
    uint8_t WriteRegisterParameters[]={0x09,0x04,0x3a,0x00,0x5F,0x04};
    uint8_t Gain[]={0x09,0x04,0x68,0x01,0x01,0xdF};
    uint8_t *pDataRead = TransBuffer;
    
    if( drv95HF_SendReceive( ProtocolSelect,pDataRead) != RESULTOK)return ERRORCODE_GENERIC;
    if( drv95HF_SendReceive( WriteRegisterParameters,pDataRead) != RESULTOK)return ERRORCODE_GENERIC;
    if( drv95HF_SendReceive( Gain,pDataRead) != RESULTOK)return ERRORCODE_GENERIC;
    return RESULTOK;
}
uint8_t GetISO14443AType4UUID()
{
 //   uint8_t uReqA[]={0x26,0x07};
    uint8_t uAnticol[]={0x93,0x20,0x08};//防冲突
    uint8_t uSelect1[]={0x93,0x70,0x00,0x00,0x00,0x00,0x00,0x28};
    uint8_t uRats[]={0xE0,0x50,0x28};
    uint8_t uPps[]={0xD0,0x11,0x00,0x28};
    uint8_t *pDataRead = TransBuffer;
    int8_t status;

    errchk(PCD_SendRecv(sizeof(uAnticol),uAnticol,pDataRead));
    if(pDataRead[2] == 0x88)
        ISO14443A_Card.CascadeLevel =0x02;

    if(pDataRead[0] == 0x80 && pDataRead[1] == 0x03)
        return ERRORCODE_GENERIC;

    memcpy(ISO14443A_Card.UID , &pDataRead[2] , 5);
    memcpy(uSelect1+2 ,ISO14443A_Card.UID,5 );   
    
           
    errchk(PCD_SendRecv(sizeof(uSelect1),uSelect1, pDataRead));
    
    ISO14443A_Card.SAK = pDataRead[PCD_DATA_OFFSET];
    if((ISO14443A_Card.SAK & SAK_FLAG_ATS_SUPPORTED)==SAK_FLAG_ATS_SUPPORTED)
    {
        errchk(PCD_SendRecv(sizeof(uRats),uRats , pDataRead));
        errchk(PCD_SendRecv(sizeof(uPps),uPps , pDataRead));
    }
	return ISO14443A_SUCCESSCODE;
Error:
	return ISO14443A_ERRORCODE_DEFAULT; 
}

uint8_t ReadTongCardId(uint8_t *uTongCardId)
{// 选择通卡主目录
  const uint8_t uSelectDir[] ={0x02,0x00,0xA4,0x00,0x00,0x02,0x3F,0x00,0x28};//读通卡卡号
  const uint8_t uReadCard[]={0x03,0x00,0xB0,0x85,0x0C,0x08,0x28};//读通卡卡号 返回 0x80 0x10 0x3 0x40 0x0 0x0 0x2 0x12 0x0 0x39 0x63 0x90 0x0 0xEB 0xCC 0x8 0x0 0x0 
    //uint8_t uReadCard[]={0x03,0x00,0xB0,0x95,0x00,0x07,0x28};//0x80 0x8 0x3 0x6A 0x82 0x4F 0x75 0x8 0x0 0x0 
    //uint8_t uReadCard[]={0x03,0x00,0xB0,0x95,0x00,0x00,0x28};//0x80 0x8 0x3 0x6A 0x82 0x4F 0x75 0x8 0x0 0x0 
    //uint8_t uReadCard[]={0x03,0x00,0xB0,0x00,0x00,0x00,0x28};//读取其他二进制文件（0033）
  uint8_t *pDataRead = TransBuffer;
  int8_t status;
  
  errchk(PCD_SendRecv(sizeof(uSelectDir),uSelectDir, pDataRead));

  errchk(PCD_SendRecv(sizeof(uReadCard),uReadCard, pDataRead));
  memcpy(uTongCardId,pDataRead+3,8);
  return ISO14443A_SUCCESSCODE;
Error:
  return ISO14443A_ERRORCODE_DEFAULT; 
}

//uint8_t guDattt[256];
uint8_t CheckTongCard(uint8_t *uRead,BOOL *bTsl)
{
  const uint8_t uSelectProperty[]={0x02,0x00,0xA4,0x00,0x00,0x02,0xDC,0x06,0x28};//选择地产物业应用目录
  const uint8_t uSelectReadBinFile[]={0x03,0x00,0xA4,0x00,0x00,0x02,0x00,0x15,0x28};//读取其他二进制文件（0015）前48字节 0x30
  const uint8_t uSelectReadBinContent[]={0x02,0x00,0xB0,0x00,0x00,0x30,0x28};//读取文件内容
  const uint8_t uSelectReadBinFile1[]={0x03,0x00,0xA4,0x00,0x00,0x02,0x00,0x15,0x28};//读取其他二进制文件（0015）从0x30 后的 48字节
  const uint8_t uSelectReadBinContent1[]={0x02,0x00,0xB0,0x00,0x30,0x35,0x28};//读取文件内容
  uint8_t i,uTongCardId[8];
  uint8_t pDataRead[260];// = uRead;
  
  if(ReadTongCardId(uTongCardId)!=ISO14443A_SUCCESSCODE)return ISO14443A_ERRORCODE_DEFAULT;

  if(PCD_SendRecv(sizeof(uSelectProperty),uSelectProperty, pDataRead)!=ISO14443A_SUCCESSCODE)return ISO14443A_ERRORCODE_DEFAULT;
  if(PCD_SendRecv(sizeof(uSelectReadBinFile),uSelectReadBinFile, pDataRead)!=ISO14443A_SUCCESSCODE)return ISO14443A_ERRORCODE_DEFAULT;
  memset(pDataRead,0,64);
  if(PCD_SendRecv(sizeof(uSelectReadBinContent),uSelectReadBinContent, pDataRead)!=ISO14443A_SUCCESSCODE)return ISO14443A_ERRORCODE_DEFAULT;
  
  for(i=0;i<48;i++)*(uRead+i)=*(pDataRead+3+i);
  
  if(PCD_SendRecv(sizeof(uSelectReadBinFile1),uSelectReadBinFile1, pDataRead)!=ISO14443A_SUCCESSCODE)return ISO14443A_ERRORCODE_DEFAULT;
  memset(pDataRead,0,64);
  if(PCD_SendRecv(sizeof(uSelectReadBinContent1),uSelectReadBinContent1, pDataRead)!=ISO14443A_SUCCESSCODE)return ISO14443A_ERRORCODE_DEFAULT;

  if((*(pDataRead+3+48)=='T')&&(*(pDataRead+3+49)=='S')&&(*(pDataRead+3+50)=='L'))*bTsl=TRUE;
  
  for(i=0;i<48;i++)*(uRead+48+i)=*(pDataRead+3+i);

  return ISO14443A_SUCCESSCODE;
}

uint8_t CheckISO14443AType4()
{
    PCD_FieldOff();
    nSpi->DelayMs(5);
    ISO14443AType4_Init();
    
    if(ISO14443A_IsPresent() == RESULTOK)
    {			
      if(ISO14443A_Card.cardType !=BUS_CARD)
      {
        return ERRORCODE_GENERIC;
      }
      if(GetISO14443AType4UUID() == RESULTOK)return RESULTOK;
    }
    return ERRORCODE_GENERIC;
}


BOOL ReadTrafficCardData(uint8_t *uBuff,BOOL *bTsl)
{
  uint8_t uRead[128];
  memset(uRead,0,128);*bTsl=FALSE;
  if(CheckISO14443AType4()== RESULTOK)
  {
    if(ISO14443A_Card.SAK&0x20)
    {//检查是否为通卡
      if(CheckTongCard(uRead,bTsl)==ISO14443A_SUCCESSCODE)
      {
          memcpy(uBuff,&uRead,96);
        return TRUE;
      }
    }
  }
  return FALSE;
}
//交通卡 读数据入口
bool chong_qing_bus_card(uint8_t *uBuff, bool *tsl)
{
  if(ReadTrafficCardData(uBuff,tsl))
  {
      return TRUE;
  }
  return FALSE;
}

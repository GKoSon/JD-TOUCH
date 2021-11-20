#include "BleDataHandle.h"
#include "config.h"
#include "buletooth.h"
#include "beep.h"
#include "open_door.h"
#include "permi_list.h"
#include "open_log.h"
#include "magnet.h"
#include "buletooth.h"
#include "esp12s.h"
#include "mqtt_task.h"
#include "timer.h"
#include "cmd_pb.h"
#include "TSLBleCommon.pb.h"
#include "TSLBleUser.pb.h"
#include "TSLBleWizard.pb.h"



//extern _SHType SHType;


uint8_t FTSLBLEDeviceInfoRequest            (ProtData_T *pag,uint16_t id);
uint8_t FTSLBLEDeviceSetPWDRequest          (ProtData_T *pag,uint16_t id);
uint8_t FTSLBLEDeviceSetGUPRequest          (ProtData_T *pag,uint16_t id);
uint8_t FTSLBLEDeviceSetMQTTNETRequest      (ProtData_T *pag,uint16_t id);
uint8_t FTSLBLEDeviceSetDEVNETRequest       (ProtData_T *pag,uint16_t id);
uint8_t FTSLBLEDeviceLOCKRequest            (ProtData_T *pag,uint16_t id);
uint8_t FTSLBLEDeviceSetDEVICECODERequest    (ProtData_T *pag,uint16_t id);
uint8_t FTSLBLEDeviceSetLOCATIONCODERequest  (ProtData_T *pag,uint16_t id);
uint8_t FTSLBLEDeviceSetLOCKPWDRequest       (ProtData_T *pag,uint16_t id);

AppHandleArryType gAppTaskArry[]={
{0x0100 ,0x0200 , FTSLBLEDeviceInfoRequest  },

{0x0300 ,0x0400 , FTSLBLEDeviceSetPWDRequest},
{0x0306 ,0x0400 , FTSLBLEDeviceSetLOCKPWDRequest},

{0x0301 ,0x0400 , FTSLBLEDeviceSetGUPRequest},
{0x0302 ,0x0400 , FTSLBLEDeviceSetMQTTNETRequest },
{0x0303 ,0x0400 , FTSLBLEDeviceSetDEVNETRequest },
{0x0304 ,0x0400 , FTSLBLEDeviceSetDEVICECODERequest },
{0x0305 ,0x0400 , FTSLBLEDeviceSetLOCATIONCODERequest},


{0x0500 ,0x0600 , FTSLBLEDeviceLOCKRequest },

};


uint16_t  ble_mode_packet(uint8_t *out ,uint8_t *msg ,uint16_t len,uint16_t idtype, ProtData_T *headpag)
{
    uint16_t size=0;
    uint16_t crc16=0;
    ProtData_T *pag=(ProtData_T *)out;

    memcpy(pag,headpag,3);
    
    pag->POS1415_len =len+4;//T+L?TCRC
    pag->POS2122T =idtype;
    pag->POS2324L = len;

    size = len+9;
    
    pag->POS1415_len = exchangeBytes( pag->POS1415_len);
    pag->POS2122T =    exchangeBytes( pag->POS2122T);
    pag->POS2324L =    exchangeBytes( pag->POS2324L);

    memcpy(pag->POS25V,msg,len );

    crc16 =  crc16_ccitt(out ,size);
   // printf(" crc16_ccitt(out ,size)=%04x\r\n",crc16);     //http://www.ip33.com/crc.html     XMODEM

    out[size] =   (crc16&0XFF00)>>8; 
    out[size+1] =  crc16&0X00FF;
    
    
    return size+2;
}


uint8_t BleApplicationHandle(ProtData_T *pag)
{ 
    uint8_t idx =0 ;
    //show_ProtData(pag);
    for(idx = 0; idx < sizeof (gAppTaskArry) / sizeof (gAppTaskArry[0]); idx++)
    {
        if( pag->POS2122T == gAppTaskArry[idx].Rxid)
        {
            log(INFO,"Bt receive command TYPE = 0X%04X\r\n" ,  gAppTaskArry[idx].Rxid);
            return (gAppTaskArry[idx].EventHandlerFn(pag,gAppTaskArry[idx].Txid));               
        }
    }
    log(DEBUG,"This command TYP has no register. cmd = %x.\r\n" ,pag->POS2122T );
    return APP_ERR;
}

static uint8_t BleAppDataDecry(ProtData_T *sorpag ,  ProtData_T *despag){ return APP_OK;}

void BleDataProcess(ProtData_T *sorpag)
{
    ProtData_T   pag;

    memset(&pag , 0x00 ,   sizeof(ProtData_T));
    memcpy(&pag , sorpag , sizeof(ProtData_T));      
    if( BleAppDataDecry(sorpag,&pag) != APP_OK)
    {    
        beep.write(BEEP_ALARM);   
        return ;
    }
    if( BleApplicationHandle(&pag) != APP_OK)
    {      
        beep.write(BEEP_ALARM);   
        return ;
    }
     beep.write(BEEP_NORMAL);  
     return;
}



//¨ª¡§¨®???¡äe
int BLEWIZ_return_comm( ProtData_T *pag, uint8_t status ,uint16_t idtype )
{
    uint8_t pbBuf[100] , msg[256] , size = 0;
    BytesType B1; 
    uint8_t back[3][5]={"YES","E-ERR","E-PWD"};
    uint8_t *p;
    TslBLEProto_TSLBLEDeviceCommonResponse    result = TslBLEProto_TSLBLEDeviceCommonResponse_init_zero; 
    pb_ostream_t RequestStream = pb_ostream_from_buffer(pbBuf, 100);  

    result.status = status;
    

    if(status==0)          p = back[0];    
    else if(status==1)     p = back[1];
    else if(status==2)     p = back[2];  
    
    pb_add_bytes(&B1 , p , strlen((char *)p)); 

    pb_encode_bytes(&result.reserved , &B1); 
    
    log(DEBUG,"BLEWIZ_return_comm BLE ACK   [%s]\r\n , " ,p);
    
    if( pb_encode_respone(&RequestStream , TslBLEProto_TSLBLEDeviceCommonResponse_fields , &result) == TRUE)
    {
        size = ble_mode_packet(msg ,pbBuf , RequestStream.bytes_written, idtype,pag);

        btModule.send(pag ,msg,size);
    }

    return 1;
}

//2¨¦?¡¥??¨¢?
int BLEWIZ_return_info( ProtData_T *pag,uint16_t idtype )
{
    uint8_t pbBuf[256] , msg[256] , size = 0;
    uint8_t manufacturers[]={"TERMINUS"};
    //_DeviceInfo *devinfo;
    TslBLEProto_TSLBLEDeviceInfoResponse    result = TslBLEProto_TSLBLEDeviceInfoResponse_init_zero; 
    pb_ostream_t RequestStream = pb_ostream_from_buffer(pbBuf, 256);  

   // result.security = config.read(BLE_PAIR_PWD ,NULL);
    
   // config.read(BLE_DEV_INFO , (void **)&devinfo );
   // result.dev_type = devinfo->dev_type;
    //show_DeviceInfo(devinfo);
    BytesType B1,B2,B3,B4,B5,B6,B7;    
    //pb_add_bytes(&B1 , devinfo->sn , strlen((char *)devinfo->sn));   
    //pb_add_bytes(&B2 , manufacturers , strlen((char *)manufacturers)); 
    //pb_add_bytes(&B3 , devinfo->dev_name , strlen((char *)devinfo->dev_name));    
    //pb_add_bytes(&B4 , devinfo->software_version , strlen((char *)devinfo->software_version));    
    //pb_add_bytes(&B5 , devinfo->hardware_version , strlen((char *)devinfo->hardware_version)); 
    //pb_add_bytes(&B6 , SHType.codedev       ,11);    
    //pb_add_bytes(&B7 , SHType.codelocation  ,11); 
    
    pb_encode_bytes(&result.sn ,                &B1);
    pb_encode_bytes(&result.manufacturers ,     &B2);   
    pb_encode_bytes(&result.dev_name ,          &B3);
    pb_encode_bytes(&result.software_version ,  &B4);
    pb_encode_bytes(&result.hardware_version ,  &B5);
    pb_encode_bytes(&result.device_code ,       &B6);
    pb_encode_bytes(&result.location_code ,     &B7);

    if( pb_encode_respone(&RequestStream , TslBLEProto_TSLBLEDeviceInfoResponse_fields , &result) == TRUE)
    {
        //printf("RequestStream.bytes_written=0X%02X && idtype=%04x\r\n",RequestStream.bytes_written,idtype);
        size = ble_mode_packet(msg ,pbBuf , RequestStream.bytes_written ,idtype,pag);
        //log_arry(DEBUG,"MSG "  ,msg ,size);
        btModule.send(pag ,msg,size);
    }

    return 1;
}

uint8_t FTSLBLEDeviceInfoRequest (ProtData_T *pag,uint16_t id)
{SHOWME
  uint32_t pwd=0;
  pb_istream_t requestStream = pb_istream_from_buffer((const uint8_t*)pag->POS25V,pag->POS2324L); 
  TslBLEProto_TSLBLEDeviceInfoRequest	A = TslBLEProto_TSLBLEDeviceInfoRequest_init_zero; 
  if(pb_decode(&requestStream, TslBLEProto_TSLBLEDeviceInfoRequest_fields, &A) == TRUE )		
  {			
   // pwd= config.read(BLE_PAIR_PWD ,NULL);
    //printf("\r\n A.security = %d\r\n",A.security);
    //printf("\r\n pwd = %d\r\n",pwd);
    if(A.security == pwd )
    {
      BLEWIZ_return_info(pag,id);
      return APP_OK;
    }
    else
    {
      BLEWIZ_return_comm(pag,2,id);
      return APP_PWD_ERR;
    }
  }
   
  BLEWIZ_return_comm(pag,1,id);
  return APP_ERR;

}
uint8_t FTSLBLEDeviceSetPWDRequest (ProtData_T *pag,uint16_t id)
{SHOWME
  uint32_t pwd=0;
  pb_istream_t requestStream = pb_istream_from_buffer((const uint8_t*)pag->POS25V,pag->POS2324L); 
  TslBLEProto_TSLBLEDeviceSetPWDRequest	A = TslBLEProto_TSLBLEDeviceSetPWDRequest_init_zero; 
  if(pb_decode(&requestStream, TslBLEProto_TSLBLEDeviceSetPWDRequest_fields, &A) == TRUE )		
  {			
    printf("\r\n set new pwd = %d\r\n",A.security);
    pwd=A.security;
    //config.write(BLE_PAIR_PWD , &pwd,1);
    BLEWIZ_return_comm(pag,0,id);
    return APP_OK;
  }
  BLEWIZ_return_comm(pag,1,id);
  return APP_ERR;
}

uint8_t FTSLBLEDeviceSetLOCKPWDRequest (ProtData_T *pag,uint16_t id)
{SHOWME
  uint32_t pwd=0;
  pb_istream_t requestStream = pb_istream_from_buffer((const uint8_t*)pag->POS25V,pag->POS2324L); 
  TslBLEProto_TSLBLEDeviceSetLOCKPWDRequest	A = TslBLEProto_TSLBLEDeviceSetLOCKPWDRequest_init_zero; 
  if(pb_decode(&requestStream,TslBLEProto_TSLBLEDeviceSetLOCKPWDRequest_fields , &A) == TRUE )		
  {			
    printf("\r\n set new door pwd = %d\r\n",A.pwd);
    pwd=A.pwd;
    //config.write(BLE_DOOR_PWD , &pwd,1);
    BLEWIZ_return_comm(pag,0,id);
    return APP_OK;
  }
  BLEWIZ_return_comm(pag,1,id);
  return APP_ERR;
}


uint8_t FTSLBLEDeviceSetGUPRequest (ProtData_T *pag,uint16_t id)
{SHOWME
  uint8_t   tem[23],  defgup[11];
  memset(tem,0,sizeof(tem));
  pb_istream_t requestStream = pb_istream_from_buffer((const uint8_t*)pag->POS25V,pag->POS2324L); 
  TslBLEProto_TSLBLEDeviceSetGUPRequest	A = TslBLEProto_TSLBLEDeviceSetGUPRequest_init_zero; 

  pb_decode_bytes(&A.groupid ,        tem);
  if(pb_decode(&requestStream, TslBLEProto_TSLBLEDeviceSetGUPRequest_fields, &A) == TRUE )	
  {
    
    log(ERR,"tem = %s \n" ,tem); 
    G_strsTobytes(tem,defgup,22);
    log_arry(ERR,"FTSLBLEDeviceSetGUPRequest defgup" ,defgup ,11);
    //config.write(CFG_SYS_DEFGUP_CODE , defgup,1);
    BLEWIZ_return_comm(pag,0,id);
    return APP_OK;
  }
  
  BLEWIZ_return_comm(pag,1,id);
  return APP_ERR;

}
 
void ip_port_handle(uint8_t *  sor)
{
  char *p=0;
  serverAddrType ip_port;
//tcp://192.168.66.34:3001
  if(p = strstr ((const char*)sor,"//"))
    p+=2;
  else
    p = (char *)sor;
  GIPStringtoarry((uint8_t *)p,ip_port.ip);

  p = strstr ((const char*)p,":");
  ++p;
  ip_port.port = atoi(p);

  //show_serverAddr(&ip_port);
  //config.write(CFG_NET_ADDR ,&ip_port,1);

}
         
uint8_t FTSLBLEDeviceSetMQTTNETRequest (ProtData_T *pag,uint16_t id)
{SHOWME
  uint8_t   tem[50];
  memset(tem,0,sizeof(tem));

  pb_istream_t requestStream = pb_istream_from_buffer((const uint8_t*)pag->POS25V,pag->POS2324L); 
  TslBLEProto_TSLBLEDeviceSetMQTTNETRequest	A = TslBLEProto_TSLBLEDeviceSetMQTTNETRequest_init_zero; 
  pb_decode_bytes(&A.ipport ,        tem);

  if(pb_decode(&requestStream, TslBLEProto_TSLBLEDeviceSetMQTTNETRequest_fields, &A) == TRUE )	
  {
    printf("FTSLBLEDeviceSetMQTTNETRequest=%s\r\n",tem);

    ip_port_handle(tem);
    
    BLEWIZ_return_comm(pag,0,id);
    
    return APP_OK;
  }
  BLEWIZ_return_comm(pag,1,id);
  return APP_ERR;  
}



void dev_net_handle(uint8_t (*a)[16],uint8_t isdhcp)
{
  DeviceIpType dnet;
  memset(&dnet,0,sizeof(DeviceIpType));
  if(isdhcp)
  {
    dnet.dhcpFlag=1;
  }

  else
  {
    GIPStringtoarry(a[0],dnet.ip);
    GIPStringtoarry(a[1],dnet.mark);
    GIPStringtoarry(a[2],dnet.gateway);
    GIPStringtoarry(a[3],dnet.dns);
  }
  
  //show_DeviceIpType(&dnet);
  //config.write(CFG_SYS_DEVIP ,&dnet,1);

}


uint8_t FTSLBLEDeviceSetDEVNETRequest (ProtData_T *pag,uint16_t id)
{SHOWME
  uint8_t   tem[4][16];
  memset(tem,0,sizeof(tem));

  pb_istream_t requestStream = pb_istream_from_buffer((const uint8_t*)pag->POS25V,pag->POS2324L); 
  TslBLEProto_TSLBLEDeviceSetDEVNETRequest	A = TslBLEProto_TSLBLEDeviceSetDEVNETRequest_init_zero; 
  pb_decode_bytes(&A.ip ,         tem[0]);
  pb_decode_bytes(&A.msk ,        tem[1]);
  pb_decode_bytes(&A.gw ,         tem[2]);
  pb_decode_bytes(&A.dns ,        tem[3]);
  if(pb_decode(&requestStream, TslBLEProto_TSLBLEDeviceSetDEVNETRequest_fields, &A) == TRUE )	
  {
    //for(char i=0;i<4;i++)
    //{ printf("tem=%s\r\n",tem[i]);    log_arry(ERR,"tem"       ,tem[i] ,16);  }

    dev_net_handle(tem,A.isdhcp);

    BLEWIZ_return_comm(pag,0,id);
    
    //xSemaphoreGive(xBleUpdateNetSemaphore);//FINISH
    
    return APP_OK;
  }
  BLEWIZ_return_comm(pag,1,id);
  return APP_ERR;

}  

#include "swipeTag.h"
void ble_door_log(TslBLEProto_TSLBLEDeviceLOCKRequest	*A)
{
  openlogDataType	logData;         
  tagBufferType     tag;

  memset(&logData , 0x00 , sizeof(openlogDataType));
  memset(&tag ,     0x00 , sizeof(tagBufferType));
  
//¡ä¨®¦Ì?¡À¡ê¡ä??¨°?¨¹ ??¨¨£¤journal_add_into_card
  logData.type =  OPENLOG_FORM_CARD;
  logData.length = 1;
//D?¦Ì?¡¤????¨°?¨¹  
  tag.type= TAG_SHANGHAI_CARD;
  tag.tagPower=BLE_CARD;
  tag.UIDLength=8;
  memcpy(tag.UID , (uint8_t *)"12345678" , 8);// // memcpy(tag.UID , (uint8_t *)&A->uid , 10);
  
  
  memcpy(logData.data , (uint8_t *)&tag , sizeof(tagBufferType)); 
  journal.save_log(&logData);
  
  //journal_add_into_card   SAVE
  //journal_puck_string     TX
}

uint8_t FTSLBLEDeviceLOCKRequest (ProtData_T *pag,uint16_t id)
{SHOWME

  pb_istream_t requestStream = pb_istream_from_buffer((const uint8_t*)pag->POS25V,pag->POS2324L); 
  TslBLEProto_TSLBLEDeviceLOCKRequest	A = TslBLEProto_TSLBLEDeviceLOCKRequest_init_zero; 
  if(pb_decode(&requestStream, TslBLEProto_TSLBLEDeviceLOCKRequest_fields, &A) == TRUE )		
  {			
    printf("\r\n type??1--¡ã2¡Á¡ã1¡è 2--¨®??¡ì 3--¡¤??¨ª?? = %d\r\n",A.type);
    printf("\r\n security = %d\r\n",A.security);
    printf("\r\n uid = %lld\r\n",A.uid);
    printf("\r\n timeStamp = %lld\r\n",A.timeStamp);
   // printf("\r\n config.read(BLE_PAIR_PWD* BLE_DOOR_PWD-ok ,NULL) = %d\r\n", config.read(BLE_DOOR_PWD ,NULL)); 
    if(A.type==1)
    {
     // if(A.security ==  config.read(BLE_DOOR_PWD ,NULL)  )
     // {open_door(); BLEWIZ_return_comm(pag,0,id);}
     // else
     // {BLEWIZ_return_comm(pag,2,id);}
    }
    if(A.type==2)
    {
        uint32_t stamp = 0;
        stamp = rtc.read_stamp();
        log(ERR,"BLE¨º¡À??=%lld   ¨¦¨¨¡À?¦Ì¡À?¡ã¨º¡À??¡ä¨¢=%u\n" , A.timeStamp,stamp);
        if( abs(stamp-A.timeStamp) < 5)
        {
         {open_door(); BLEWIZ_return_comm(pag,0,id);ble_door_log(&A);}
        }
        else
        {log(ERR,"¨º¡À??¨°¨¬3¡ê\n"); }
     }
    return APP_OK;
  }
  
  BLEWIZ_return_comm(pag,1,id);
  return APP_ERR;
}


uint8_t FTSLBLEDeviceSetDEVICECODERequest (ProtData_T *pag,uint16_t id)
{SHOWME
  uint8_t   tem[30];
  memset(tem,0,sizeof(tem));
 
  
  pb_istream_t requestStream = pb_istream_from_buffer((const uint8_t*)pag->POS25V,pag->POS2324L); 
  TslBLEProto_TSLBLEDeviceSetDEVICECODERequest	A = TslBLEProto_TSLBLEDeviceSetDEVICECODERequest_init_zero; 
  
  pb_decode_bytes(&A.code ,        tem);
  
  if(pb_decode(&requestStream, TslBLEProto_TSLBLEDeviceSetDEVICECODERequest_fields, &A) == TRUE )		
  {			
    log_arry(ERR,"FTSLBLEDeviceSetDEVICECODERequest" ,tem ,11);
    //memcpy( SHType.codedev ,      tem     ,11);    
    //config.write(CFG_SYS_SHANGHAI,&SHType , TRUE);
    BLEWIZ_return_comm(pag,0,id);
    return APP_OK;
  }
  
  BLEWIZ_return_comm(pag,1,id);
  return APP_ERR;
}


uint8_t FTSLBLEDeviceSetLOCATIONCODERequest (ProtData_T *pag,uint16_t id)
{SHOWME
  uint8_t   tem[30];
  memset(tem,0,sizeof(tem));
  
  pb_istream_t requestStream = pb_istream_from_buffer((const uint8_t*)pag->POS25V,pag->POS2324L); 
  TslBLEProto_TSLBLEDeviceSetLOCATIONCODERequest	A = TslBLEProto_TSLBLEDeviceSetLOCATIONCODERequest_init_zero; 
  
    
  pb_decode_bytes(&A.code ,        tem);
  if(pb_decode(&requestStream, TslBLEProto_TSLBLEDeviceSetLOCATIONCODERequest_fields, &A) == TRUE )		
  {			
    log_arry(ERR,"FTSLBLEDeviceSetLOCATIONCODERequest" ,tem ,11);
      
   // memcpy( SHType.codelocation  ,tem,     11);
    
    //memset(&SHType.gup,0,(sizeof(_SHType) - offsetof(_SHType,gup)));
    
    BLEWIZ_return_comm(pag,0,id);
    
    //config.write(CFG_SYS_SHANGHAI,&SHType , TRUE);
    
    set_clear_flash( FLASH_PERMI_LIST_BIT|FLASH_PWD_BIT );
    
    return APP_OK;
  }
  
  BLEWIZ_return_comm(pag,1,id);
  return APP_ERR;
}

#include "BleDataHandle.h"
#include "config.h"
#include "buletooth.h"
#include "beep.h"
#include "open_door.h"
#include "permi_list.h"
#include "open_log.h"
#include "magnet.h"
#include "buletooth.h"
#include "mqtt_task.h"
#include "timer.h"
#include "cmd_pb.h"
#include "BleProtocol.pb.h"



//extern _SHType SHType;


uint8_t FTSLBLEDeviceInfoRequest            (BleProtData *pag);


AppHandleArryType gAppTaskArry[]={
{0x0100 , FTSLBLEDeviceInfoRequest  },


};

#define BLE_DEBUG_LOG(type, message)                                           \
do                                                                            \
{                                                                             \
    if (type)                                                                 \
        printf message;                                                   \
}                                                                             \
while (0)
#define BLE_DEBUG                                                     1



void show_BleProtData(BleProtData *p)
{
  
  BLE_DEBUG_LOG(BLE_DEBUG, ("\r\n"));
  BLE_DEBUG_LOG(BLE_DEBUG, (" ---HEAD:ID---0X%02X----\r\n",p->id.data));
  BLE_DEBUG_LOG(BLE_DEBUG, (" ---HEAD:CMD---0X%02X----\r\n",p->cmd));
  BLE_DEBUG_LOG(BLE_DEBUG, (" ---HEAD:SEQ---0X%02X----\r\n",p->num.data));
  BLE_DEBUG_LOG(BLE_DEBUG, (" ---HEAD:LEN---0X%02X----\r\n",p->len));

  log_arry(DEBUG,"---DATA---" ,p->body ,p->len);
  //printf("---------0X%02X----\r\n",p->alllen);
}

/*
组包  完成HEAD-BODY结构发出去
*/

static uint16_t  ble_mode_packet(uint8_t *out ,uint8_t *msg ,uint16_t len, BleProtData *in)
{
uint16_t size=0;

BleProtData *pag=(BleProtData *)out;

pag->id.data = in->id.data;
pag->cmd = 0x01;
pag->num.data = 0x10;
pag->len = len;
size = pag->len +4;
memcpy(pag->body,msg,len );
show_BleProtData(pag);
return size;
}

/********************************下行1****************************/

/*
message DeviceOpenRequest {
   int32 pwd = 1;    	//开门密码 
   int32 type=2;     	//0:安装工、1:用户 2:访客
   int64 uid=3;      	//用户标识
   int64 timeStamp=4;  	//时间戳
   string phoneNo=5;  	//手机号
}
*/
uint8_t Down_DeviceOpenRequest (BleProtData *pag)
{SHOWME
  uint8_t   phoneNo[20]={0};
  pb_istream_t requestStream ;// pb_istream_from_buffer((const uint8_t*)pag->POS25V,pag->POS2324L); 
  DeviceOpenRequest A = DeviceOpenRequest_init_zero; 
  pb_decode_bytes(&A.phoneNo ,phoneNo);
  if(pb_decode(&requestStream, DeviceOpenRequest_fields, &A) == TRUE )		
  {			
  BLE_DEBUG_LOG(BLE_DEBUG, ("【BLE】pwd      =%d\n",A.pwd));
  BLE_DEBUG_LOG(BLE_DEBUG, ("【BLE】type     =%d\n",A.type));
  BLE_DEBUG_LOG(BLE_DEBUG, ("【BLE】uid      =%d\n",A.uid));
  BLE_DEBUG_LOG(BLE_DEBUG, ("【BLE】timeStamp=%lld\n",A.timeStamp));
  BLE_DEBUG_LOG(BLE_DEBUG, ("【BLE】phoneNo  =%s\n",phoneNo));
    log_arry(ERR,"phoneNo" ,phoneNo ,12);
    printf("phoneNo %s" ,phoneNo);
    if(A.type==1)
    {

    } else if(A.type==2)
    {
        uint32_t stamp = 0;
        stamp = rtc.read_stamp();
        BLE_DEBUG_LOG(BLE_DEBUG, ("【BLE】A.timeStamp =%lld rtc.read_stamp()=%lld\n",A.timeStamp,stamp));
        if( abs(stamp-A.timeStamp) < 5)
        {
         
        }
        else
        {
        
        }
     }
    return APP_OK;
  }
  

  return APP_ERR;
}
/*


message DeviceSetDeviceNameRequest {
        string name = 1;	//设备名称
	string code = 2;	//设备编码
	int32 pairPWD = 3;	//配对密码
	int32 openPWD = 4;	//开门密码
	int32 openDelay = 5;	// 开门延迟
	int32 alarmDelay = 6;	// 开门报警延迟
	int32 installPurpose = 7;	//安装用途 0:单元机  1：围墙机 2：多围墙 
	string  mqttServer = 8; //设置mqtt server ip:port信息
	string  ntpServer = 9;//设置ntp server ip:port信息
	int32   isdhcp = 10;   //0:手动、1:自动
	string  ip = 11; 	  //设备ip
	string  gateway = 12;  //网关
	string  mask = 13; 	  //子网掩码
	string  dns = 14; 	  //dns
	string  groupId = 15;  //默认通行组
}

*/
uint8_t Down_DeviceSetDeviceNameRequest (BleProtData *pag)
{SHOWME
uint8_t   name[20]={0};
uint8_t   code[20]={0};
uint8_t   mqttServer[20]={0};
uint8_t   ntpServer[20]={0};
uint8_t   ip[20]={0};
uint8_t   gateway[20]={0};
uint8_t   mask[20]={0};
uint8_t   dns[20]={0};
uint8_t   groupId[20]={0};
  pb_istream_t requestStream ;//pb_istream_from_buffer((const uint8_t*)pag->POS25V,pag->POS2324L); 
  DeviceSetDeviceNameRequest A = DeviceSetDeviceNameRequest_init_zero; 
  
  
pb_decode_bytes(&A.name , name);
pb_decode_bytes(&A.code , code);
pb_decode_bytes(&A.mqttServer , mqttServer);
pb_decode_bytes(&A.ntpServer , ntpServer);
pb_decode_bytes(&A.ip , ip);
pb_decode_bytes(&A.gateway , gateway);
pb_decode_bytes(&A.mask , mask);
pb_decode_bytes(&A.dns , dns);
pb_decode_bytes(&A.groupId , groupId);

  if(pb_decode(&requestStream, DeviceSetDeviceNameRequest_fields, &A) == TRUE )		
  {			
BLE_DEBUG_LOG(BLE_DEBUG, ("【BLE】name           =%s\n",name));
BLE_DEBUG_LOG(BLE_DEBUG, ("【BLE】code           =%s\n",code));
BLE_DEBUG_LOG(BLE_DEBUG, ("【BLE】pairPWD        =%d\n",A.pairPWD));
BLE_DEBUG_LOG(BLE_DEBUG, ("【BLE】openPWD        =%d\n",A.openPWD));
BLE_DEBUG_LOG(BLE_DEBUG, ("【BLE】openDelay      =%d\n",A.openDelay));
BLE_DEBUG_LOG(BLE_DEBUG, ("【BLE】alarmDelay     =%d\n",A.alarmDelay));
BLE_DEBUG_LOG(BLE_DEBUG, ("【BLE】installPurpose =%d\n",A.installPurpose));
BLE_DEBUG_LOG(BLE_DEBUG, ("【BLE】mqttServer     =%s\n",mqttServer));
BLE_DEBUG_LOG(BLE_DEBUG, ("【BLE】ntpServer      =%s\n",ntpServer));
BLE_DEBUG_LOG(BLE_DEBUG, ("【BLE】isdhcp         =%d\n",A.isdhcp));
BLE_DEBUG_LOG(BLE_DEBUG, ("【BLE】ip             =%s\n",ip));
BLE_DEBUG_LOG(BLE_DEBUG, ("【BLE】gateway        =%s\n",gateway));
BLE_DEBUG_LOG(BLE_DEBUG, ("【BLE】mask           =%s\n",mask));
BLE_DEBUG_LOG(BLE_DEBUG, ("【BLE】dns            =%s\n",dns));
BLE_DEBUG_LOG(BLE_DEBUG, ("【BLE】groupId        =%s\n",groupId));
  }
   return APP_ERR;
}

/********************************上行1****************************/
int UP_Return_Comm( BleProtData *pag, uint8_t status ,uint16_t type )
{
    uint8_t pbBuf[100] , msg[256] , size = 0;
    BytesType B1; 
    uint8_t back[2][4]={"YES","ERR"};
    uint8_t *p;
    DeviceCommonResponse B = DeviceCommonResponse_init_zero; 
    pb_ostream_t RequestStream = pb_ostream_from_buffer(pbBuf, 100);  

    B.status = status;
    

    if(status==0)           p = back[0];    
    else if(status==1)     p = back[1];
   
    
    pb_add_bytes(&B1 , p , strlen((char *)p)); 
    pb_encode_bytes(&B.reserved , &B1); 
    
    BLE_DEBUG_LOG(BLE_DEBUG, ("【BLE】UP_Return_Comm=%s\n",p));
    
    if( pb_encode_respone(&RequestStream , DeviceCommonResponse_fields , &B) == TRUE)
    {
        size = ble_mode_packet(msg ,pbBuf , RequestStream.bytes_written, pag);

        btModule.send(pag->hdr.FormAddr ,pag->hdr.Handle, msg,size);
    }

    return 1;
}

uint8_t BleDataHandleDetails(BleProtData *pag)
{ 
    uint8_t idx =0 ;
    show_BleProtData(pag);
    for(idx = 0; idx < sizeof (gAppTaskArry) / sizeof (gAppTaskArry[0]); idx++)
    {
        if( pag->cmd == gAppTaskArry[idx].Rxid)
        {
            log(INFO,"Bt receive command TYPE = 0X%04X\r\n" ,  gAppTaskArry[idx].Rxid);
           // return (gAppTaskArry[idx].EventHandlerFn(pag,gAppTaskArry[idx].Txid));               
        }
    }
    
    UP_Return_Comm(pag,0,1);
    log(DEBUG,"This command TYP has no register. cmd = %x.\r\n" ,pag->cmd );
    return APP_ERR;
}

void BleDataHandle(BleProtData *sorpag)
{
    BleProtData   pag;

    memset(&pag , 0x00 ,   sizeof(BleProtData));
    memcpy(&pag , sorpag , sizeof(BleProtData));      

    if( BleDataHandleDetails(&pag) != APP_OK)
    {      
        beep.write(BEEP_ALARM);   
        return ;
    }
     beep.write(BEEP_NORMAL);  
     return;
}





//2é?ˉ??á?
//int BLEWIZ_return_info( ProtData_T *pag,uint16_t idtype )
//{
    //uint8_t pbBuf[256] , msg[256] , size = 0;

    //_DeviceInfo *devinfo;
    //TslBLEProto_TSLBLEDeviceInfoResponse    result = TslBLEProto_TSLBLEDeviceInfoResponse_init_zero; 
    //pb_ostream_t RequestStream = pb_ostream_from_buffer(pbBuf, 256);  

   // result.security = config.read(BLE_PAIR_PWD ,NULL);
    
   // config.read(BLE_DEV_INFO , (void **)&devinfo );
   // result.dev_type = devinfo->dev_type;
    //show_DeviceInfo(devinfo);
    //BytesType B1,B2,B3,B4,B5,B6,B7;    
    //pb_add_bytes(&B1 , devinfo->sn , strlen((char *)devinfo->sn));   
    //pb_add_bytes(&B2 , manufacturers , strlen((char *)manufacturers)); 
    //pb_add_bytes(&B3 , devinfo->dev_name , strlen((char *)devinfo->dev_name));    
    //pb_add_bytes(&B4 , devinfo->software_version , strlen((char *)devinfo->software_version));    
    //pb_add_bytes(&B5 , devinfo->hardware_version , strlen((char *)devinfo->hardware_version)); 
    //pb_add_bytes(&B6 , SHType.codedev       ,11);    
    //pb_add_bytes(&B7 , SHType.codelocation  ,11); 
    
    //pb_encode_bytes(&result.sn ,                &B1);
    //pb_encode_bytes(&result.manufacturers ,     &B2);   
    //pb_encode_bytes(&result.dev_name ,          &B3);
    //pb_encode_bytes(&result.software_version ,  &B4);
    //pb_encode_bytes(&result.hardware_version ,  &B5);
    //pb_encode_bytes(&result.device_code ,       &B6);
    //pb_encode_bytes(&result.location_code ,     &B7);

    //if( pb_encode_respone(&RequestStream , TslBLEProto_TSLBLEDeviceInfoResponse_fields , &result) == TRUE)
    //{
        //printf("RequestStream.bytes_written=0X%02X && idtype=%04x\r\n",RequestStream.bytes_written,idtype);
        //size = ble_mode_packet(msg ,pbBuf , RequestStream.bytes_written ,idtype,pag);
        //log_arry(DEBUG,"MSG "  ,msg ,size);
       // btModule.send(pag ,msg,size);
   // }

   // return 1;
//}

uint8_t FTSLBLEDeviceInfoRequest (BleProtData *pag)
{SHOWME


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
  //GIPStringtoarry((uint8_t *)p,ip_port.ip);

  p = strstr ((const char*)p,":");
  ++p;
  ip_port.port = atoi(p);

  //show_serverAddr(&ip_port);
  //config.write(CFG_NET_ADDR ,&ip_port,1);

}
         

#include "swipeTag.h"
void ble_door_log(DeviceOpenRequest	*A)
{
  openlogDataType	logData;         
  tagBufferType     tag;

  memset(&logData , 0x00 , sizeof(openlogDataType));
  memset(&tag ,     0x00 , sizeof(tagBufferType));
  
  logData.type =  OPENLOG_FORM_CARD;
  logData.length = 1;
 
  tag.type= TAG_SHANGHAI_CARD;
  tag.tagPower=BLE_CARD;
  tag.UIDLength=8;
  memcpy(tag.UID , (uint8_t *)"12345678" , 8);
  
  
  memcpy(logData.data , (uint8_t *)&tag , sizeof(tagBufferType)); 
  journal.save_log(&logData);
  
  //journal_add_into_card   SAVE
  //journal_puck_string     TX
}

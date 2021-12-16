#include "BleDataHandle.h"
#include "config.h"
#include "buletooth.h"
#include "beep.h"
#include "open_door.h"
#include "open_log.h"
#include "timer.h"
#include "cmd_pb.h"
#include "BleProtocol.pb.h"


void ble_door_log(DeviceOpenRequest	*A);

uint8_t down_device_A9    (BleProtData *pag);
uint8_t down_device_info  (BleProtData *pag);
uint8_t down_device_open  (BleProtData *pag);
uint8_t down_device_card  (BleProtData *pag);

AppHandleArryType gAppTaskArry[]={
{3 , down_device_open  },
{4 , down_device_info  },
{5 , down_device_A9    },
{0 , down_device_card  },
};


#define BLE_DEBUG_LOG(type, message)                                           \
do                                                                            \
{                                                                             \
    if (type)                                                                 \
        printf message;                                                   \
}                                                                             \
while (0)
#define BLE_DEBUG                                                        1



static void show_BleProtData(BleProtData *p)
{
  BLE_DEBUG_LOG(BLE_DEBUG, ("\r\n"));
  BLE_DEBUG_LOG(BLE_DEBUG, ("---HEAD:ID  0X%02X\r\n",p->id.data));
  BLE_DEBUG_LOG(BLE_DEBUG, ("---HEAD:CMD 0X%02X\r\n",p->cmd));
  BLE_DEBUG_LOG(BLE_DEBUG, ("---HEAD:SEQ 0X%02X\r\n",p->num.data));
  BLE_DEBUG_LOG(BLE_DEBUG, ("---HEAD:LEN 0X%02X\r\n",p->len));
  BLE_DEBUG_LOG(BLE_DEBUG, ("---HEAD:LEN 0X%02X\r\n",p->len));
  BLE_DEBUG_LOG(BLE_DEBUG, ("---BODY:LEN 0X%02X\r\n",p->bodylen));
  log_arry(DEBUG,"---BODY" ,p->body ,p->bodylen);
}

/********************************上行1****************************/
/*
组包  完成HEAD-BODY结构发出去
*/

static uint16_t  ble_mode_packet(uint8_t *out ,uint8_t *msg ,uint16_t len, BleProtData *in)
{
  uint16_t size=0;

  BleProtData *pag=(BleProtData *)out;

  pag->id.data = in->id.data;
  pag->cmd = 0x01;
  pag->num.data = 0x11;/*控制在1包回答完毕*/
  pag->len = len;
  pag->bodylen = len;
  size = pag->len +4;
  memcpy(pag->body,msg,len );
  //show_BleProtData(pag);
  return size;
}

/*
message DeviceCommonResponse {
        int32   status = 1;           
	string  reserved = 2; 
}
*/
int up_return_comm( BleProtData *pag, uint8_t status)
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
    
    BLE_DEBUG_LOG(BLE_DEBUG, ("【BLE】up_return_comm=%s\n",p));
    
    if( pb_encode_respone(&RequestStream , DeviceCommonResponse_fields , &B) == TRUE)
    {
        size = ble_mode_packet(msg ,pbBuf , RequestStream.bytes_written, pag);

        btModule.send(pag->hdr.FormAddr ,pag->hdr.Handle, msg,size);
    }

    return 1;
}


uint8_t down_device_A9 (BleProtData *pag)
{SHOWME
  
  up_return_comm(pag,0);

  sys_delay(3000);
  
  soft_system_resert(__FUNCTION__);
  
  return APP_OK;
}

uint8_t down_device_card (BleProtData *pag)
{
   if(pag->id.data==0X33){
    open_door(); ble_door_log(NULL);SHOWME
  }
  up_return_comm(pag,0);
  return APP_OK;
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
uint8_t down_device_open (BleProtData *pag)
{SHOWME
  uint8_t   phoneNo[20]={0};
  pb_istream_t requestStream = pb_istream_from_buffer((const uint8_t*)pag->body,pag->bodylen); 
  DeviceOpenRequest A = DeviceOpenRequest_init_zero; 
  pb_decode_bytes(&A.phoneNo ,phoneNo);
  if(pb_decode(&requestStream, DeviceOpenRequest_fields, &A) == TRUE )		
  {			
      BLE_DEBUG_LOG(BLE_DEBUG, ("【BLE】pwd      =%d\n",A.pwd));
      BLE_DEBUG_LOG(BLE_DEBUG, ("【BLE】type     =%d\n",A.type));
      BLE_DEBUG_LOG(BLE_DEBUG, ("【BLE】uid      =%lld\n",A.uid));
      BLE_DEBUG_LOG(BLE_DEBUG, ("【BLE】timeStamp=%lld\n",A.timeStamp));
      BLE_DEBUG_LOG(BLE_DEBUG, ("【BLE】phoneNo  =%s\n",phoneNo));

           
      if(A.type==0)  {
          open_door();
      } else if(A.type)  {
          uint32_t stamp = 0;
          stamp = rtc.read_stamp();
          BLE_DEBUG_LOG(BLE_DEBUG, ("【BLE】A.timeStamp =%lld rtc.read_stamp()=%u\n",A.timeStamp,stamp));
          if( abs(stamp-A.timeStamp) < 5)   {
             open_door();
             ble_door_log(&A);
          }
          else  {
        
          }
     }
         
    up_return_comm(pag,0);
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
uint8_t down_device_info (BleProtData *pag)
{
    uint8_t   name[16]={"NO_HANDLE"};
    uint8_t   code[22]={0};
    uint8_t   mqttServer[35]={0};
    uint8_t   ntpServer[20]={0};
    uint8_t   ip[20]={0};
    uint8_t   gateway[20]={0};
    uint8_t   mask[20]={0};
    uint8_t   dns[20]={0};

    pb_istream_t requestStream = pb_istream_from_buffer((const uint8_t*)pag->body,pag->bodylen); 
    DeviceSetDeviceNameRequest A = DeviceSetDeviceNameRequest_init_zero; 
      
      
    //pb_decode_bytes(&A.name , name);
    pb_decode_bytes(&A.code , code);
    pb_decode_bytes(&A.mqttServer , mqttServer);
    pb_decode_bytes(&A.ntpServer , ntpServer);
    pb_decode_bytes(&A.ip , ip);
    pb_decode_bytes(&A.gateway , gateway);
    pb_decode_bytes(&A.mask , mask);
    pb_decode_bytes(&A.dns , dns);

    if(pb_decode(&requestStream, DeviceSetDeviceNameRequest_fields, &A) == TRUE ){	
      BLE_DEBUG_LOG(BLE_DEBUG, ("\r\n"));
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



//config.write(CFG_SYS_DEVICE_NAME ,name,0);//前4个hex替换 暂时不处理 名字乱码


      uint8_t *dc;
      config.read(CFG_MQTT_DC , (void **)&dc);
      log(DEBUG,"新下发设备编码 = %s  原始设备编码 = %s\n" , (char*)code,(char*)dc);
      if(aiot_strcmp(dc,code,21))
      {
        log(DEBUG,"编码一致 啥也不做 \n");;
      } else{
      log(DEBUG,"编码有变 清空本地黑白名单 \n");
        set_clear_flash(FLASH_PERMI_LIST_BIT);
        config.write(CFG_MQTT_DC ,code,1);//21个char
      }


      /*如果再次安装，devicecode和上次不一样，要把黑白名单和通行组都删掉，如果devicecode一样，就不需要删了*/
      char DeafultPwd[BLE_PASSWORD_LENGTH*2] ={0};


      sprintf(DeafultPwd,"%d",A.pairPWD);
      memcpy_down(DeafultPwd,DeafultPwd,strlen(DeafultPwd));
      config.write(CFG_PAIR_PWD ,DeafultPwd ,0);
      log_arry(DEBUG,"配对密码 "  ,(uint8_t *)DeafultPwd,BLE_PASSWORD_LENGTH);


      sprintf(DeafultPwd,"%d",A.openPWD);
      memcpy_down(DeafultPwd,DeafultPwd,strlen(DeafultPwd));
      config.write(CFG_USER_PWD ,DeafultPwd ,0);
      log_arry(DEBUG,"开门密码 "  ,(uint8_t *)DeafultPwd,BLE_PASSWORD_LENGTH);


      config.write(CFG_SYS_OPEN_TIME ,&A.openDelay,0);
      config.write(CFG_SYS_ALARM_TIME ,&A.alarmDelay,0);
      config.write(CFG_SYS_LOCK_MODE ,&A.installPurpose,1);

      serverAddrType *ip_port = (serverAddrType *)ip_port_handle((char*)mqttServer);
      config.write(CFG_NET_ADDR ,ip_port,0);
      ShowIp(ip_port);
      up_return_comm(pag,0);
      return APP_OK;
  }
  
  return APP_ERR;
}


uint8_t BleDataHandleDetails(BleProtData *pag)
{ 
    uint8_t idx =0 ;
    show_BleProtData(pag);
    for(idx = 0; idx < sizeof (gAppTaskArry) / sizeof (gAppTaskArry[0]); idx++)
    {
        if( pag->cmd == gAppTaskArry[idx].cmd)
        {
            log(INFO,"Bt receive command cmd = 0X%02X\r\n" ,pag->cmd );
            return (gAppTaskArry[idx].EventHandlerFn(pag));               
        }
    }

    log(DEBUG,"This command TYP has no register. cmd = 0X%02X\r\n" ,pag->cmd );
    return APP_ERR;
}

void BleDataHandle(BleProtData *pag)
{
    if( BleDataHandleDetails(pag) != APP_OK)
    {      
        beep.write(BEEP_ALARM);   
        return ;
    }
     beep.write(BEEP_NORMAL);  
     return;
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

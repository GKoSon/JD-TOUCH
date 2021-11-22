#include "tsl_mqtt.h"
#include "crc32.h"
#include "string.h"
#include "unit.h"
#include "mqtt_client.h"
#include <stdlib.h>
#include <string.h>
#include "ctr_drbg.h"
#include "timer.h"
#include "mqtt_task.h"
#include "mqtt_client.h"
#include "config.h"
#include "beep.h"
#include "sysCfg.h"
#include "cJSON.h"
#include "permi_list.h"
#include "open_door.h"
#include "open_log.h"

xTaskHandle tslMqttTask;
uint8_t  sendGetTimeTimerHandle = 0xFF;
__IO char    RXOKLOCK=0;
int tsl_mqtt_recv_message(mqttClientType* c , mqttRecvMsgType *p) ;
static char taskID[33]={0};
enum
{
  MQTIME,
  MQCTL,
  MQBWLIST,
  MQGUP,
  MQOTA,
};


#define RT_DEBUG_LOG(type, message)                                           \
do                                                                            \
{                                                                             \
    if (type)                                                                 \
        printf message;                                                   \
}                                                                             \
while (0)


#define RT_DEBUG_IPC                                                      1



char *getBleMac() {
  
    uint8_t *mac;

    config.read(CFG_MQTT_MAC , (void **)&mac);
        
    return (char *)mac;
}

char *getdeviceCode() {
  
    uint8_t *dc;

    config.read(CFG_MQTT_DC , (void **)&dc);
        
    return (char *)dc;
}

char *getDeviceId() {
    uint8_t *mac;

    config.read(CFG_MQTT_MAC , (void **)&mac);
        
    return (char *)mac;
}

static void mqtt_recv_task( void const *pvParameters) 
{
    mqttRecvMsgType msg;
    
    mqttClientType    *client = NULL;

    configASSERT(pvParameters);
    
    client = (mqttClientType *)pvParameters;

    memset(&msg , 0x00 , sizeof(mqttRecvMsgType));

    while( 1 ) 
        {
          
          if(xQueueReceive(xMqttRecvQueue, &msg, 1000) == pdTRUE) 
          {
              MutexLock(&client->mutex);
      
              tsl_mqtt_recv_message(client, &msg);
              
              RXOKLOCK=0;
              
              free(msg.payload);
              
              memset(&msg, 0x00, sizeof(mqttRecvMsgType));
              
              MutexUnlock(&client->mutex);
          }
                
    }
}

void creat_tsl_mqtt_task( mqttClientType* c) {
    osThreadDef( mqtt_recv, mqtt_recv_task , osPriorityNormal, 0, configMINIMAL_STACK_SIZE*18);
    tslMqttTask = osThreadCreate(osThread(mqtt_recv), (void *)c);
    configASSERT(tslMqttTask);
}




/////////////////////////上行//////////////////////////


/*
openType 开门方式：0-刷卡，1-蓝牙开门，2-人脸开门，3-密钥开门，4-呼叫开门，5-门内开门,6-远程开门
openTime 开门时间
lockStatus 锁的状态：0-开，1-关闭
openResult 0-成功，1-无效二维码 , 2-无效用户
*/

static char *cj_create_uploadAccessLog_card(long openTime,char lockStatus,char openResult,    char *cardNo,int cardType,int cardIssueType) 
{
    cJSON *root = NULL;//基础类型

    char *outStr;

    root =  cJSON_CreateObject();
    cJSON_AddStringToObject(root,"deviceNo", getDeviceId());
    cJSON_AddNumberToObject(root,"openType", 0);
    cJSON_AddNumberToObject(root,"openTime",(double) openTime*1000);
    cJSON_AddNumberToObject(root,"lockStatus", lockStatus);
    cJSON_AddNumberToObject(root,"openResult", openResult);


    cJSON_AddStringToObject(root,"cardNo", cardNo);
    cJSON_AddNumberToObject(root,"cardType", cardType);
    cJSON_AddNumberToObject(root,"cardIssueType", cardIssueType);


    outStr = cJSON_Print(root);
    cJSON_Delete(root);
        
        if(strlen(outStr) < 2)
      soft_system_resert(__FUNCTION__);
      
    printf("\r\n\r\n【%d】[%x][%x][%x]\r\n\r\n", strlen(outStr),outStr[0],outStr[1],outStr[2]);
    if(strlen(outStr) < 10)
      soft_system_resert(__FUNCTION__);
    return outStr;
}


static char *cj_create_uploadAccessLog_pwd(long openTime,   int passwordType) 
{return NULL;}


static char *cj_create_uploadAccessLog(long openTime,int openType,int Result) 
{  return NULL;}



static char *cj_create_uploadAccessSensor(int sensorStatus) 
{
    cJSON *root = NULL;
    char *outStr;

    root =  cJSON_CreateObject();
    cJSON_AddStringToObject(root,"deviceNo", getDeviceId());
    cJSON_AddNumberToObject(root,"logTime",  rtc.read_stamp());
    cJSON_AddNumberToObject(root,"sensorStatus", sensorStatus);
    

    outStr = cJSON_Print(root);
    cJSON_Delete(root);
        
    printf("\r\n\r\n【%d】[%x][%x][%x]\r\n\r\n", strlen(outStr),outStr[0],outStr[1],outStr[2]);
    if(strlen(outStr) < 10)
      soft_system_resert(__FUNCTION__);
    return outStr;
}

static char *cj_create_uploadDeviceVer(void)
{
    cJSON *root = NULL;
    char *outStr;
    char versionData[5];//---------------------不能是4
    uint16_t swVer = config.read(CFG_SYS_SW_VERSION , NULL); 
    memset(versionData , 0x00 , sizeof(versionData));
    sprintf(versionData ,"v%03d" , swVer);
    
    root =  cJSON_CreateObject();
    cJSON_AddStringToObject(root,"deviceNo", getDeviceId());
    cJSON_AddStringToObject(root,"version", versionData);

    outStr = cJSON_Print(root);
    cJSON_Delete(root);

    if(strlen(outStr) < 10)
      soft_system_resert(__FUNCTION__);
    printf("\r\n\r\n【%s】【%d】\r\n\r\n", outStr,strlen(outStr));
    return outStr;
}

static char *cj_create_uploadDeviceInfo(void) 
{
    cJSON *root = NULL;
    char *outStr;
    uint8_t deviceLockMode = config.read(CFG_SYS_LOCK_MODE , NULL);
    char versionData[4];
    uint16_t swVer = config.read(CFG_SYS_SW_VERSION , NULL); 
    memset(versionData , 0x00 , sizeof(versionData));
    sprintf(versionData ,"v%03d" , swVer);
    
    root =  cJSON_CreateObject();
    cJSON_AddStringToObject(root,"deviceCode", getdeviceCode());
    cJSON_AddNumberToObject(root,"type", deviceLockMode);
    cJSON_AddStringToObject(root,"apkVersion", versionData);
    cJSON_AddStringToObject(root,"bluetoothMac", getBleMac());

    outStr = cJSON_Print(root);
    cJSON_Delete(root);

    if(strlen(outStr) < 10)
      soft_system_resert(__FUNCTION__);
    printf("\r\n\r\n【%s】【%d】\r\n\r\n", outStr,strlen(outStr));
    return outStr;
}


//timeStamp==1标识初始化
static char *cj_create_filterRequest(void) 
{
    cJSON *root = NULL;
    char *outStr;
    double timeStamp = 0;

    if(config.read(MQTT_FILTER_SYNCED , NULL))
    {
      log(INFO,"[MQTT-BUS]日常利用时间戳拉名单\n");
        timeStamp =(double)rtc.read_stamp()*1000;
    } else{
        log(INFO,"[MQTT-BUS]历史第一次安装强拉名单\n");
        timeStamp = 1;
        uint8_t rc =1;
        config.write(MQTT_FILTER_SYNCED , &rc,TRUE);/*写为1 从此关闭flag*/
    }
         
    root =  cJSON_CreateObject();

    cJSON_AddStringToObject(root,"deviceCode", getdeviceCode());
    cJSON_AddNumberToObject(root,"timeStamp", timeStamp);

    outStr = cJSON_Print(root);
    cJSON_Delete(root);

    if(strlen(outStr) < 10)
      soft_system_resert(__FUNCTION__);
    printf("\r\n\r\n【%s】【%d】\r\n\r\n", outStr,strlen(outStr));
    return outStr;
}



static char *cj_create_keepAlive(int status) 
{
    cJSON *root = NULL;
    char *outStr;

    root =  cJSON_CreateObject();
    SHOWME
    if(NULL == cJSON_AddStringToObject(root,"deviceCode", getdeviceCode())) {SHOWME;return NULL;};
    if(NULL == cJSON_AddNumberToObject(root,"status", status) ){SHOWME;return NULL;};
    if(NULL == cJSON_AddNumberToObject(root,"timeStamp", (double)rtc.read_stamp()*1000)) {SHOWME;return NULL;};
    

    outStr = cJSON_Print(root);
    cJSON_Delete(root);

    if(strlen(outStr) < 10)
      soft_system_resert(__FUNCTION__);
    printf("\r\n\r\n【%s】【%d】\r\n\r\n", outStr,strlen(outStr));
    return outStr;
}



void cj_response(char * taskID,int statusCode) //1--失败 0--成功
{
    cJSON *root = NULL;

    char *outStr;

    char topicPath[50];    memset(topicPath,0,50);
      
    root =  cJSON_CreateObject();

    cJSON_AddStringToObject(root,"taskID", taskID);
    cJSON_AddNumberToObject(root,"statusCode", statusCode);

    outStr = cJSON_Print(root);
    cJSON_Delete(root);


    sprintf(topicPath,"%s%s","/star_line/client/ack/",getdeviceCode());
    
    
    mqtt_send_publish(&client, (uint8_t *)topicPath, (uint8_t *)outStr, strlen(outStr), QOS1, 0);
    
    log(DEBUG,"topicPath【%s】[%s]\n",topicPath,outStr);
    return ;
}
             
void upuploadDevicever(void) 
{
    SHOWME
    char topicPath[60];    memset(topicPath,0,60);   
      
    char *send = cj_create_uploadDeviceVer();

    sprintf(topicPath,"%s%s","/client/uploadReaderProgramVersion/",getdeviceCode());
    
    mqtt_send_publish(&client, (uint8_t *)topicPath,  (uint8_t *)send, strlen(send), QOS1, 0);
    
    log(DEBUG,"topicPath【%s】[%s]\n",topicPath,send);
}

void upuploadDeviceInfo(void) 
{
    SHOWME
    char topicPath[50];    memset(topicPath,0,50);    
         
    char *send = cj_create_uploadDeviceInfo();

    sprintf(topicPath,"%s%s","/star_line/client/uploadDeviceInfo/",getdeviceCode());
    
    mqtt_send_publish(&client, (uint8_t *)topicPath,  (uint8_t *)send, strlen(send), QOS1, 0);
    
    log(DEBUG,"topicPath【%s】[%s]\n",topicPath,send);
}

void upuploadAccessLog_card(long openTime,char lockStatus,char openResult,    char *cardNo,int cardType,int cardIssueType) 
{
    SHOWME
    char topicPath[50];    memset(topicPath,0,50); 
        
     char *send = cj_create_uploadAccessLog_card(openTime,lockStatus, openResult,cardNo, cardType,cardIssueType);

    //sprintf(topicPath,"%s%s","/client/uploadAccessLog/",getBleMac());
    memcpy(topicPath,"/client/uploadAccessLog/",strlen("/client/uploadAccessLog/"));
    //strcat(topicPath,getBleMac());
   
    log(DEBUG,"topicPath【%s】[%s]\n",topicPath,send);
    mqtt_send_publish(&client,  (uint8_t *)topicPath,  (uint8_t *)send, strlen(send), QOS1, 0);
        
    journal.send_queue(LOG_DEL , 0);
    
    log(DEBUG,"topicPath【%s】[%s]\n",topicPath,send);
}



void upuploadAccessLog_pwd(long openTime,  int passwordType) 
{       
    SHOWME
    char topicPath[50];    memset(topicPath,0,50); 
    char *send = cj_create_uploadAccessLog_pwd( openTime,   passwordType) ;
    sprintf(topicPath,"%s%s","/client/uploadAccessLog/",getdeviceCode());
    printf("topicPath:%s\r\n",topicPath);
    mqtt_send_publish(&client, (uint8_t *)topicPath, (uint8_t *)send, strlen(send), QOS1, 0);
    journal.send_queue(LOG_DEL , 0);
}

void upuploadAccessLog_indoor(long openTime) 
{       
    SHOWME
    char topicPath[50];    memset(topicPath,0,50); 
    char *send = cj_create_uploadAccessLog(openTime,5,0);
    sprintf(topicPath,"%s%s","/client/uploadAccessLog/",getdeviceCode());   
    printf("topicPath:%s\r\n",topicPath);
    mqtt_send_publish(&client, (uint8_t *)topicPath,  (uint8_t *)send, strlen(send), QOS1, 0);   
    journal.send_queue(LOG_DEL , 0);
}

     

void upuploadAccessSensor(long logTime ,int sensorStatus) 
{
    SHOWME
    char topicPath[50];    memset(topicPath,0,50); 

    char *send = cj_create_uploadAccessSensor(sensorStatus) ;

    sprintf(topicPath,"%s%s","/client/uploadAccessSensor/",getdeviceCode());
    
    
    mqtt_send_publish_form_isr(&client,  (uint8_t *)topicPath,  (uint8_t *)send, strlen(send), QOS1, 0);
    
    log(DEBUG,"topicPath【%s】[%s]\n",topicPath,send);

}
                                                             

void upfilterRequest(void) 
{
    SHOWME
    char *send =NULL;
    char topicPath[50];    memset(topicPath,0,50); 
    
    send = cj_create_filterRequest() ;

    sprintf(topicPath,"%s%s","/star_line/client/filterSync/",getdeviceCode());
    
    mqtt_send_publish(&client,  (uint8_t *)topicPath,  (uint8_t *)send, strlen(send), QOS1, 0);
    
    log(INFO,"topicPath【%s】[%s]\n",topicPath,send);
}

void upkeepAlive(char isr) 
{
    SHOWME
    char topicPath[50];    memset(topicPath,0,50); 
    char *send = cj_create_keepAlive(0);//-------------1 故障     0正常在线  可以做全局标识！！！！！
    
    sprintf(topicPath,"%s%s","/star_line/client/keepAlive/",getdeviceCode());
    
    
    log(INFO,"topicPath【%s】[%s]--[%d 1--中断0--函数]\n",topicPath,send,isr);
    
    if(isr)
      mqtt_send_publish_form_isr(&client,  (uint8_t *)topicPath,  (uint8_t *)send, strlen(send), QOS1, 0);
    else
      mqtt_send_publish(&client,  (uint8_t *)topicPath,  (uint8_t *)send, strlen(send), QOS1, 0);
}




//////////////////////////////下行////////////////////////
typedef struct _cj_dispatchFilterItem
{
    char cardNo[17] ;
    long endTime;
    char filterType;
    long timeStamp;
} cj_dispatchFilterItem;

void showdispatchFilterItem(cj_dispatchFilterItem *p)
{
   
    log(ERR,"p->taskID      %s\r\n",taskID);
    log(ERR,"p->cardNo     %s\r\n",p->cardNo);
    log(ERR,"p->endTime    %d\r\n",p->endTime);
    log(ERR,"p->filterType %d\r\n",p->filterType);
    log(ERR,"p->timeStamp  %d\r\n",p->timeStamp);

}
int cj_parse_dispatchFilterItem(const char * pJson,cj_dispatchFilterItem *item)
{

      if(NULL == pJson) return 1;

      cJSON * pRoot = cJSON_Parse(pJson);
      if(NULL == pRoot) 
      {
          const char *error_ptr = cJSON_GetErrorPtr();
          if (error_ptr != NULL) 
          {
            log(ERR,"Error before: %s\n", error_ptr);
          }

          cJSON_Delete(pRoot);

          return 2;
      }
      
      memset(taskID,0,33);
      cJSON * pSubONE = cJSON_GetObjectItem(pRoot, "seqNo");
      if(NULL == pSubONE)
      {
          cJSON_Delete(pRoot);
          return 3;
      }
      sprintf(taskID,"%.32s",pSubONE->valuestring);
      
      
       cJSON * pSubALL = cJSON_GetObjectItem(pRoot, "data");
      if(NULL == pSubALL)
      {
          cJSON_Delete(pRoot);
          return 3;
      }   

      
      cJSON * pSub = cJSON_GetObjectItem(pSubALL, "cardNo");
      if(NULL == pSub)
      {
          cJSON_Delete(pRoot);
          return 3;
      }

      sprintf(item->cardNo,"%.16s",pSub->valuestring);

      pSub = cJSON_GetObjectItem(pSubALL, "timeStamp");
      if(NULL == pSub)
      {
          cJSON_Delete(pRoot);
          return 4;
      }

      item->timeStamp = pSub->valueint;
      
      
      pSub = cJSON_GetObjectItem(pSubALL, "endTime");
      if(NULL == pSub)
      {
          cJSON_Delete(pRoot);
          return 4;
      }   
      item->endTime = pSub->valueint;

      
      
      
      pSub = cJSON_GetObjectItem(pSubALL, "filterType");
      if(NULL == pSub)
      {
          cJSON_Delete(pRoot);
          return 5;
      }
      
      item->filterType = pSub->valueint;


      cJSON_Delete(pRoot);

      //showdispatchFilterItem(item);
      
      return 0;
}
char downProgramURL(char *pJson)
{
      SHOWME 
      char code = 1;/*1--失败*/
      int port = 0,ver=0;
      char *p = NULL;
      char url[64];
      cJSON * pRoot = cJSON_Parse(pJson);
      cJSON * pSub  = NULL;
      if(NULL == pRoot) 
      {
          const char *error_ptr = cJSON_GetErrorPtr();
          if (error_ptr != NULL)    log(ERR,"Error before: %s\n", error_ptr);
          cJSON_Delete(pRoot);  
          return code;
      }
      

      cJSON * pSubONE = cJSON_GetObjectItem(pRoot, "seqNo");
      if(NULL == pSubONE)
      {
          cJSON_Delete(pRoot);
          return code;
      }
      memset(taskID,0,33);
      sprintf(taskID,"%.32s",pSubONE->valuestring);
      printf("NO--%s\r\n",pSubONE->valuestring);
      
      cJSON * pSubALL = cJSON_GetObjectItem(pRoot, "data");
      if(NULL == pSubALL)
      {
          cJSON_Delete(pRoot);
          goto out;
      }   
      
      pSub = cJSON_GetObjectItem(pSubALL, "url");
      if(NULL == pSub)
      {
          cJSON_Delete(pRoot);
          goto out;
      }
      code = 0;
      p = strstr(pSub->valuestring,":");
      p++;
      p = strstr(p,":");
      if(p)
      {
        memset(url,0,64);
        url[0]='/';
        p++;

        for(char i=0;i<strlen(p);i++)
        if(p[i]=='/')
        {
          p[i]=' ';break;
        }

          sscanf(p, "%d%s",  &port, &url[1]);
        printf("[port-d-%d][url--%s]\r\n",port,url);
        
        config.write(CFG_OTA_URL ,url ,0);
        
        config.write( CFG_OTA_PORT , &port ,0);
      }
      
      pSub = cJSON_GetObjectItem(pSubALL, "version");
      if(NULL == pSub)
      {
          cJSON_Delete(pRoot);
          goto out;
      }

      printf("【%s】",pSub->valuestring);
      if(strstr(pSub->valuestring,"V")||strstr(pSub->valuestring,"v"))
      p = pSub->valuestring + 1;
      else
      p = pSub->valuestring;

      ver = atoi(p);
      printf("ver【%d】",ver);
      
      otaType otaCfg;
      otaCfg.ver = ver;
      config.write(CFG_OTA_CONFIG , &otaCfg,1); 

      
out:
     cj_response(taskID ,code ); 
     return code;
}

void downdispatchFilterItem(char *p)
{
        char code = 0;
        static int cnt=0;
        permiListType list;
        uint8_t userListResault = FALSE;
        cj_dispatchFilterItem item;
        
        memset(&item,0,sizeof(cj_dispatchFilterItem));
        
        cj_parse_dispatchFilterItem(p,&item);

        log(DEBUG,"接到黑白名单命令[cnt=%d],item.cardNo=%s , item.endTime=%ld , item.filterType=%d(0:增加黑名单，2：增加白名单，1&3:删除名单)\n" ,++cnt,item.cardNo , item.endTime ,item.filterType  );
        list.ID = atol64((char*)item.cardNo);
        list.time = item.endTime;

                
        switch( item.filterType )
        {
            case 0:
            {
                list.status = LIST_BLACK;
                userListResault = permi.add(&list);
            }break;
            case 2:
            {
                list.status = LIST_WRITE;
                userListResault = permi.add(&list);
            }break;

            case 1:
            case 3:
            {
                userListResault = permi.del(list.ID);
            }break;
            default: code=1;log_err("黑白名单没有这个操作 \n" );
        }
        
        if(userListResault == 1)
          RT_DEBUG_LOG(RT_DEBUG_IPC, ("黑白名单permi操作成功\n"));      
        else
          code=1;
   
       cj_response(taskID ,code ); 
}





typedef struct _cj_uploadDeviceInfoRequest
{
    char deviceNo[12] ;
    int  apkVersion;
    char bluetoothMac[12] ;
    char ipAddress[16];
} cj_uploadDeviceInfoRequest;

void showuploadDeviceInfoRequest(cj_uploadDeviceInfoRequest *p)
{
    printf("p->deviceNo     %s\r\n",p->deviceNo);
    printf("p->apkVersion   %d\r\n",p->apkVersion);
    printf("p->bluetoothMac %s\r\n",p->bluetoothMac);
    printf("p->ipAddress    %s\r\n",p->ipAddress);
}

int cj_parse_uploadDeviceInfoRequest(const char * pJson,cj_uploadDeviceInfoRequest *item)
{

    char cnt=0;

    if(NULL == pJson) return 1;

    cJSON * pRoot = cJSON_Parse(pJson);
    if(NULL == pRoot) 
    {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) 
        {
          log(ERR,"Error before: %s\n", error_ptr);
        }
        cJSON_Delete(pRoot);

        return 2;
    }


    cJSON * pSub = cJSON_GetObjectItem(pRoot, "deviceNo");
    if(NULL != pSub)
    {
      cnt++;
      memcpy(item->deviceNo,pSub->valuestring,strlen(pSub->valuestring));
    }



    pSub = cJSON_GetObjectItem(pRoot, "apkVersion");
    if(NULL != pSub)
    {
      cnt++;    
      item->apkVersion = pSub->valueint;
    }


    pSub = cJSON_GetObjectItem(pRoot, "bluetoothMac");
    if(NULL != pSub)
    {
      cnt++;    
      memcpy(item->bluetoothMac,pSub->valuestring,strlen(pSub->valuestring));
    }

    pSub = cJSON_GetObjectItem(pRoot, "ipAddress");
    if(NULL != pSub)
    {
      cnt++;    
      memcpy(item->ipAddress,pSub->valuestring,strlen(pSub->valuestring));
    }        

    cJSON_Delete(pRoot);

    showuploadDeviceInfoRequest(item);

    if(cnt==4)
    return 0;
    else
    return 3;
}








































//////////////////////////////

/* {"taskID":"deb211024a4411ec87a60242ac170003","data":{"timeStamp":1637442019608}} */
/*
topic /star_line/server/timeCalibration
type TimeCalibration struct {
	TaskID			string 			`json:"taskID"`				//	会话ID
	Data 	struct{
		TimeStamp 		int64 			`json:"timeStamp"`			// 时间戳
	} `json:"data"`

}
*/
static char downtimeCalibration(char *pJson) 
{    
  SHOWME
    
  uint32_t stamp = 0;

  if(NULL == pJson) return 1;

  cJSON * pRoot = cJSON_Parse(pJson);

  if(NULL == pRoot) { cJSON_Delete(pRoot);  SHOWME  return 2;  }

  cJSON * pSub = cJSON_GetObjectItem(pRoot, "data");

  if(NULL == pSub)  {  cJSON_Delete(pRoot);  SHOWME return 2; }

  cJSON * pSub_2 = cJSON_GetObjectItem(pSub, "timeStamp");

  if(NULL == pSub_2) {  cJSON_Delete(pRoot); SHOWME return 2; }


  log(INFO,"[MQTT-TSL]获得服务器同步时间= %lf\n" ,  pSub_2->valuedouble);//1564725927916.000038 其实/1000 是1564725927

  
  stamp = (uint32_t)(pSub_2->valuedouble/1000);   //https://tool.lu/timestamp/ 输入毫秒准话为北京时间
        
  
  if( !Gequal( stamp, rtc.read_stamp(), 5))
  {
        log(DEBUG,"[MQTT-TSL]需要修改时间,服务器返回时间戳 = %d , 设备当前时间戳 = %d\n" , stamp , rtc.read_stamp());
        rtc.set_time_form_stamp(stamp);
  } 
  log(DEBUG,"[MQTT-TSL]不需要修改时间,服务器返回时间戳 = %d , 设备当前时间戳 = %d\n" , stamp , rtc.read_stamp());
  cJSON_Delete(pRoot);
  return 0;
        
}

/*
topic /star_line/server/deviceControl/(设备code)
type DeviceControlSync struct {
	DeviceCode 		string 			`json:"deviceCode"`			//	设备编号
	TaskID			string 			`json:"taskID"`				//	会话ID
	Data 	struct{
		PhoneNo 	string 			`json:"phoneNo"`			//	手机号
		Type 		int 			`json:"type"`				//	控制命令类型, 1-表示开，2-表示关, 3-常开
	} `json:"data"`

*/
static char downdeviceControl(char *pJson) 
{    
  SHOWME
    
  if(NULL == pJson) return 1;
  

  
  
  cJSON * pRoot = cJSON_Parse(pJson);

  if(NULL == pRoot) { cJSON_Delete(pRoot);  SHOWME  return 2;  }
  
  
  

  cJSON * pSubONE = cJSON_GetObjectItem(pRoot, "taskID");
  if(NULL == pSubONE) { cJSON_Delete(pRoot); return 3; }
  memset(taskID,0,33);
  sprintf(taskID,"%.32s",pSubONE->valuestring);

  
  

  cJSON * pSub = cJSON_GetObjectItem(pRoot, "data");
  if(NULL == pSub)  {  cJSON_Delete(pRoot);  SHOWME return 2; }

  
  
  
  
  cJSON * pSub_2 = cJSON_GetObjectItem(pSub, "type");
  if(NULL == pSub_2) {  cJSON_Delete(pRoot); SHOWME return 2; }

  
  
  
  log(INFO,"[MQTT-TSL]获得服务器控制命令= %d(1-表示开，2-表示关, 3-常开)\n" ,  pSub_2->valueint); 
  if(pSub_2->valueint==2)
  {
        open_door();
        beep.write(BEEP_NORMAL);
        log(ERR,"[MQTT-TSL]远程开门成功\n");
  }
  
  
  cj_response(taskID ,0); 
  cJSON_Delete(pRoot);
  return 0;
}

void downuploadDeviceInfoRequest(char *p)
{
    cj_uploadDeviceInfoRequest item;
    memset(&item,0,sizeof(cj_uploadDeviceInfoRequest));
    cj_parse_uploadDeviceInfoRequest(p,&item);
    upuploadDeviceInfo();

}


int tsl_mqtt_recv_message(mqttClientType* c , mqttRecvMsgType *p) 
{
    if(p->topicNo==44) { log(ERR,"[MQTT-TSL]该主题没有预定\n",); return MQTT_RECV_SUCCESS; }

    printf("[MQTT-TSL]收到主题 ￥￥￥￥￥￥￥￥￥￥￥￥￥%d(enum{MQTIME,MQCTL,MQBWLIST,MQGUP,MQOTA,})\r\n",p->topicNo);
    
    switch(p->topicNo)
    {
       case MQTIME:
         downtimeCalibration((char *)p->payload);
         break;
         case MQCTL:
         downdeviceControl((char *)p->payload);
         break;  
        case MQBWLIST:
          downdispatchFilterItem((char *)p->payload);
        break;
        case MQGUP:
          downuploadDeviceInfoRequest((char *)p->payload);///////////////////////////需要测试 现在是我自己杜撰的
        break;
        

        case MQOTA:
           if(downProgramURL((char *)p->payload)==0)
           {
              xSemaphoreGive(xMqttOtaSemaphore);         
           }
          break;
    }
    


    return MQTT_RECV_SUCCESS;
}




































/*接口设计

/v1/device/canRegister
/v1/device/register
/v1/device/update
/v1/device/delete

*/

const char httpurl[4][12]={\
  "register",\
  "update",\
  "delete",\
  "canRegister",\
};


void HTTP_POSTHEAD(char *out,const char *url,uint8_t *strip,uint16_t intport,uint16_t len)
{
sprintf(out,"POST /v1/device/%s HTTP/1.1\r\n"\
                            "Host:%s:%d\r\n"\
                            "Content-Length:%d\r\n\r\n",url,(char *)strip,intport,len);

}



//【1】一共5个int 其他str
uint16_t cj_create_httpreg0update1(char id,char *out) 
{
      cJSON *root = NULL;//基础类型
      cJSON *son  = NULL;//嵌套再里面的
      char *outStr;uint16_t bodylen=0;

      char *pd;
#if 0
      config.read(CFG_PRO_PWD , (void **)&pd);
      root =  cJSON_CreateObject();
      cJSON_AddStringToObject(root,"deviceName",         bh->NAME);
      cJSON_AddStringToObject(root,"deviceNo",          getDeviceId());
      cJSON_AddStringToObject(root,"bluetoothPassword",     pd);
      cJSON_AddStringToObject(root,"doorOpenPassword",        pd);
      cJSON_AddNumberToObject(root,"doorOpenDelay",         config.read(CFG_SYS_OPEN_TIME , NULL));//【1】
      cJSON_AddNumberToObject(root,"doorAlarmDelay",         config.read(CFG_SYS_ALARM_TIME , NULL));//【2】
      cJSON_AddStringToObject(root,"streetID",                 bh->streetID);
      cJSON_AddStringToObject(root,"committeeID",             bh->committeeID);
      cJSON_AddStringToObject(root,"villageID",                bh->villageID);
      cJSON_AddStringToObject(root,"buildingID",             bh->buildingID);
      cJSON_AddStringToObject(root,"type",                    "access");
      cJSON_AddStringToObject(root,"productModel",             bh->productName_productModel);//-------------------------写死
      cJSON_AddNumberToObject(root,"longitude",             bh->longitude);//【3】
      cJSON_AddNumberToObject(root,"latitude",                 bh->latitude);//【4】

      son =  cJSON_CreateObject();
      DeviceIpType    *devIp;    
      config.read(CFG_IP_INFO , (void **)&devIp);
      char temp[16];
      memset(temp,0,16);
      IP4ARRToStr(devIp->ip,temp);
      cJSON_AddStringToObject(son,"deviceIP",         temp);
      memset(temp,0,16);
      IP4ARRToStr(devIp->mark,temp) ;
      cJSON_AddStringToObject(son,"deviceMask",             temp);
      memset(temp,0,16);
      IP4ARRToStr(devIp->gateway,temp) ;
      cJSON_AddStringToObject(son,"deviceGateway",             temp);
      memset(temp,0,16);
      IP4ARRToStr(devIp->dns,temp) ;
      cJSON_AddStringToObject(son,"dnsServer",         temp);
      cJSON_AddItemToObject(root, "network", son);
#endif

/*冲突 seriesType 12围墙机  10门口机*/

    //  cJSON_AddNumberToObject(root,"seriesType", bh->type);/*【5】12围墙机(小区)      10门口机*/
      
      outStr = cJSON_Print(root);
      cJSON_Delete(root);

      bodylen = strlen(outStr);

      serverAddrType *addr;         
      uint16_t port  =  config.read(CFG_HTTP_ADDR , (void **)&addr);
      HTTP_POSTHEAD(out,httpurl[id],addr->ip,port,bodylen);

      strcat(out,outStr); 
      //RT_DEBUG_LOG(RT_DEBUG_IPC,("\r\n\r\n[bodylen %d  len %d]\r\n\r\n",  bodylen,strlen(out)));
      //RT_DEBUG_LOG(RT_DEBUG_IPC,("\r\n\r\n[%s]\r\n\r\n",  out));
      return strlen(out);
}

uint16_t HTTP_packREG(char *out)
{
    return cj_create_httpreg0update1(0,out);
}
uint16_t HTTP_packUP(char *out)
{
    return cj_create_httpreg0update1(1,out);
}

uint16_t cj_create_httpdel2canreg3(char id,char *out) 
{
      cJSON *root = NULL;

      char *outStr;uint16_t bodylen=0;

      root =  cJSON_CreateObject();
      cJSON_AddStringToObject(root,"deviceNo",         getDeviceId());

      outStr = cJSON_Print(root);
      cJSON_Delete(root);

      
      
      bodylen = strlen(outStr);
      serverAddrType *addr;         
      uint16_t port  =  config.read(CFG_HTTP_ADDR , (void **)&addr);
      HTTP_POSTHEAD(out,httpurl[id],addr->ip,port,bodylen);
      strcat(out,outStr);

      //RT_DEBUG_LOG(RT_DEBUG_IPC,("\r\n\r\n[bodylen %d  len %d]\r\n\r\n",  bodylen,strlen(out)));
      //RT_DEBUG_LOG(RT_DEBUG_IPC,("\r\n\r\nTX:[%s]\r\n\r\n",  out));
      return strlen(out);
}

uint16_t HTTP_packDEL(char *out)
{

    return cj_create_httpdel2canreg3(2,out);
}

uint16_t HTTP_packCANREG(char *out)
{

    return cj_create_httpdel2canreg3(3,out);
}


//1---我发上去的数据有问题 
char HTTP_checkbody(char *pJson)
{

  
  if(NULL == pJson) return -1;

  cJSON * pRoot = cJSON_Parse(pJson);

  if(NULL == pRoot) { cJSON_Delete(pRoot);  SHOWME  return -1;  }

  cJSON * pSub = cJSON_GetObjectItem(pRoot, "code");

  if(NULL == pSub)  {  cJSON_Delete(pRoot);  SHOWME return -1; }

  return (char)(pSub->valueint);

}

char HTTP_checkack(char *in)
{
  char *p=NULL;

  if( (p = strstr(in,"HTTP/1.1 2") )==0 )
    return -1;
     
  if( (p = strstr(in,"\r\n\r\n") )==0 )
    return -1; 
  
  p+=4;

  return HTTP_checkbody(p);

}
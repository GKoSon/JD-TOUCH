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
#include "tempwd.h"
#include "permi_list.h"
#include "open_door.h"
#include "open_log.h"

xTaskHandle tslMqttTask;
uint8_t  sendGetTimeTimerHandle = 0xFF;
__IO char    RXOKLOCK=0;
int tsl_mqtt_recv_message(mqttClientType* c , mqttRecvMsgType *p) ;


#define RT_DEBUG_LOG(type, message)                                           \
do                                                                            \
{                                                                             \
    if (type)                                                                 \
        printf message;                                                   \
}                                                                             \
while (0)


#define RT_DEBUG_IPC                  1



char *getBleMac() {
  
	uint8_t *mac;

	config.read(CFG_MQTT_MAC , (void **)&mac);
        
	return (char *)mac;
}

char *getDeviceId() {
	uint8_t *mac;

	config.read(CFG_MQTT_MAC , (void **)&mac);
        
	return (char *)mac;
}




char *getselfIP() {
  
    static char ip[16];

    DeviceIpType	*devIp;
    
    memset(ip,0,16);

    config.read(CFG_IP_INFO , (void **)&devIp);
    
    sprintf(ip,"%d.%d.%d.%d",devIp->ip[0],devIp->ip[1],devIp->ip[2],devIp->ip[3]);
    
    return ip;

}

char *getprojpwd() {
	uint8_t *pd;

	config.read(CFG_PRO_PWD , (void **)&pd);

	return (char *)pd;
}







static void mqtt_recv_task( void const *pvParameters) 
{
    mqttRecvMsgType msg;
    
	mqttClientType	*client = NULL;

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















//1测试嵌套
char * GmakeJson(void)
{
    cJSON * pRoot  = NULL;
    cJSON * pSub_1 = NULL;
    cJSON * pSub_2 = NULL;

    if((pRoot = cJSON_CreateObject()) == NULL)
    {
        return NULL;
    }
    if((pSub_1 = cJSON_CreateObject()) == NULL)
    {
        return NULL;
    }
    if((pSub_2 = cJSON_CreateObject()) == NULL)
    {
        return NULL;
    }

    cJSON_AddStringToObject(pSub_1, "bStr", "bbbbbbb");//往pSub_1 放东西
    cJSON_AddStringToObject(pSub_2, "cStr", "ccccccc");//往pSub_2 放东西

    cJSON_AddItemToObject(pSub_1, "subobject_2", pSub_2);//把pSub_2放到pSub_1里面 嵌套


    cJSON_AddStringToObject(pRoot, "aStr", "aaaaaaa");//往pRoot 放东西
    cJSON_AddItemToObject(pRoot, "subobject_1", pSub_1);//把pSub_1放到pRoot里面 嵌套
    cJSON_AddStringToObject(pRoot, "xStr", "xxxxxxx");

    char * pJson = cJSON_Print(pRoot);
    if(NULL == pJson)
    {
        cJSON_Delete(pRoot);
        return NULL;
    }
	printf("%s",pJson);
    return pJson;
}

//测试嵌套
int GparseJson(const char * pJson)
{
//入口判断
    if(NULL == pJson)
    {
        return 1;
    }
	
//直接解析	
    cJSON * pRoot = cJSON_Parse(pJson);
    if(NULL == pRoot)
    {
        return 2;
    }
	
//拿到	aStr
    cJSON * pSub_1 = cJSON_GetObjectItem(pRoot, "aStr");
    if(NULL == pSub_1)
    {
        cJSON_Delete(pRoot);
        return 3;
    }
	
    printf("get aStr : [%s]\n", pSub_1->valuestring);
	
//拿到	xStr	
    pSub_1 = cJSON_GetObjectItem(pRoot, "xStr");
    if(NULL == pSub_1)
    {
        cJSON_Delete(pRoot);
        return 4;
    }
    printf("get xStr : [%s]\n", pSub_1->valuestring);
	
	
//拿到	subobject_1	
    pSub_1 = cJSON_GetObjectItem(pRoot, "subobject_1");
    if(NULL == pSub_1)
    {
        cJSON_Delete(pRoot);
        return 5;
    }
    printf("get Sub Obj 1\n");
	
//从pSub_1拿到	bStr	
    cJSON * pSub_2 = cJSON_GetObjectItem(pSub_1, "bStr");
    if(NULL == pSub_2)
    {
        cJSON_Delete(pRoot);
        return 6;
    }
    printf("get bStr : [%s]\n", pSub_2->valuestring);
	
//从pSub_1拿到	subobject_2
    pSub_2 = cJSON_GetObjectItem(pSub_1, "subobject_2");
    if(NULL == pSub_2)
    {
        cJSON_Delete(pRoot);
        return 7;
    }
    printf("get Obj 2\n");
	
//从pSub_2拿到	cStr	
    cJSON * pStr = cJSON_GetObjectItem(pSub_2, "cStr");
    if(NULL == pStr)
    {
        cJSON_Delete(pRoot);
        return 8;
    }
    printf("get cStr : [%s]\n", pStr->valuestring);

    cJSON_Delete(pRoot);
    return 0;
}

/////////////////////////上行//////////////////////////


/*
openType 开门方式：0-刷卡，1-蓝牙开门，2-人脸开门，3-密钥开门，4-呼叫开门，5-门内开门,6-远程开门
openTime 开门时间
lockStatus 锁的状态：0-开，1-关闭
openResult 0-成功，1-无效二维码 , 2-无效用户
*/

char *cj_create_uploadAccessLog_card(long openTime,char lockStatus,char openResult,    char *cardNo,int cardType,int cardIssueType) 
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


char *cj_create_uploadAccessLog_pwd(long openTime,   int passwordType) 
{

	return NULL;
}


char *cj_create_uploadAccessLog(long openTime,int openType,int Result) 
{
  return NULL;
}



char *cj_create_uploadAccessSensor(int sensorStatus) 
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

char *cj_create_uploadDeviceVer(void)
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

    printf("\r\n\r\n【%d】[%x][%x][%x]\r\n\r\n", strlen(outStr),outStr[0],outStr[1],outStr[2]);
    if(strlen(outStr) < 10)
      soft_system_resert(__FUNCTION__);
    return outStr;
}

char *cj_create_uploadDeviceInfo(int type) 
{
    cJSON *root = NULL;
    char *outStr;
    char versionData[4];
    uint16_t swVer = config.read(CFG_SYS_SW_VERSION , NULL); 
    memset(versionData , 0x00 , sizeof(versionData));
    sprintf(versionData ,"v%03d" , swVer);
    
    root =  cJSON_CreateObject();
    cJSON_AddStringToObject(root,"deviceNo", getDeviceId());
    cJSON_AddNumberToObject(root,"type", type);
   // cJSON_AddStringToObject(root,"firmwareVersion", apkVersion);
    //cJSON_AddStringToObject(root,"hardwareVersion", "V0.0.0");
    cJSON_AddStringToObject(root,"apkVersion", versionData);
    cJSON_AddStringToObject(root,"iccid", "");

    cJSON_AddStringToObject(root,"bluetoothMac", getBleMac());
    cJSON_AddStringToObject(root,"ipAddress",    getselfIP());
    cJSON_AddStringToObject(root,"password",     getprojpwd());

    cJSON_AddStringToObject(root,"primarySipUrl", "");
    cJSON_AddStringToObject(root,"primarySipAccount", "");
    cJSON_AddStringToObject(root,"primarySipPW", "");
    cJSON_AddStringToObject(root,"spareSipUrl", "");
    cJSON_AddStringToObject(root,"spareSipAccount", "");
    cJSON_AddStringToObject(root,"spareSipPW", NULL);
 
    outStr = cJSON_Print(root);
    cJSON_Delete(root);

        printf("\r\n\r\n【%d】[%x][%x][%x]\r\n\r\n", strlen(outStr),outStr[0],outStr[1],outStr[2]);
    if(strlen(outStr) < 10)
      soft_system_resert(__FUNCTION__);
//printf("\r\n\r\n【%s】【%d】\r\n\r\n", outStr,strlen(outStr));
    return outStr;
}


char *cj_create_timeCalibration(long timeStamp) 
{
    cJSON *root = NULL;
    char *outStr;

    root =  cJSON_CreateObject();

    cJSON_AddNumberToObject(root,"timeStamp", timeStamp);

    outStr = cJSON_Print(root);
    cJSON_Delete(root);

        printf("\r\n\r\n【%d】[%x][%x][%x]\r\n\r\n", strlen(outStr),outStr[0],outStr[1],outStr[2]);
    if(strlen(outStr) < 10)
      soft_system_resert(__FUNCTION__);
//printf("\r\n\r\n【%s】\r\n\r\n", outStr);
    return outStr;
}

//timeStamp==1标识初始化
char *cj_create_filterRequest(double timeStamp) 
{
    cJSON *root = NULL;
    char *outStr;

    root =  cJSON_CreateObject();

    cJSON_AddStringToObject(root,"deviceNo", getDeviceId());
    cJSON_AddNumberToObject(root,"timeStamp", timeStamp);

    outStr = cJSON_Print(root);
    cJSON_Delete(root);

        printf("\r\n\r\n【%d】[%x][%x][%x]\r\n\r\n", strlen(outStr),outStr[0],outStr[1],outStr[2]);
    if(strlen(outStr) < 10)
      soft_system_resert(__FUNCTION__);
//printf("\r\n\r\n【%s】\r\n\r\n", outStr);
    return outStr;
}



char *cj_create_keepAlive(int status) 
{
    cJSON *root = NULL;
    char *outStr;


    root =  cJSON_CreateObject();
    SHOWME
    if(NULL == cJSON_AddStringToObject(root,"deviceNo", getDeviceId())) {SHOWME;return NULL;};
    if(NULL == cJSON_AddNumberToObject(root,"timeStamp", (double)rtc.read_stamp()*1000)) {SHOWME;return NULL;};
    if(NULL == cJSON_AddNumberToObject(root,"status", status) ){SHOWME;return NULL;};

    outStr = cJSON_Print(root);

    cJSON_Delete(root);

    printf("\r\n\r\n【%d】[%x][%x][%x]\r\n\r\n", strlen(outStr),outStr[0],outStr[1],outStr[2]);
    if(strlen(outStr) < 10)
      soft_system_resert(__FUNCTION__);
    
    //printf("\r\n\r\n【%s】\r\n\r\n", outStr);
    
    //log_arry(ERR,"RT_DEBUG_IPC "  ,outStr ,strlen(outStr));
    return outStr;
}



enum
{
  MQTIME,
  MQBW,
  MQIOR,
  MQOTAURL,
};

#include "mqtt_client.h"

extern char topicPath[GMAX_MESSAGE_HANDLERS][52];


static char CommonseqNo[33];

void cj_response(char * seqNo,int statusCode) //1--失败 0--成功
{
    cJSON *root = NULL;

    char *outStr;

    char topicPath[50];    memset(topicPath,0,50);
      
    root =  cJSON_CreateObject();

    cJSON_AddStringToObject(root,"seqNo", seqNo);
    cJSON_AddNumberToObject(root,"statusCode", statusCode);

    outStr = cJSON_Print(root);
    cJSON_Delete(root);


    sprintf(topicPath,"%s%s","/client/response/",getBleMac());
    
    
    mqtt_send_publish(&client, (uint8_t *)topicPath, (uint8_t *)outStr, strlen(outStr), QOS1, 0);
    
//    log(DEBUG,"topicPath【%s】[%s]\n",topicPath,outStr);
	return ;
}
             
void upuploadDevicever(void) 
{
    SHOWME
    char topicPath[60];    memset(topicPath,0,60);    //q00 err
      
    
    char *send = cj_create_uploadDeviceVer();

    sprintf(topicPath,"%s%s","/client/uploadReaderProgramVersion/",getBleMac());
    
    
    mqtt_send_publish(&client, (uint8_t *)topicPath,  (uint8_t *)send, strlen(send), QOS1, 0);
    
    log(DEBUG,"topicPath【%s】[%s]\n",topicPath,send);
}

void upuploadDeviceInfo(void) 
{
    SHOWME
    char topicPath[50];    memset(topicPath,0,50);    
      
    uint8_t deviceLockMode = config.read(CFG_SYS_LOCK_MODE , NULL);
    
    
    char *send = cj_create_uploadDeviceInfo(deviceLockMode);

    sprintf(topicPath,"%s%s","/client/uploadDeviceInfo/",getBleMac());
    
    
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
    strcat(topicPath,getBleMac());
   
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
    sprintf(topicPath,"%s%s","/client/uploadAccessLog/",getBleMac());
    printf("topicPath:%s\r\n",topicPath);
    mqtt_send_publish(&client, (uint8_t *)topicPath, (uint8_t *)send, strlen(send), QOS1, 0);
    journal.send_queue(LOG_DEL , 0);
}

void upuploadAccessLog_indoor(long openTime) 
{       
    SHOWME
    char topicPath[50];    memset(topicPath,0,50); 
    char *send = cj_create_uploadAccessLog(openTime,5,0);
    sprintf(topicPath,"%s%s","/client/uploadAccessLog/",getBleMac());   
    printf("topicPath:%s\r\n",topicPath);
    mqtt_send_publish(&client, (uint8_t *)topicPath,  (uint8_t *)send, strlen(send), QOS1, 0);   
    journal.send_queue(LOG_DEL , 0);
}

     

void upuploadAccessSensor(long logTime ,int sensorStatus) 
{
    SHOWME
    char topicPath[50];    memset(topicPath,0,50); 

    char *send = cj_create_uploadAccessSensor(sensorStatus) ;

    sprintf(topicPath,"%s%s","/client/uploadAccessSensor/",getBleMac());
    
    
    mqtt_send_publish_form_isr(&client,  (uint8_t *)topicPath,  (uint8_t *)send, strlen(send), QOS1, 0);
    
    log(DEBUG,"topicPath【%s】[%s]\n",topicPath,send);

}
                                                             

void upfilterRequest(void) 
{
    SHOWME
     char *send =NULL;
    char topicPath[50];    memset(topicPath,0,50); 
    
    double timeStamp = (double)rtc.read_stamp()*1000 ;
    
    if( config.read(CFG_DEV_USED , NULL))/*旧设备的日常复位*/
      send = cj_create_filterRequest( timeStamp) ; 
    else/*第一次入网的设备*/
      send = cj_create_filterRequest(1) ;

    sprintf(topicPath,"%s%s","/client/filterRequest/",getBleMac());
    
    
    mqtt_send_publish(&client,  (uint8_t *)topicPath,  (uint8_t *)send, strlen(send), QOS1, 0);
    
    log(INFO,"topicPath【%s】[%s]\n",topicPath,send);
}

void upkeepAlive(char isr) 
{
    SHOWME
    char topicPath[50];    memset(topicPath,0,50); 
 	char *send = cj_create_keepAlive(0);//-------------1 故障     0在线  可以做全局标识！！！！！
    
    sprintf(topicPath,"%s%s","/client/keepAlive/",getBleMac());
    
    
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
   
    log(ERR,"p->seqNo      %s\r\n",CommonseqNo);
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
      
      memset(CommonseqNo,0,33);
      cJSON * pSubONE = cJSON_GetObjectItem(pRoot, "seqNo");
      if(NULL == pSubONE)
      {
          cJSON_Delete(pRoot);
          return 3;
      }
      sprintf(CommonseqNo,"%.32s",pSubONE->valuestring);
      
      
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
	  memset(CommonseqNo,0,33);
      sprintf(CommonseqNo,"%.32s",pSubONE->valuestring);
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
     cj_response(CommonseqNo ,code ); 
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
   
       cj_response(CommonseqNo ,code ); 
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
// {"seqNo":"","data":{"timeStamp":1564722332040}} 
char downtimeCalibration(char *pJson) 
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

 // printf("timeStamp : [%s]\n", pSub_2->valuestring); 无法输出 约定是long

  log(INFO,"获得服务器同步时间= %lf\n" ,  pSub_2->valuedouble);//1564725927916.000038 其实/1000 是1564725927

  
  stamp = (uint32_t)(pSub_2->valuedouble/1000);   //https://tool.lu/timestamp/ 输入毫秒准话为北京时间
		
  
  if( !Gequal( stamp, rtc.read_stamp(), 5))
  {
        log(DEBUG,"[需要修改时间]服务器返回时间戳 = %d , 设备当前时间戳 = %d\n" , stamp , rtc.read_stamp());
        rtc.set_time_form_stamp(stamp);
  } 
  
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
    if(p->topicNo==44) { log(ERR,"该主题没有预定\n",); return MQTT_RECV_SUCCESS; }

//    printf("[tsl_mqtt_recv_message 该主题IS %s]\r\n",topicPath[p->topicNo]);
    
    switch(p->topicNo)
    {
       case MQTIME:
         downtimeCalibration((char *)p->payload);
         break;
        case MQBW:
          downdispatchFilterItem((char *)p->payload);
        break;
        case MQIOR:
          downuploadDeviceInfoRequest((char *)p->payload);///////////////////////////需要测试 现在是我自己杜撰的
        break;
        case MQOTAURL:
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

      config.read(CFG_PRO_PWD , (void **)&pd);
      root =  cJSON_CreateObject();
      cJSON_AddStringToObject(root,"deviceName", 		bh->NAME);
      cJSON_AddStringToObject(root,"deviceNo",  		getDeviceId());
      cJSON_AddStringToObject(root,"bluetoothPassword",     pd);
      cJSON_AddStringToObject(root,"doorOpenPassword",		pd);
      cJSON_AddNumberToObject(root,"doorOpenDelay", 		config.read(CFG_SYS_OPEN_TIME , NULL));//【1】
      cJSON_AddNumberToObject(root,"doorAlarmDelay", 		config.read(CFG_SYS_ALARM_TIME , NULL));//【2】
      cJSON_AddStringToObject(root,"streetID", 		        bh->streetID);
      cJSON_AddStringToObject(root,"committeeID", 		    bh->committeeID);
      cJSON_AddStringToObject(root,"villageID",		        bh->villageID);
      cJSON_AddStringToObject(root,"buildingID", 		    bh->buildingID);
      cJSON_AddStringToObject(root,"type",		            "access");
      cJSON_AddStringToObject(root,"productModel", 		    bh->productName_productModel);//-------------------------写死
      cJSON_AddNumberToObject(root,"longitude", 		    bh->longitude);//【3】
      cJSON_AddNumberToObject(root,"latitude", 		        bh->latitude);//【4】

      son =  cJSON_CreateObject();
      DeviceIpType	*devIp;    
      config.read(CFG_IP_INFO , (void **)&devIp);
      char temp[16];
      memset(temp,0,16);
      IP4ARRToStr(devIp->ip,temp);
      cJSON_AddStringToObject(son,"deviceIP", 		temp);
      memset(temp,0,16);
      IP4ARRToStr(devIp->mark,temp) ;
      cJSON_AddStringToObject(son,"deviceMask", 	        temp);
      memset(temp,0,16);
      IP4ARRToStr(devIp->gateway,temp) ;
      cJSON_AddStringToObject(son,"deviceGateway", 	        temp);
      memset(temp,0,16);
      IP4ARRToStr(devIp->dns,temp) ;
      cJSON_AddStringToObject(son,"dnsServer", 		temp);
      cJSON_AddItemToObject(root, "network", son);

/*冲突 seriesType 12围墙机  10门口机*/

      cJSON_AddNumberToObject(root,"seriesType", bh->type);/*【5】12围墙机(小区)      10门口机*/
      
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
      cJSON_AddStringToObject(root,"deviceNo", 		getDeviceId());

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
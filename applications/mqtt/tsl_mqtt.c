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
#define CLIENT_TOPIC_LEN          80
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

  /*  printf("\r\n\r\n��%s����%d��\r\n\r\n", outStr,strlen(outStr));\ */
#define COMMON_END_CODE   \
    outStr = cJSON_Print(root);  \
    cJSON_Delete(root);\
    if(strlen(outStr) < 10)\
      soft_system_resert(__FUNCTION__);\
    return outStr;\

        
#define MUST_TRUE(p) \
        do { \
              if(NULL == p) \
              {\
                  cJSON_Delete(pRoot);\
                  return 1;\
              }\
        }while(0); 
        
        
        
static char *getBleMac() {
    uint8_t *mac;
    config.read(CFG_MQTT_MAC , (void **)&mac);       
    return (char *)mac;
}

char *getdeviceCode() {
    uint8_t *dc;
    config.read(CFG_MQTT_DC , (void **)&dc);     
    return (char *)dc;
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




/////////////////////////����//////////////////////////


/*
openType ���ŷ�ʽ��0-ˢ����1-�������ţ�2-�������ţ�3-��Կ���ţ�4-���п��ţ�5-���ڿ���,6-Զ�̿���
openTime ����ʱ��
lockStatus ����״̬��0-����1-�ر�
openResult 0-�ɹ���1-��Ч��ά�� , 2-��Ч�û�

����DC ����
*/

static char *cj_create_uploadAccessLog_card(long openTime,char lockStatus,char openResult,    char *cardNo,int cardType,int cardIssueType) 
{
    cJSON *root = NULL;//��������

    char *outStr;

    root =  cJSON_CreateObject();

    cJSON_AddStringToObject(root,"deviceCode", getdeviceCode());
    cJSON_AddNumberToObject(root,"openType", 0);//0--��ʾ���ŷ�ʽ�� ˢ��
    cJSON_AddNumberToObject(root,"openTime",(double) openTime*1000);
    cJSON_AddNumberToObject(root,"lockStatus", lockStatus);
    cJSON_AddNumberToObject(root,"openResult", openResult);


    cJSON_AddStringToObject(root,"cardNo", cardNo);
    cJSON_AddNumberToObject(root,"cardType", cardType);
    cJSON_AddNumberToObject(root,"cardIssueType", cardIssueType);


    COMMON_END_CODE
}



static char *cj_create_uploadAccessLog_Indoor(long openTime) 
{ 
    cJSON *root = NULL;//��������

    char *outStr;

    root =  cJSON_CreateObject();

    cJSON_AddStringToObject(root,"deviceCode", getdeviceCode());
    cJSON_AddNumberToObject(root,"openType", 5);//5--��ʾ���ŷ�ʽ�����ڿ���
    cJSON_AddNumberToObject(root,"openTime",(double) openTime*1000);
    cJSON_AddNumberToObject(root,"openResult", 1);//1--��ʾ���ųɹ� ���Ǳ����


    COMMON_END_CODE
}



static char *cj_create_uploadAccessSensor(int sensorStatus) 
{
    cJSON *root = NULL;
    char *outStr;

    root =  cJSON_CreateObject();
  
    cJSON_AddNumberToObject(root,"logTime",  rtc.read_stamp());
    cJSON_AddNumberToObject(root,"sensorStatus", sensorStatus);
    

    COMMON_END_CODE
}

static char *cj_create_uploadDeviceInfo(void) 
{
    cJSON *root = NULL;
    char *outStr;
    uint8_t deviceLockMode = config.read(CFG_SYS_LOCK_MODE , NULL);
    char versionData[6]={0};
    uint32_t swVer = config.read(CFG_SYS_SW_VERSION , NULL); 
    StringVer(versionData,swVer);
    
    root =  cJSON_CreateObject();
    cJSON_AddStringToObject(root,"deviceCode", getdeviceCode());
    cJSON_AddNumberToObject(root,"type", deviceLockMode);
    cJSON_AddStringToObject(root,"version", versionData);
    cJSON_AddStringToObject(root,"bluetoothMac", getBleMac());

    COMMON_END_CODE
}


/*��ʱ����?������ʱ���*/
static char *cj_create_filterRequest(void) 
{
    cJSON *root = NULL;
    char *outStr;
    double timeStamp = 0;

    if(config.read(MQTT_FILTER_SYNCED , NULL))
    {
      log(INFO,"[MQTT-BUS]�ճ�����ʱ���������\n");
        timeStamp =(double)rtc.read_stamp()*1000;
    } else{
        log(INFO,"[MQTT-BUS]��ʷ��һ�ΰ�װǿ������\n");
        timeStamp = 1;
        uint8_t rc =1;
        config.write(MQTT_FILTER_SYNCED , &rc,TRUE);/*дΪ1 �Ӵ˹ر�flag*/
    }
         
    root =  cJSON_CreateObject();

    cJSON_AddStringToObject(root,"deviceCode", getdeviceCode());
    cJSON_AddNumberToObject(root,"timeStamp",  timeStamp);

    COMMON_END_CODE
}



static char *cj_create_keepAlive(int status) 
{
    cJSON *root = NULL;
    char *outStr;

    root =  cJSON_CreateObject();

    cJSON_AddStringToObject(root,"deviceCode", getdeviceCode());
    cJSON_AddNumberToObject(root,"status", status) ;
    cJSON_AddNumberToObject(root,"timeStamp", (double)rtc.read_stamp()*1000);
    
    COMMON_END_CODE
}


void cj_response(char * taskID,int statusCode) //1--ʧ�� 0--�ɹ�
{
    cJSON *root = NULL;

    char *outStr;

    char clientTopic[CLIENT_TOPIC_LEN];    memset(clientTopic,0,CLIENT_TOPIC_LEN);
      
    root =  cJSON_CreateObject();

    cJSON_AddStringToObject(root,"taskID", taskID);
    cJSON_AddNumberToObject(root,"statusCode", statusCode);

    outStr = cJSON_Print(root);
    cJSON_Delete(root);


    sprintf(clientTopic,"%s%s","/star_line/client/ack/",getdeviceCode());
    
    mqtt_send_publish(&client, (uint8_t *)clientTopic, (uint8_t *)outStr, strlen(outStr), QOS1, 0);
    
    log(DEBUG,"clientTopic ��%s��[%s]\n",clientTopic,outStr);
    return ;
}
             
void uploadDeviceInfo(void) 
{
    SHOWME
    char clientTopic[CLIENT_TOPIC_LEN];    memset(clientTopic,0,CLIENT_TOPIC_LEN);   
         
    char *send = cj_create_uploadDeviceInfo();

    sprintf(clientTopic,"%s%s","/star_line/client/uploadDeviceInfo/",getdeviceCode());
    
    mqtt_send_publish(&client, (uint8_t *)clientTopic,  (uint8_t *)send, strlen(send), QOS1, 0);
    
    log(DEBUG,"clientTopic ��%s��[%s]\n",clientTopic,send);
}

void uploadAccessLog_card(long openTime,char lockStatus,char openResult,    char *cardNo,int cardType,int cardIssueType) 
{
    SHOWME
    char clientTopic[CLIENT_TOPIC_LEN];    memset(clientTopic,0,CLIENT_TOPIC_LEN);
        
     char *send = cj_create_uploadAccessLog_card(openTime,lockStatus, openResult,cardNo, cardType,cardIssueType);

    sprintf(clientTopic,"%s%s","/client/uploadAccessLog/",getdeviceCode());
   
    mqtt_send_publish(&client,  (uint8_t *)clientTopic,  (uint8_t *)send, strlen(send), QOS1, 0);
        
    journal.send_queue(LOG_DEL , 0);
    
    log(DEBUG,"clientTopic ��%s��[%s]\n",clientTopic,send);
}


void uploadAccessLog_indoor(long openTime) 
{       
    SHOWME
    char clientTopic[CLIENT_TOPIC_LEN];    memset(clientTopic,0,CLIENT_TOPIC_LEN);
    char *send = cj_create_uploadAccessLog_Indoor(openTime);
    sprintf(clientTopic,"%s%s","/client/uploadAccessLog/",getdeviceCode());      
    mqtt_send_publish(&client, (uint8_t *)clientTopic,  (uint8_t *)send, strlen(send), QOS1, 0);   
    journal.send_queue(LOG_DEL , 0);
    log(DEBUG,"clientTopic ��%s��[%s]\n",clientTopic,send);
}

     

void uploadAccessSensor(int sensorStatus) 
{
    SHOWME
    char clientTopic[CLIENT_TOPIC_LEN];    memset(clientTopic,0,CLIENT_TOPIC_LEN);

    char *send = cj_create_uploadAccessSensor(sensorStatus) ;

    sprintf(clientTopic,"%s%s","/client/uploadAccessSensor/",getdeviceCode());
    
    mqtt_send_publish_form_isr(&client,  (uint8_t *)clientTopic,  (uint8_t *)send, strlen(send), QOS1, 0);
    
    log(DEBUG,"clientTopic ��%s��[%s]\n",clientTopic,send);

}
                                                             

void upfilterRequest(void) 
{
    SHOWME
    char *send =NULL;
    char clientTopic[CLIENT_TOPIC_LEN];    memset(clientTopic,0,CLIENT_TOPIC_LEN);
    
    send = cj_create_filterRequest() ;

    sprintf(clientTopic,"%s%s","/star_line/client/filterSync/",getdeviceCode());
    
    mqtt_send_publish(&client,  (uint8_t *)clientTopic,  (uint8_t *)send, strlen(send), QOS1, 0);
    
    log(DEBUG,"clientTopic ��%s��[%s]\n",clientTopic,send);
}


void upkeepAlive(char isr) 
{
    
    char clientTopic[CLIENT_TOPIC_LEN];    memset(clientTopic,0,CLIENT_TOPIC_LEN);
    char *send = cj_create_keepAlive(0);//-------------1 ����     0��������  ������ȫ�ֱ�ʶ����������
    
    sprintf(clientTopic,"%s%s","/star_line/client/keepAlive/",getdeviceCode());
    
    log(DEBUG,"clientTopic ��%s��[%s]--[%d 1--�ж�0--����]\n",clientTopic,send,isr);
    
    if(isr)
      mqtt_send_publish_form_isr(&client,  (uint8_t *)clientTopic,  (uint8_t *)send, strlen(send), QOS1, 0);
    else
      mqtt_send_publish(&client,  (uint8_t *)clientTopic,  (uint8_t *)send, strlen(send), QOS1, 0);
}





//////////////////////////////����////////////////////////
typedef struct _cj_dispatchFilterItem
{
    char cardNo[17] ;
    long authEndTime;
    char filterType;
    long timeStamp;
} cj_dispatchFilterItem;

static void showdispatchFilterItem(cj_dispatchFilterItem *p)
{
   
    log(INFO,"p->taskID      %s\r\n",taskID);
    log(INFO,"p->cardNo      %s\r\n",p->cardNo);
    log(INFO,"p->authEndTime %ld\r\n",p->authEndTime);
    log(INFO,"p->filterType  %d\r\n",p->filterType);
    log(INFO,"p->timeStamp   %ld\r\n",p->timeStamp);

}
/*
topic /star_line/server/syncFilterItem/(�豸code)
type FilterItemSync struct {
	DeviceCode 		string 			`json:"deviceCode"`			//	�豸���
	TaskID			string 			`json:"taskID"`				//	�ỰID
	Data 	struct{
		CardNo 			string 			`json:"cardNo"`				//	����
		AuthEndTime 	int64			`json:"authEndTime"`		//	��Ȩ��ֹʱ��
		FilterType		int 			`json:"filterType"`			//	1: ������������2: ɾ�������� 3: ������������4: ȡ��������
		TimeStamp 		int64			`json:"timeStamp"`			//	��¼����ʱ��
	} `json:"data"`

}

 {"deviceCode":"110101001001003102001","taskID":"5bb56aaa4b4511ec935d0242ac170003","data":{"cardNo":"F1D47137042302E0","authEndTime":4102300800000,"filterType":1,"timeStamp":1637552180508}} 

*/
static int cj_parse_dispatchFilterItem(const char * pJson,cj_dispatchFilterItem *item)
{

      if(NULL == pJson) return 1;

      cJSON * pRoot = cJSON_Parse(pJson); MUST_TRUE(pRoot);
      
      cJSON * pSubONE = cJSON_GetObjectItem(pRoot, "taskID"); MUST_TRUE(pSubONE);

      memset(taskID,0,33);
      sprintf(taskID,"%.32s",pSubONE->valuestring);
   
      cJSON * pSubALL = cJSON_GetObjectItem(pRoot, "data");MUST_TRUE(pSubALL);
      
      cJSON * pSub = cJSON_GetObjectItem(pSubALL, "cardNo");MUST_TRUE(pSub);

      sprintf(item->cardNo,"%.16s",pSub->valuestring);

      pSub = cJSON_GetObjectItem(pSubALL, "timeStamp");MUST_TRUE(pSub);

      item->timeStamp = pSub->valueint;
    
      pSub = cJSON_GetObjectItem(pSubALL, "authEndTime");MUST_TRUE(pSub);

      item->authEndTime = pSub->valueint;
      
      pSub = cJSON_GetObjectItem(pSubALL, "filterType");MUST_TRUE(pSub);
      
      item->filterType = pSub->valueint;

      cJSON_Delete(pRoot);

      showdispatchFilterItem(item);
      
      return 0;
}



void downdispatchFilterItem(char *p)
{
        static int cnt=0;
        permiListType list;
        uint8_t userListResault = FALSE;
        cj_dispatchFilterItem item;
        
        memset(&item,0,sizeof(cj_dispatchFilterItem));
        
        if(cj_parse_dispatchFilterItem(p,&item)) return;

        log(DEBUG,"�ӵ��ڰ���������[cnt=%d],item.cardNo=%s , item.authEndTime=%ld , item.filterType=%d(1: ������������2: ɾ�������� 3: ������������4: ȡ��������)\n" ,++cnt,item.cardNo , item.authEndTime ,item.filterType  );
        list.ID = atol64((char*)item.cardNo);
        list.time = item.authEndTime;

                
        switch( item.filterType )
        {
            case 1:
            {
                list.status = LIST_BLACK;
                userListResault = permi.add(&list);
            }break;
            case 3:
            {
                list.status = LIST_WRITE;
                userListResault = permi.add(&list);
            }break;

            case 2:
            case 4:
            {
                userListResault = permi.del(list.ID);
            }break;
            default: log_err("�ڰ�����û��������� \n" );
        }
        
        if(userListResault == 1)
        {
          log_err("�ڰ�����permi�����ɹ�\n");  
          cj_response(taskID ,0 );
        }
        else
        {
         cj_response(taskID ,1 );
        }

}







/*
topic /star_line/server/otaDown/(�豸code)
type OtaDown struct {
	TaskID 			string 		`json:"taskID"`				//	�ỰID
	Data 	struct{
		FileUrl 	string 		`json:"fileUrl"`			//	�ļ����·��
		Md5Str		string 		`json:"md5Str"`				//	�ļ�md5ֵ
	} `json:"data"`
}


{"taskID":"11e3e0a27cc311eb91bc784f437b55ee","data":{"fileUrl":"http://139.9.66.72:17100/starline/headzip.bin","md5str":"2d5b4efd001049a67f7cd5e1e5da4c66","size":142430,"version":"1.0.0"}} 

*/

char downProgramURL(char *pJson)
{
      SHOWME 
      otaType otaCfg;
      char *p = NULL;
      char url[100]={0};
      cJSON * pSub  = NULL;
      uint8_t Md5[16]={0};
      cJSON * pRoot = cJSON_Parse(pJson); MUST_TRUE(pRoot);   
      cJSON * pSubONE = cJSON_GetObjectItem(pRoot, "taskID"); MUST_TRUE(pSubONE);

      memset(taskID,0,33);
      sprintf(taskID,"%.32s",pSubONE->valuestring);

      cJSON * pSubALL = cJSON_GetObjectItem(pRoot, "data");MUST_TRUE(pSubALL);

      if(NULL == pSubALL)
      {
          cJSON_Delete(pRoot);
          goto out;
      }   

      
      pSub = cJSON_GetObjectItem(pSubALL, "fileUrl");
      if(NULL == pSub)
      {
          cJSON_Delete(pRoot);
          goto out;
      }
      printf("��%s��",pSub->valuestring);
      memcpy(url,pSub->valuestring,strlen(pSub->valuestring));
 

      serverAddrType ip_port;
      p = strstr ((const char*)url,"//");
      p+=2;
      sscanf((const char*)p,"%[^:]", ip_port.ip);
      p = strstr ((const char*)p,":");
      p+=1;
      ip_port.port=atoi(p);
      
      ShowIp(&ip_port);
      config.write(CFG_OTA_ADDR ,&ip_port,0);


      p = strstr ((const char*)p,"/");
      config.write(CFG_OTA_URL ,p,0);


      pSub = cJSON_GetObjectItem(pSubALL, "md5Str");
      if(NULL == pSub)
      {
          cJSON_Delete(pRoot);
          goto out;
      }


      printf("��%s��",pSub->valuestring);

      memcpy_down(Md5,pSub->valuestring,32);
      otaCfg.crc32=CRC16_CCITT(Md5,16);      
            

      pSub = cJSON_GetObjectItem(pSubALL, "size");
      if(NULL == pSub)
      {
          cJSON_Delete(pRoot);
          goto out;
      }
      printf("��%d��",pSub->valueint);
      otaCfg.fileSize=pSub->valueint;
      
      
      pSub = cJSON_GetObjectItem(pSubALL, "version");
      if(NULL == pSub)
      {
          cJSON_Delete(pRoot);
          goto out;
      }

      printf("��%s��",pSub->valuestring);

      otaCfg.ver=InterVer(pSub->valuestring);
      printf("��%d��",otaCfg.ver);
          

       /*��Ϣȫ���õ��˵Ļ� �ͱ�������*/
      config.write(CFG_OTA_CONFIG , &otaCfg,1);
      
      cj_response(taskID ,0); 
      show_OTA();
      xSemaphoreGive(xMqttOtaSemaphore);   
      return 0;
      
out:
     cj_response(taskID ,1);
     return 1;

}



//////////////////////////////

/* {"taskID":"deb211024a4411ec87a60242ac170003","data":{"timeStamp":1637442019608}} */
/*
topic /star_line/server/timeCalibration
type TimeCalibration struct {
	TaskID			string 			`json:"taskID"`				//	�ỰID
	Data 	struct{
		TimeStamp 		int64 			`json:"timeStamp"`			// ʱ���
	} `json:"data"`

}
*/
static char downtimeCalibration(char *pJson) 
{    
  SHOWME
    
  uint32_t stamp = 0;

  cJSON * pRoot = cJSON_Parse(pJson);MUST_TRUE(pRoot);

  cJSON * pSub = cJSON_GetObjectItem(pRoot, "data");MUST_TRUE(pSub);

  cJSON * pSub_2 = cJSON_GetObjectItem(pSub, "timeStamp");MUST_TRUE(pSub_2);

  log(DEBUG,"[MQTT-TSL]��÷�����ͬ��ʱ��= %lf\n" ,  pSub_2->valuedouble);//1564725927916.000038 ��ʵ/1000 ��1564725927
 
  stamp = (uint32_t)(pSub_2->valuedouble/1000);   //https://tool.lu/timestamp/ �������׼��Ϊ����ʱ��
        
  if( !Gequal( stamp, rtc.read_stamp(), 5))
  {
        log(DEBUG,"[MQTT-TSL]��Ҫ�޸�ʱ��,����������ʱ��� = %d , �豸��ǰʱ��� = %d\n" , stamp , rtc.read_stamp());
        rtc.set_time_form_stamp(stamp);
  } else 
  log(DEBUG,"[MQTT-TSL]����Ҫ�޸�ʱ��,����������ʱ��� = %d , �豸��ǰʱ��� = %d\n" , stamp , rtc.read_stamp());
  
  cJSON_Delete(pRoot);
  return 0;
        
}

/*
topic /star_line/server/deviceControl/(�豸code)
type DeviceControlSync struct {
	DeviceCode 		string 			`json:"deviceCode"`			//	�豸���
	TaskID			string 			`json:"taskID"`				//	�ỰID
	Data 	struct{
		PhoneNo 	string 			`json:"phoneNo"`			//	�ֻ���
		Type 		int 			`json:"type"`				//	������������, 1-��ʾ����2-��ʾ��, 3-����
	} `json:"data"`


{"DeviceCode":"110101001001003102001","TaskID":"b47e8d194a6711ec87a60242ac170011","Data":{"PhoneNo":"13621896469","Type":1}} 
*/
static char downdeviceControl(char *pJson) 
{    
  SHOWME
  
  cJSON * pRoot = cJSON_Parse(pJson);MUST_TRUE(pRoot)

  cJSON * pSubONE = cJSON_GetObjectItem(pRoot, "taskID");MUST_TRUE(pSubONE)

  memset(taskID,0,33);
  sprintf(taskID,"%.32s",pSubONE->valuestring);


  cJSON * pSub = cJSON_GetObjectItem(pRoot, "data");MUST_TRUE(pSub)
  
  
  cJSON * pSub_2 = cJSON_GetObjectItem(pSub, "type");MUST_TRUE(pSub_2)
 
  
  log(INFO,"[MQTT-TSL]��÷�������������= %d(1-��ʾ����2-��ʾ��, 3-����)\n" ,  pSub_2->valueint); 
  if(pSub_2->valueint==1)
  {
        open_door();
        beep.write(BEEP_NORMAL);
        log(ERR,"[MQTT-TSL]Զ�̿��ųɹ�\n");
  }
  
  
  cj_response(taskID ,0); 
  cJSON_Delete(pRoot);
  return 0;
}


/*

{"taskID":"88d25d1d4b5811ec935d0242ac170003","deviceCode":"110101001001003102001","data":{"md5str":"a4b18b05dce72bdf144ca071865cc535","groupList":["1101010010010030000001","1101010010010030000002","1101010010010030000003"],"timeStamp":1637560416633}} 
type GroupSync struct {
	TaskID			string 			`json:"taskID"`				//	�ỰID
	DeviceCode 		string 			`json:"deviceCode"`			//	�豸���
	Data 	struct{
		Md5Str 			string 			`json:"md5str"`				//	����groupCodeList��md5ֵ
		GroupList 		[]string 		`json:"groupList"`			//	��code
		TimeStamp 		int64			`json:"timeStamp"`			//	��¼����ʱ��
	} `json:"data"`

}
*/
extern _SHType SHType;
static char downGupcode(char *pJson) 
{    
    SHOWME
    uint8_t dosave = 1;

    cJSON * pRoot = cJSON_Parse(pJson);MUST_TRUE(pRoot)

    cJSON * pSubONE = cJSON_GetObjectItem(pRoot, "taskID");MUST_TRUE(pSubONE)

    memset(taskID,0,33);
    sprintf(taskID,"%.32s",pSubONE->valuestring);


    cJSON * pSub = cJSON_GetObjectItem(pRoot, "data");MUST_TRUE(pSub)


    cJSON * pmd5str = cJSON_GetObjectItem(pSub, "md5str");MUST_TRUE(pmd5str)

    uint16_t md5 = CRC16_CCITT((uint8_t *)pmd5str->valuestring,32);
    printf("[MQTT-TSL]pmd5str = %s [%d][%d]\n",pmd5str->valuestring,md5,SHType.gup.md5);
    
    cJSON * gup_arry = cJSON_GetObjectItem(pSub, "groupList");MUST_TRUE(gup_arry)

    char  array_size   = cJSON_GetArraySize ( gup_arry );
    printf("[MQTT-TSL]array_size = [%d]\n",array_size);
    
    if(SHType.gup.cnt == array_size)
    {
      printf("[MQTT-TSL]first SHType.gup.cnt == array_size go on check md5\n");
      
      if(md5 == SHType.gup.md5)
      {
        printf("[MQTT-TSL]md5 == SHType.gup.md5 do nothing\n");
        dosave = 0;
      }     
    }
    
    if(dosave)
    {
      SHType.gup.cnt = array_size;
      SHType.gup.md5 = md5;
      
      for( char iCnt = 0 ; iCnt < array_size ; iCnt ++ ){
          pSub = cJSON_GetArrayItem(gup_arry, iCnt);
          if(NULL == pSub ){ continue ; }
          char * ivalue = pSub->valuestring ;
          memcpy_down(SHType.gup.code[iCnt],ivalue,22);
          printf("[MQTT-TSL]groupList[%d] : %s\r\n",iCnt,ivalue);
      }
      config.write(CFG_SYS_SHANGHAI ,&SHType, TRUE);
    }


    cj_response(taskID ,0); 
    cJSON_Delete(pRoot);
    show_SH(&SHType);
    return 0;
}


int tsl_mqtt_recv_message(mqttClientType* c , mqttRecvMsgType *p) 
{

    log(ERR,"[MQTT-TSL]�յ����� %d(enum{MQTIME,MQCTL,MQBWLIST,MQGUP,MQOTA,})\r\n",p->topicNo);
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
          downGupcode((char *)p->payload);
          break;
       case MQOTA:
          downProgramURL((char *)p->payload);
          break;
    }
    
    return MQTT_RECV_SUCCESS;
}
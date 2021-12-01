#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "cmsis_os.h"
#include "unit.h"
#include "mqtt_task.h"
#include "sysCfg.h"
#include "mqtt_client.h"
#include "timer.h"
#include "md5.h"
#include "sha1.h"
#include "sysCfg.h"
#include "utils_hmac.h"
#include "tsl_mqtt.h"
#include "rsa.h"
#include "encryDecry.h"
#include "open_log.h"
#include "mqtt_task.h"
#include "tsl_mqtt.h"
#include "sys_led.h"
#include "sysCntSave.h"

xTaskHandle                mqttTask;
uint8_t                    mqttRunType = GMQTT_CONNECT_TCP;
mqttClientType             client;
Network                    network;
uint8_t                    mqttAliveTimerPort = 0xFF,mqttKEEPAliveBLUE=0XFF;
extern void startota(void);

char suball(void)
{
    char rc = 0;
    rc = mqtt_subscribe(&client, ( uint8_t *)topicPath0 , QOS2, mqtt_message_arrived);if(rc !=0)return 1;
    rc = mqtt_subscribe(&client, ( uint8_t *)topicPath1 , QOS2, mqtt_message_arrived);if(rc !=0)return 1;
    rc = mqtt_subscribe(&client, ( uint8_t *)topicPath2 , QOS2, mqtt_message_arrived);if(rc !=0)return 1;
    rc = mqtt_subscribe(&client, ( uint8_t *)topicPath3 , QOS2, mqtt_message_arrived);if(rc !=0)return 1;
    rc = mqtt_subscribe(&client, ( uint8_t *)topicPath4 , QOS2, mqtt_message_arrived);if(rc !=0)return 1;
    return 0;
}

void pack_connect_message(MQTTPacket_connectData *con)
{


    uint8_t *pwd; 
    uint8_t *client; 
    uint8_t *username; 
    static  uint8_t  mqttWillInfoFlag = FALSE;

    if(mqttWillInfoFlag == FALSE)
    {


        config.read(CFG_MQTT_USERPWD, (void **)&pwd);
        config.read(CFG_MQTT_CLIENTID, (void **)&client);
        config.read(CFG_MQTT_USERNAME, (void **)&username);
        
        con->MQTTVersion = 4;
        con->clientID.cstring = client;
        con->username.cstring = username;
        con->password.cstring = pwd;
 

        log(DEBUG,"user name = %s \n", con->username.cstring);
        log(DEBUG,"password = %s \n", con->password.cstring);
        log(DEBUG,"clientID = %s \n", con->clientID.cstring);
        mqttWillInfoFlag = TRUE;
    }
  
}

void mqtt_login_info( MQTTPacket_connectData *con )
{
    pack_connect_message(con);
}

extern char *getdeviceCode();
extern void upuploadDevicever(void) ;
extern void upuploadDeviceInfo(void) ;
extern void upfilterRequest(void)  ;
extern void upkeepAlive(char isr) ;

void BLUE_keep_alive(void)
{
    upkeepAlive(1);
}

void mqtt_keep_alive( void )
{
    mqtt_send_keep_alive();
}

static void mqtt_task( void const *pvParameters)
{
    int rc;
    MQTTPacket_connectData    connectData = MQTTPacket_connectData_initializer;

    mqtt_network_init(&network);
    mqtt_client_init(&client, &network, 30 , 30000,mqttreadbuf, sizeof(mqttreadbuf));
    mqttAliveTimerPort = timer.creat(50000 , FALSE , mqtt_keep_alive );
    mqttKEEPAliveBLUE =  timer.creat(60000 , FALSE , BLUE_keep_alive );//60000 一分钟
        
    while(1)
    {

        switch(mqttRunType)
        {
        case GMQTT_INIT:                 
                  mqtt_disconnect(&client);
                  mqtt_network_close();
                  mqttRunType = GMQTT_CONNECT_TCP;
                break;
        case GMQTT_CONNECT_TCP:
                  /*
                  //GmakeJson();
        //          cj_create_uploadAccessLog_card(20190722,1,0,"HELLO",1,2);
        //          cj_create_uploadAccessLog_pwd (20190722,1,0,2);

                  */
          
                  if(0==strlen(getdeviceCode()))
                  {
                    log(WARN , "[MQTT-STA]尚未分配设备编码 去睡觉\n");
                    mqttRunType = GMQTT_INIT;
                    break;
                  }

                  timer.stop(mqttKEEPAliveBLUE);                    
                  serverAddrType *addr;         
                  config.read(CFG_NET_ADDR , (void **)&addr);
                  while ((rc = mqtt_network_connect(&network,addr->ip, addr->port )) < 0)
                  {
                    log(WARN , "[MQTT-STA]return code from network connect is %d\n", rc);
                    mqtt_network_close();
                  }
                  mqttRunType = GMQTT_CONNECT_MQTT;
                break;
        case GMQTT_CONNECT_MQTT:
                  mqtt_login_info(&connectData);             
                  if ((rc = mqtt_connect_server(&client, &connectData)) != MQTT_SUCCESS)
                  {
                      log(WARN,"[MQTT-STA]MQTT connect return code from MQTT connect is %d\n", rc);
                      mqttRunType = GMQTT_INIT;  
                  }
                  else
                  {
                      log(DEBUG,"[MQTT-STA]MQTT Connected success \n");    
                      timer.start(mqttAliveTimerPort);
                      mqttRunType = GMQTT_SUBSCRIBE;
                  }
                break;
        case GMQTT_SUBSCRIBE:
                  if (suball()!= 0 )
                      {
                        log(WARN,"[MQTT-STA]MQTT subscribe return code from MQTT subscribe is %d\n", rc);
                        mqttRunType = GMQTT_INIT;
                      }
                      else
                      {
                        log(DEBUG,"[MQTT-STA]MQTT subscribe success\r\n");
                        mqttRunType = GMQTT_DEVINFO;
                        sysLed.write(SYS_LED_CONNECT_SERVER);
                      }
                break;

        case GMQTT_DEVINFO:
                    upuploadDeviceInfo();
                    sys_delay(10);       
                    //vTaskSuspend( NULL );/*会把自己挂起 后面没有了  挂起*/
                    mqttRunType = GMQTT_VER;
                break;

        case GMQTT_VER:
                    upuploadDevicever();
                    sys_delay(10);
                    mqttRunType = GMQTT_ALIEVE;
                break;
  
        case GMQTT_ALIEVE:
                    upkeepAlive(0);
                    sys_delay(10);
                    timer.start(mqttKEEPAliveBLUE);
                    mqttRunType = GMQTT_FILTER;
                    //startota();
                break;  
                
       case GMQTT_FILTER:
                    upfilterRequest();
                    sys_delay(10);
                    mqttRunType = GMQTT_OK;                
                break;                
        case GMQTT_OK:
                  if( mqtt_run( &client ) < 0)
                  {
                      SHOWME
                      mqtt_disconnect(&client);
                      mqttRunType = GMQTT_CONNECT_MQTT;
                  }       
                  sys_delay(10);
                break;
              default:   break;
        }

    }
}

uint8_t mqtt_network_normal( void )
{
    return ((mqttRunType == GMQTT_OK ) ? TRUE : FALSE );
}

void mqtt_set_resert( void )
{
    mqtt_disconnect(&client);
    mqtt_network_close();
}

void create_mqtt_task( void )
{
    osThreadDef(net, mqtt_task, osPriorityHigh, 0, configMINIMAL_STACK_SIZE*10);
    mqttTask = osThreadCreate(osThread(net), NULL);
    configASSERT(mqttTask);
}


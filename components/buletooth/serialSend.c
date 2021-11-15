#include "serialSend.h"
#include "usart.h"
#include "config.h"
#include "string.h"
#include "unit.h"
#include "socket.h"
#include "sysCfg.h"
#include "tempwd.h"
#include "open_log.h"
#include "permi_list.h"
#include "bsp_rtc.h"
#include "mbedtls/md_internal.h"
#include "open_door.h"
#include "magnet.h"
#include "timer.h"
#include "androidSerial.h"
#include "cJSON.h"
#include "sysCntSave.h"
#include "BleDataHandle.h"

static xTaskHandle 	serialSendTask;
uint8_t macTimerHandle = 0;
uint8_t serialId = 0;
volatile uint8_t macReceiveFlag = false;
volatile uint8_t devParaReceiveFlag = false;
volatile uint8_t devInstallReceiveFlag = false;
volatile uint8_t installStatus = false;
volatile uint8_t getDevParaFlag = false;
volatile uint16_t devParaDataLen = 0;
/**
 * 标准的CRC校验算法
 * @src 校验字符串首地址
 * @sizes 总字节数
 */
uint16_t crc16Check(uint8_t *bufData, uint16_t buflen)
{
	int ret = 0;
	uint16_t crc = 0xffff;
	uint16_t polynomial = 0xa001;
	int i,j;
 
 
	if(bufData == NULL)
	{
		return -1;
	}
 
	if (buflen == 0)
	{
		return ret;
	}
	for (i = 0; i < buflen; i++)
	{
		crc ^= bufData[i];
		for (j = 0; j < 8; j++)
		{
			if ((crc & 0x0001) != 0)
			{
				crc >>= 1;
				crc ^= polynomial;
			}
			else
			{
				crc >>= 1;
			}
		}
	}
 
	return crc;
}


int8_t serialSendLogData(uint8_t cmdType, uint8_t *buf, int32_t len) {
    uint16_t dataLen = HEAD_LEN + len;
    uint16_t crc16 = 0;
    uint8_t sendData[512];
      
    uint8_t *pData = sendData;
    *pData++ = (uint8_t)(START_NUM >> 8);
    *pData++ = (uint8_t)(START_NUM);
    *pData++ = (uint8_t)(dataLen >> 8);
    *pData++ = (uint8_t)(dataLen);
    *pData++ = cmdType;
   // *pData++ = ++journalSn;
    if (buf != NULL) {
        memcpy(pData, buf, len);
        pData += len;
    }
    crc16 = crc16Check(sendData, dataLen);
    
    *pData++ = (uint8_t)(crc16 >> 8);
    *pData++ = (uint8_t)(crc16);
    
    androidSerial.send(sendData , dataLen+2);
    return 0;
}

int8_t serialSendData(uint8_t cmdType, uint8_t *buf, int32_t len) {
    uint16_t dataLen = HEAD_LEN + len;
    uint16_t crc16 = 0;
    uint8_t sendData[1024];
      
    uint8_t *pData = sendData;
    *pData++ = (uint8_t)(START_NUM >> 8);
    *pData++ = (uint8_t)(START_NUM);
    *pData++ = (uint8_t)(dataLen >> 8);
    *pData++ = (uint8_t)(dataLen);
    *pData++ = cmdType;
   // *pData++ = serialSn++;
    if (buf != NULL) {
        memcpy(pData, buf, len);
        pData += len;
    }
    crc16 = crc16Check(sendData, dataLen);
    
    *pData++ = (uint8_t)(crc16 >> 8);
    *pData++ = (uint8_t)(crc16);
    
    androidSerial.send(sendData , dataLen+2);
    return 0;
}

uint16_t send_json_data_for_mac(void)
{
    char *temp=NULL;
    char macTemp[64];
    uint8_t *mac;
    cJSON * root;  
    uint16_t len;
    uint8_t sendBuff[256];
      
    root = cJSON_CreateObject();
    if(!root) 
    {
        log(DEBUG,"get root faild !\n");
        return false;
    }     
    
    memset(sendBuff,0x00 , sizeof(sendBuff)); 
    memset(macTemp,0x00 , sizeof(macTemp)); 
    config.read(CFG_BLE_MAC , (void **)&mac);
    sprintf((char *)macTemp ,"%02X%02X%02X%02X%02X%02X",mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);

    cJSON_AddStringToObject(root,"bleMac",macTemp);
    temp = cJSON_PrintUnformatted( root );
    len = strlen(temp);
    memcpy( sendBuff, temp, len);

    serialSendData(HANDLE_TYPE_CMD_MAC,sendBuff,len);
    if(temp)
    {
       free(temp);
    }

    if(root)
    {
      cJSON_Delete(root);
    }
    return len;
}


uint16_t send_json_data_for_softVersion(void)
{
    char *temp=NULL;
    char softVersion[64];
    uint8_t sw = DEVICE_SW_VERSION;
    cJSON * root;  
    uint16_t len;
    uint8_t sendBuff[256];
      
    root = cJSON_CreateObject();
    if(!root) 
    {
        log(DEBUG,"get root faild !\n");
        return false;
    }     
    
    memset(sendBuff,0x00 , sizeof(sendBuff)); 
    memset(softVersion,0x00 , sizeof(softVersion)); 
    sprintf((char *)softVersion ,"%s%d%d%d","v",(sw/100) , (sw/10)%10 ,(sw%10));
    cJSON_AddStringToObject(root,"version",softVersion);
    temp = cJSON_PrintUnformatted( root );
    len = strlen(temp);
    memcpy( sendBuff, temp, len);

    serialSendData(HANDLE_TYPE_CMD_SOFTWARE_VERSION,sendBuff,len);
    if(temp)
    {
       free(temp);
    }

    if(root)
    {
      cJSON_Delete(root);
    }
    return len;
}

uint16_t send_json_data_for_list(void)
{
    char *temp=NULL;
    char macTemp[64];
    uint8_t *mac;
    cJSON * root;  
    uint16_t len;
    uint8_t sendBuff[256];
      
    root = cJSON_CreateObject();
    if(!root) 
    {
        log(DEBUG,"get root faild !\n");
        return false;
    }     
    
    memset(sendBuff,0x00 , sizeof(sendBuff)); 
    memset(macTemp,0x00 , sizeof(macTemp)); 
    config.read(CFG_BLE_MAC , (void **)&mac);
    sprintf((char *)macTemp ,"%02X%02X%02X%02X%02X%02X",mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);

    cJSON_AddStringToObject(root,"deviceNo",macTemp);
    cJSON_AddNumberToObject(root,"timeStamp",sysCnt.listUpdataTime);
    
    
    temp = cJSON_PrintUnformatted( root );
    len = strlen(temp);
    memcpy( sendBuff, temp, len);

    serialSendData(HANDLE_TYPE_CMD_LIST,sendBuff,len);
    if(temp)
    {
       free(temp);
    }

    if(root)
    {
      cJSON_Delete(root);
    }
    return len;
}

uint16_t send_json_data_for_status(int magnetState)
{
    char *temp=NULL;
    char macTemp[64];
    uint8_t *mac;
    cJSON * root;  
    uint16_t len;
    uint8_t sendBuff[256];
      
    root = cJSON_CreateObject();
    if(!root) 
    {
        log(DEBUG,"get root faild !\n");
        return false;
    }     
    
    memset(sendBuff,0x00 , sizeof(sendBuff)); 
    memset(macTemp,0x00 , sizeof(macTemp)); 
    config.read(CFG_BLE_MAC , (void **)&mac);
    sprintf((char *)macTemp ,"%02X%02X%02X%02X%02X%02X",mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);

    cJSON_AddStringToObject(root,"deviceNo",macTemp);
    if(magnetState == 2)
    {  
        cJSON_AddNumberToObject(root,"time",alarmStartTimer);
    }
    else
    {
        cJSON_AddNumberToObject(root,"time",rtc.read_stamp());
    }  
    cJSON_AddNumberToObject(root,"sensorStatus",magnetState);
    temp = cJSON_PrintUnformatted( root );
    len = strlen(temp);
    memcpy( sendBuff, temp, len);

    serialSendData(HANDLE_TYPE_CMD_STATUS,sendBuff,len);
    if(temp)
    {
       free(temp);
    }

    if(root)
    {
      cJSON_Delete(root);
    }
    return len;
}

uint16_t send_json_data_for_install_finish(void)
{
    serialSendData(HANDLE_TYPE_CMD_INSTALL_FINISH,NULL,0);
    
    return true;
}

uint16_t send_json_data_for_device_reset(void)
{
    serialSendData(HANDLE_TYPE_CMD_DEVICE_RESET,NULL,0);
    
    return true;
}

uint16_t send_json_data_for_open_succ(void)
{
    serialSendData(HANDLE_TYPE_CMD_OPEN_SUCC,NULL,0);
    
    return true;
}

uint16_t send_json_data_for_install_step_one(void)
{
    serialSendData(HANDLE_TYPE_CMD_INSTALL_ONE,devInstallInfo,DEV_INSTALL_DATA_LENGTH);

    return true;
}

uint16_t send_json_data_for_modify_para(void)
{
    char *temp=NULL;
    char macTemp[32];
    char bluetoothPassword[16];
    char doorOpenPassword[16];
    uint16_t OpenDelay;
    uint16_t AlarmTime;
    uint8_t *userPwd , *pairPwd;
    uint8_t *mac;
    cJSON * root;  
    uint16_t len;
    uint8_t sendBuff[512];
    
    root = cJSON_CreateObject();
    if(!root) 
    {
        log(DEBUG,"get root faild !\n");
        return false;
    }     
    
    memset(sendBuff,0x00 , sizeof(sendBuff)); 
    memset(macTemp,0x00 , sizeof(macTemp)); 
    memset(bluetoothPassword,0x00 , sizeof(bluetoothPassword)); 
    memset(doorOpenPassword,0x00 , sizeof(doorOpenPassword)); 

    config.read(CFG_BLE_MAC , (void **)&mac);
    config.read(CFG_PAIR_PWD , (void **)&pairPwd );
    config.read(CFG_USER_PWD , (void **)&userPwd ); 
    sprintf((char *)macTemp ,"%02X%02X%02X%02X%02X%02X",mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);    
    sprintf((char *)bluetoothPassword ,"%02X%02X%02X",pairPwd[0],pairPwd[1],pairPwd[2]);
    sprintf((char *)doorOpenPassword ,"%02X%02X%02X",userPwd[0],userPwd[1],userPwd[2]);
   

    OpenDelay  = config.read(CFG_SYS_OPEN_TIME , NULL)/10; 
    //AlarmTime = config.read(CFG_SYS_ALARM_TIME , NULL); 
    
    cJSON_AddStringToObject(root,"deviceMac",macTemp);
    cJSON_AddStringToObject(root,"bluetoothPassword",bluetoothPassword);
    cJSON_AddStringToObject(root,"doorOpenPassword",doorOpenPassword);
    cJSON_AddNumberToObject(root,"doorOpenDelay",OpenDelay);
    cJSON_AddNumberToObject(root,"doorAlarmDelay",AlarmTime);
    
    temp = cJSON_PrintUnformatted( root );
    len = strlen(temp);
    memcpy( sendBuff, temp, len);
    
    serialSendData(HANDLE_TYPE_CMD_MODIFY_PARA,sendBuff,len);
    if(temp)
    {
       free(temp);
    }

    if(root)
    {
      cJSON_Delete(root);
    }
    return len;
}

uint16_t send_json_data_for_set_para(void)
{
    serialSendData(HANDLE_TYPE_CMD_SET_PARA,devParaData,DEV_PARA_DATA_LENGTH);
    
    return true;
}

uint16_t send_json_data_for_get_install_status(void)
{
    serialSendData(HANDLE_TYPE_CMD_GET_INSTALL_STATUS,NULL,0);
    
    return true;
}

uint16_t send_json_data_for_get_para(void)
{
    serialSendData(HANDLE_TYPE_CMD_GET_PARA,NULL,0);
    
    return true;
}

void serial_send_message_handle( serialTaskQueueType *serial)
{

    switch( serial->handle )  
    {
        case HANDLE_TYPE_CMD_LOG:
        {
            uint8_t msg[512] ;
            uint16_t len =0;
            
            memset( msg , 0x00 , sizeof(msg));
              
            if( (len = journal_read_send_log(msg)) > 0)
            {           
                //log(DEBUG,"%.*s\n" , len,msg);
                
                serialSendLogData(HANDLE_TYPE_CMD_LOG,msg,len);
            }
        }break;
        case HANDLE_TYPE_CMD_STATUS:
		{
            if(serial->length == 2)
            {  
                log(INFO,"设备发送报警信息\n");
                send_json_data_for_status(serial->length);
                
                if(magnetAlarmStatus == STATUS_CLOSE)
                {  
                    alarmStartTimer = 0;
                }    
            }
            else if(serial->length == 3)
            {  
                log(INFO,"设备发送报警取消信息\n");
                send_json_data_for_status(serial->length);
                
                if(magnetAlarmStatus == STATUS_CLOSE)
                {  
                    alarmStartTimer = 0;
                }    
            }
            else if(serial->length == 0)
            {  
                log(INFO,"设备发送门磁开状态信息\n");
                send_json_data_for_status(serial->length); 
            }
            else if(serial->length == 1)
            {  
                log(INFO,"设备发送门磁关状态信息\n");
                send_json_data_for_status(serial->length); 
            }
            

		}break;
        case HANDLE_TYPE_CMD_MAC:
		{
            log(DEBUG,"设备发送mac地址给安卓\n");
			send_json_data_for_mac();
		}break;
        
        case HANDLE_TYPE_CMD_SOFTWARE_VERSION:
		{
            log(DEBUG,"设备发送读头版本号地址给安卓\n");
			send_json_data_for_softVersion();
		}break;
        
        case HANDLE_TYPE_CMD_LIST:
		{
            log(DEBUG,"设备发送黑白名单时间戳给安卓\n");
			send_json_data_for_list();
		}break;
        
        
        case HANDLE_TYPE_CMD_INSTALL_FINISH:
		{
            log(DEBUG,"设备发送安装工安装完成给安卓\n");
			send_json_data_for_install_finish();
		}break;
          
        case HANDLE_TYPE_CMD_DEVICE_RESET:
		{
            log(DEBUG,"设备发送安装工重置命令给安卓\n");
			send_json_data_for_device_reset();
		}break;
        
        case HANDLE_TYPE_CMD_OPEN_SUCC:
		{
            log(DEBUG,"设备发送开门成功给安卓\n");
			send_json_data_for_open_succ();
		}break;    
         
        case HANDLE_TYPE_CMD_INSTALL_ONE:
		{
            log(DEBUG,"设备发送设备注册信息给安卓\n");
			send_json_data_for_install_step_one();
		}break;  
        
        case HANDLE_TYPE_CMD_MODIFY_PARA:
		{
            log(DEBUG,"设备发送设备读头参数给安卓\n");
			send_json_data_for_modify_para();
		}break;       
        
        case HANDLE_TYPE_CMD_SET_PARA:
		{
            log(DEBUG,"设备发送设备设置参数给安卓\n");
			send_json_data_for_set_para();
		}break;
        
        case HANDLE_TYPE_CMD_GET_INSTALL_STATUS:
		{
            log(DEBUG,"设备发送获取安装状态命令给安卓\n");
			send_json_data_for_get_install_status();
		}break;         
          
        case HANDLE_TYPE_CMD_GET_PARA:
		{
            log(DEBUG,"设备发送获取设备参数命令给安卓\n");
			send_json_data_for_get_para();
		}break;   
        
        default:
        {
        	 log_err(" %s 没有这个处理选项 , HANDLE = %d\n" , __func__ , serial->handle );
        }break;
        
    }
}
 

void send_mac_data(void)
{
    if(macReceiveFlag)
    {
        timer.stop(macTimerHandle);
    }  
    else
    {
        serialTaskQueueType    sendLog ;

        sendLog.handle = HANDLE_TYPE_CMD_MAC;

        xQueueSendFromISR( xSerialQueue, ( void* )&sendLog, NULL );
    }
}

void send_softversion_data(void)
{
    serialTaskQueueType    sendLog ;

    sendLog.handle = HANDLE_TYPE_CMD_SOFTWARE_VERSION;

    xQueueSendFromISR( xSerialQueue, ( void* )&sendLog, NULL );
}

void send_list_data(void)
{
    serialTaskQueueType    sendLog ;

    sendLog.handle = HANDLE_TYPE_CMD_LIST;

    xQueueSend( xSerialQueue, ( void* )&sendLog, NULL );
}    
    
void send_install_finish_data(void)
{
    serialTaskQueueType    sendLog ;

    sendLog.handle = HANDLE_TYPE_CMD_INSTALL_FINISH;

    xQueueSend( xSerialQueue, ( void* )&sendLog, NULL );
}    

void send_device_reset_data(void)
{
    serialTaskQueueType    sendLog ;

    sendLog.handle = HANDLE_TYPE_CMD_DEVICE_RESET;

    xQueueSend( xSerialQueue, ( void* )&sendLog, NULL );
}    

void send_device_open_succ_data(void)
{
    serialTaskQueueType    sendLog ;

    sendLog.handle = HANDLE_TYPE_CMD_OPEN_SUCC;

    xQueueSend( xSerialQueue, ( void* )&sendLog, NULL );
}   

void send_device_install_data(void)
{
    serialTaskQueueType    sendLog ;

    sendLog.handle = HANDLE_TYPE_CMD_INSTALL_ONE;

    xQueueSend( xSerialQueue, ( void* )&sendLog, NULL );
}    

void send_device_modify_para_data(void)
{
    serialTaskQueueType    sendLog ;

    sendLog.handle = HANDLE_TYPE_CMD_MODIFY_PARA;

    xQueueSend( xSerialQueue, ( void* )&sendLog, NULL );
}  

void send_device_set_para_data(void)
{
    serialTaskQueueType    sendLog ;

    sendLog.handle = HANDLE_TYPE_CMD_SET_PARA;

    xQueueSend( xSerialQueue, ( void* )&sendLog, NULL );
}    

void send_device_get_install_status_data(void)
{
    serialTaskQueueType    sendLog ;

    sendLog.handle = HANDLE_TYPE_CMD_GET_INSTALL_STATUS;

    xQueueSend( xSerialQueue, ( void* )&sendLog, NULL );
}   

void send_device_get_para_data(void)
{
    serialTaskQueueType    sendLog ;

    sendLog.handle = HANDLE_TYPE_CMD_GET_PARA;

    xQueueSend( xSerialQueue, ( void* )&sendLog, NULL );
}    

static void serial_send_task( void const *pvParameters)
{
    serialTaskQueueType  serial;
            
    macTimerHandle = timer.creat(10000 , TRUE ,send_mac_data );
    
    for(; ;)
    {
        int systime = HAL_GetTick();
      
        if(xQueueReceive( xSerialQueue, &serial, 1000 ) == pdTRUE)
        {
            serial_send_message_handle(&serial);

            memset(&serial , 0x00 , sizeof(serialTaskQueueType));
            
            sys_delay(50);
        }
        //read_task_stack(__func__,serialSendTask);
        
        if(HAL_GetTick()-systime>3500)   
        {
            log(WARN,"serial send 喂狗时间:%d\n",HAL_GetTick()-systime);
        }  
    }
  
}

void creat_send_task( void )
{      
    osThreadDef( serialSend, serial_send_task , osPriorityNormal, 0, configMINIMAL_STACK_SIZE*10);
    serialSendTask = osThreadCreate(osThread(serialSend), NULL);
    configASSERT(serialSendTask);
}





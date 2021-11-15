#include "esp12s.h"
#include "stdlib.h"
#include "beep.h"
#include "bsp.h"
#include "usart.h"
#include "modules_init.h"
#include "config.h"
#include "timer.h"
#include "socket.h"
#include "syscfg.h"
#include "unit.h"


void                            *esp12sPort = NULL;

static portBASE_TYPE			xHigherPriorityTaskWoken = pdFALSE;
__IO uint8_t                    esp12sInitStatus = FALSE;
static xTaskHandle              esp12sTask; 

wifiRunStatusEnum                wifiRun = WIFI_INIT;
wifiRunEnum			 wifiRunStatus = WIFI_POWER_ON;
static __IO uint8_t             receiveTimeStart = FALSE;
static __IO uint32_t            receiveTimecnt = 0;
static __IO uint8_t				wifiReceiveMode = WIFI_DATA_MODE;
static __IO int8_t				receiveSocketConnectStatus = FALSE;
static __IO int8_t				sendStartStatus = FALSE;
static uint8_t					receiveReturnBuff[50] , receiveReturnBuffLeng;

void esp12s_close( void );




void esp12s_receive_byte( uint8_t ch )
{
    receiveTimeStart = TRUE;
    receiveTimecnt =0;
    receiveData.rxBuff[receiveData.len++] = ch;

    if ( receiveData.len >= RECEIVE_MAX)
    {
        log(WARN,"wifi recv too length\n");
        memset(&receiveData , 0x00 ,sizeof(usartReceiveDataType));
    }    
}


void esp12s_timer_isr(void)
{
	if( receiveTimeStart)
	{
		if( receiveTimecnt++ >= 3)
		{
			receiveTimeStart = FALSE;
			receiveTimecnt =0;
			//log(DEBUG,"WIFI接收一帧数据\n");
                        if((wifiReceiveMode == WIFI_COMMAND_MODE)||(wifiRunStatus != WIFI_INIT_SUCCESS))
			{
				receiveData.receiveFinsh = TRUE;
			}
			else
			{
                                xSemaphoreGiveFromISR( xUsartNetSemaphore, &xHigherPriorityTaskWoken );
                                portEND_SWITCHING_ISR(xHigherPriorityTaskWoken );
			}
		}
	}	
}

void esp12s_resert( void )
{
      log(DEBUG,"WIFI模块开始复位\n");

      pin_ops.pin_write(ESP12S_RESERT_PIN ,PIN_LOW);

      sys_delay(10);

      pin_ops.pin_write(ESP12S_RESERT_PIN ,PIN_HIGH);

      sys_delay(100);

      log(DEBUG,"WIFI模块复位完成\n");
}

void esp12s_init(void)
{
    esp12sPort = serial.open("serial2");
    
    if( esp12sPort == NULL)
    {
        beep.write(BEEP_ALARM);
        return ;
    }
    
    pin_ops.pin_mode(ESP12S_RESERT_PIN , PIN_MODE_OUTPUT);
    pin_ops.pin_write(ESP12S_RESERT_PIN ,PIN_HIGH);
    
    serial.init(esp12sPort  , 115200 ,esp12s_receive_byte);
    timer.creat(1 , TRUE , esp12s_timer_isr);

    wifiRunStatus = WIFI_POWER_OFF; 
    wifiRun = WIFI_INIT;
}


void wifi_send_data( uint8_t *data , uint16_t length)
{
    serial.puts(esp12sPort , data , length);

}

uint8_t wifi_send_receive( uint8_t *cmd , uint16_t length , usartReceiveDataType *respone , uint16_t timeout)
{
    uint16_t timeCnt = timeout/10;

    wifiReceiveMode = WIFI_COMMAND_MODE;

    memset( &receiveData ,0x00 ,  sizeof(usartReceiveDataType));
    
    wifi_send_data(cmd , length);

    while(timeCnt--)
    {
            if(receiveData.receiveFinsh == TRUE)
            {
                    //log(DEBUG,"命令返回 = %s\n" , receiveData.rxBuff);
                    memcpy(respone , &receiveData , sizeof(usartReceiveDataType));         
                    memset( &receiveData ,0x00 ,  sizeof(usartReceiveDataType));
                    wifiReceiveMode = WIFI_DATA_MODE;
                    return TRUE;
            }
            sys_delay(10);
            
    }
    wifiReceiveMode = WIFI_DATA_MODE;
    return FALSE;    
}

uint8_t wifi_send_command(uint8_t *cmd , uint16_t length , uint16_t timeout , uint8_t repert , uint8_t *checkData)
{
	usartReceiveDataType respone;

	memset(&respone , 0x00 , sizeof(usartReceiveDataType));
	
	while (repert--)
	{
		if( wifi_send_receive(cmd , length , &respone , timeout) ==TRUE )
		{
		    //log(DEBUG,"返回数据, DATA = %s\r\n" ,respone.rxBuff);		
			if( strstr(respone.rxBuff , (char const *)checkData) != NULL)
			{
				return TRUE;
			}
                        sys_delay(timeout);
			
			memset(&respone , 0x00 , sizeof(usartReceiveDataType));
			
		}	
		log(DEBUG,"repert=%d\n" ,repert);  
	}
	
	return FALSE;

}

uint16_t wifi_wait_data(  uint16_t timeout ,uint8_t *respone)
{
    uint16_t timeCnt = timeout/10;
	uint16_t rt = 0;
	wifiReceiveMode = WIFI_COMMAND_MODE;
    
	while(timeCnt--)
	{
		if(receiveData.receiveFinsh == TRUE)
		{
			//log(DEBUG,"命令返回 = %s\n" , receiveData.rxBuff);
            memcpy(respone , receiveData.rxBuff , receiveData.len);   
			rt = receiveData.len;
            memset( &receiveData ,0x00 ,  sizeof(usartReceiveDataType));
            wifiReceiveMode = WIFI_DATA_MODE;
            return rt;
		}
		sys_delay(10);
	}
	wifiReceiveMode = WIFI_DATA_MODE;
	return rt;   

}


uint8_t wifi_send_receive_repeat(uint8_t *cmd , uint16_t length ,  uint16_t timeout , uint8_t repert ,usartReceiveDataType *respone )
{

	while (1)
	{
		if( wifi_send_receive(cmd , length , respone , timeout) ==TRUE )
		{
			return TRUE;
		}

		if(repert != 0xFF)
		{
			if( --repert == 0 )	return FALSE;
		}
	}
}

uint8_t wifi_set_ssid( uint8_t *ssid , uint8_t *pwd )
{

    wifiApInfoType  wifiInfo;
    uint32_t restoreBit = SYS_NET_RESTORE_BIT;

    memset(&wifiInfo , 0x00 , sizeof(wifiApInfoType) );


    sprintf((char *)wifiInfo.ssid , "%s" , ssid);
    sprintf((char *)wifiInfo.pwd , "%s" , pwd);
        
    config.write(CFG_WIFI_INFO , &wifiInfo , TRUE);

    wifiRunStatus = WIFI_POWER_OFF; 
    wifiRun = WIFI_INIT;
    socket_clear_all();

    config.write(CFG_SET_RESTORE_FLAG , &restoreBit ,TRUE);
    
	return TRUE;
}


uint8_t wifi_set_connect_ap( ) 
{
    uint8_t connectAp[100] , len = 0 , repeat = 10 ;
	uint8_t respone[100];
	uint16_t  rxLen = 0;
    wifiApInfoType  *wifiInfo;

    config.read(CFG_WIFI_INFO,(void **)&wifiInfo);

    memset(connectAp , 0x00  , 100 );
    len = sprintf( (char *)connectAp , "AT+CWJAP_DEF=\"%s\",\"%s\"\r\n" , wifiInfo->ssid , wifiInfo->pwd);

    log(DEBUG,"WIFI set connect ap = %s \n" , connectAp);
	
	wifi_send_data(connectAp , len);

	while(repeat--)
	{
		memset(respone , 0x00 , sizeof(respone));
		rxLen = wifi_wait_data(2000 , respone );
		if( rxLen > 0)
		{
			log(DEBUG,"read buff =%s , repeat = %d\n" , respone , repeat );
			if( strstr((char *)respone , "OK") != NULL)
			{
				return TRUE;
			}
		}
	}
	
	return FALSE; 
}


uint8_t wifi_set_default( void )
{
    usartReceiveDataType respone;

    memset(&respone , 0x00 , sizeof(usartReceiveDataType) );
    if( wifi_send_receive_repeat("AT+GMR\r\n" , strlen("AT+GMR\r\n") , 1000 , 1 , &respone) == FALSE)
    {
        log(INFO,"获取WIFI模块版本信息错误\n");
        return FALSE;
    }

    log(DEBUG,"wifi 模块版本信息 : %s\n" , respone.rxBuff);

    if( wifi_send_command("ATE0\r\n" , strlen("ATE0\r\n") , 1000 , 1, "\r\nOK\r\n") == FALSE )
    {
        log(INFO,"WIFI模块设置回显失败\n");
        return FALSE;
    }

    log(DEBUG,"WIFI关闭回显\n");

    if( wifi_send_command("AT+CWMODE_CUR=1\r\n" , strlen("AT+CWMODE_CUR=1\r\n") , 1000 , 1, "\r\nOK\r\n")  == FALSE )
    {
        log(INFO,"WIFI模块设置运行模式失败，MODE= STATION n");
        return FALSE;
    }

    log(DEBUG,"WIFI模块设置运行模式成功，MODE=STATION\n");
    
    if( wifi_send_command("AT+CWDHCP_DEF=1,1\r\n" , strlen("AT+CWDHCP_DEF=1,1\r\n") , 1000 , 1, "\r\nOK\r\n")  == FALSE )
    {
        log(INFO,"WIFI模块设置自动DHCP失败\n");
        return FALSE;
    }

    log(DEBUG,"WIFI模块设置自动DHCP成功\n");
    

    if( wifi_send_command("AT+CWAUTOCONN=1\r\n" , strlen("AT+CWAUTOCONN=1\r\n") , 1000 , 1, "\r\nOK\r\n")  == FALSE )
    {
        log(INFO,"WIFI模块设置自动连接失败\n");
        return FALSE;
    }

    if( wifi_set_connect_ap() == FALSE)
    {
        log(INFO,"WIFI模块设置SSID失败\n");
        return FALSE;
    }
	
    log(INFO,"WIFI模块设置SSID成功\n");
    
    log(DEBUG,"WIFI模块设置自动连接成功\n");

    log(DEBUG,"WIFI模块初始化设置成功\n");

    return TRUE;

}


uint8_t wifi_check_power_on( void )
{
    if ( wifi_send_command("AT\r\n" , strlen("AT\r\n") , 500 ,20 , "\r\nOK\r\n") == TRUE )
    {          
        log(INFO,"WIFI开机成功\n");
        return TRUE;
    }

	log(INFO,"WIFI开机失败\n");
	return FALSE;    
}


uint8_t esp12s_read_connect_status( void )
{
    usartReceiveDataType respone;
    uint8_t connectStatus = 0;
    uint8_t repeatCnt = 5;

    while(repeatCnt--)
    {
        memset(&respone , 0x00 , sizeof(usartReceiveDataType) );
        if( wifi_send_receive_repeat("AT+CIPSTATUS\r\n" , strlen("AT+CIPSTATUS\r\n") , 1000 , 1 , &respone) == FALSE)
        {
            log(INFO,"获取WIFI模块连接信息错误\n");
            
            wifiRunStatus = WIFI_POWER_OFF;
            
            return FALSE;
        }
        
        connectStatus  = respone.rxBuff[7]-'0';

        if( connectStatus <= WIFI_DISCONNECT_AP )
        {
            log(DEBUG,"WIFI模块连接状态:%d\n" , connectStatus);
            return connectStatus;
        }
        
        sys_delay(100);
    }

    
    return WIFI_INIT_STATUS;

}



void esp12s_start_init( void )
{

    switch(wifiRunStatus)
    {
        case WIFI_POWER_OFF:
        {
            esp12s_resert();
            wifiRunStatus = WIFI_POWER_ON;

        }break;
    	case WIFI_POWER_ON:
        {
            wifi_assert(wifi_check_power_on() , WIFI_SET_ECHO);
        }break;
        case WIFI_SET_ECHO:
        {
            if ( wifi_send_command("ATE0\r\n" , strlen("ATE0\r\n") , 500 ,20 , "\r\nOK\r\n") == TRUE )
            {          
                log(DEBUG,"WIFI设置回显成功\n");
                wifiRunStatus = WIFI_SET_DEFAULT;
            }
            else
            {
                log(INFO,"WIFI设置回显失败\n");
                wifiRunStatus =  WIFI_POWER_OFF; 
            }
        }break;

    	case WIFI_SET_DEFAULT:
        {
            uint32_t restoreBit = 0;

            restoreBit = config.read(CFG_SET_RESTORE_FLAG , NULL);

            if( restoreBit & SYS_NET_RESTORE_BIT)
            {
                if( wifi_set_default() == TRUE )
                {
                    uint32_t bit =  SYS_NET_RESTORE_BIT;
                    config.write(CFG_CLEAR_RESTORE_FLAG , &bit , TRUE );
                    wifiRunStatus = WIFI_WAIT_CONNECT;
                }
                else   
                {
                    wifiRunStatus = WIFI_POWER_OFF;
                }
            }
            else
            {
                wifiRunStatus = WIFI_WAIT_CONNECT;
            }
            
        }break;
    	case WIFI_WAIT_CONNECT:
        {
			uint32_t timeout = 30;

			while( --timeout != 0 )
			{				
                if( esp12s_read_connect_status() <  WIFI_DISCONNECT_AP )
                {
                    wifiRunStatus = WIFI_WAIT_GOT_IP;
                    return ;
                }
                
                sys_delay(1000);
			}

			wifiRunStatus = WIFI_POWER_OFF;
			
 
        }break;
    	case WIFI_WAIT_GOT_IP:
        {

            usartReceiveDataType respone;

			memset(&respone , 0x00 , sizeof(usartReceiveDataType) );
			
			if( wifi_send_receive_repeat("AT+CIFSR\r\n" , strlen("AT+CIFSR\r\n") , 1000 , 1 , &respone) == FALSE)
			{
				log(INFO,"获取WIFI模块IP信息错误\n");
				wifiRunStatus = WIFI_POWER_OFF;

				return;
			}

			log(DEBUG,"wifi 模块IP信息 : \n%s\n" , respone.rxBuff);

			wifiRunStatus = WIFI_SET_MUX;
  
			
        }break;

        case WIFI_SET_MUX:
        {
            if ( wifi_send_command("AT+CIPMUX=1\r\n" , strlen("AT+CIPMUX=1\r\n") , 500 ,20 , "\r\nOK\r\n") == TRUE )
            {          
                log(INFO,"WIFI设置多连接模式成功\n");
                wifiRunStatus = WIFI_INIT_FINSG;
            }
            else
            {
                log(INFO,"WIFI设置多连接模式失败\n");
                wifiRunStatus =  WIFI_POWER_OFF; 
            }
        }break;
    	case WIFI_INIT_FINSG:
        {
            log(DEBUG,"WIFI初始化完成，可以建立tcp连接\n");
            wifiRunStatus = WIFI_INIT_SUCCESS;
        }break;
        case WIFI_INIT_SUCCESS:
        {
            wifiRun = WIFI_RECEIVE;
            socket_set_status(SOCKET_WORKING_STATUS);
        }break;

        default: log(WARN,"wifi init no this handle = %d\n" , wifiRunStatus); wifiRunStatus = WIFI_POWER_OFF; break;
    }

}


static void wifi_rx_data_analisys(char *str , int len)
{
	uint8_t *otp = (uint8_t *)str;
	
	while((len-- > 0)&&( otp != NULL ))
	{

		if( aiot_strcmp(otp+2 , "CONNECT" , strlen("CONNECT")) == TRUE )
        {
            receiveSocketConnectStatus = TRUE;
            
            xSemaphoreGive( xSocketConnectSemaphore);
            otp = otp+strlen("0,CONNECT");
			len -= strlen("0,CONNECT");
			
			continue;
        }
        else if( aiot_strcmp(otp+2 , "CLOSE" , strlen("CLOSE")) == TRUE )
        {
            uint8_t *pst = otp;
			int8_t id =0 ;
			sockeArryType   *socket;
            
			id = *pst - '0';

			log(WARN,"关闭socket = %d\n" , id);

			socket =   socket_read_obj(id);
			if(socket->useFlag == TRUE)
			{
				socket->len = 0;
				socket->status =  SOCKET_CLOSE;
				socket->useFlag = FALSE;
				socket->msg = NULL;
			}
			else
			{
				log(WARN,"socket未使用\n");
			}
			otp = pst+strlen("0,CLOSED");
			len -= strlen("0,CLOSED");
			continue;
        }
        else if( aiot_strcmp( otp ,"\r\n+IPD,",strlen("\r\n+IPD,")) == TRUE )
        {
            uint8_t *pst = otp + strlen("\r\n+IPD,") ;
            int8_t id = 0 , i = 0;
			uint16_t dlen = 0;
            uint8_t lenBuff[6];
			sockeArryType   *socket;

            id = *pst++ -'0';
            
            memset(lenBuff , 0x00 , sizeof(lenBuff));
        	pst++;

            while(*pst != ':')
    		{
        		lenBuff[i++] = *pst++;
                if(i >4 )
                {
                    log_err("ipd error\n");
                    return ;
                }
    		}
    		dlen = atoi((char *)lenBuff);
            
            if( dlen > 0 )
    		{
				socket =   socket_read_obj(id);
				if(( socket->useFlag == TRUE ) &&(socket->id == id))
				{						
					socket->status = SOCKET_READ;
					log(INFO," socket = %d ,len = %d , data len = %d\n" , id , dlen , socket->len);
					if( socket->msg != NULL)
					{
                                          if( socket->len + dlen < socket->maxSize )
                                          {
                                              memcpy(socket->msg+socket->len , pst+1 ,dlen);
                                               log_arry(WARN,"IS "  ,pst+1 ,dlen);
                                               if(socket)printf("[%s]\r\n",pst+1);
                                              socket->len += dlen;
                                          }
                                          else
                                          {
                                              log_err("接收数据太长\n");
                                          }
					}
					else
					{
						log(WARN,"当前socket的Buff未启用\n");
					}
				}
				else
				{
					log(WARN,"socket未启用 , 或者不是当前Id , id=%d , socket id =%d\n" , id , socket->id);
				}
    		}
			otp = pst+1+dlen;
            len = len - ( dlen + strlen("\r\n+IPD,")+i+1);
            continue;
        }
        else if(aiot_strcmp(otp , "\r\nOK\r\n>" , strlen("\r\nOK\r\n>")) == true )
		{
			//printf("准备发送数据\n");
			otp = otp + strlen("\r\nOK\r\n>");
			len -= strlen("\r\nOK\r\n>");
            xSemaphoreGive( xSocketCmdSemaphore );
			continue;
		}
        else if(strstr((char *)otp, "SEND OK") != NULL )
		{
			//int socket =0;

			//socket = *pst - '0';
			
			//log(DEBUG,"socket 发送完成 \n");
			
			otp = otp+strlen("SEND OK");
			len -= strlen("SEND OK");
			
            xSemaphoreGive( xSocketSendSemaphore );
            
			continue;
		}
		
		else if(aiot_strcmp(otp, receiveReturnBuff , receiveReturnBuffLeng ) == true )
		{
			otp += receiveReturnBuffLeng;
			len -= receiveReturnBuffLeng;
            
			continue;
		}
        else if ( (aiot_strcmp(otp, "WIFI DISCONNECT" , strlen("WIFI DISCONNECT") ) == true )||
				  (aiot_strcmp(otp, "UNLINK" , strlen("UNLINK") ) == true )||
				   (aiot_strcmp(otp, "link is not valid" , strlen("link is not valid") ) == true ))
		{
		
            wifiRunStatus = WIFI_POWER_OFF; 
            wifiRun = WIFI_INIT;
			socket_clear_all();

			return ;
		}
		otp++;
    
        if(( len < 0) || ( len > RECEIVE_MAX))
        {
            log(INFO,"esp12解析失败\n");
            return;
        }
	}
	//log(DEBUG,"\n数据解析完成\n");
	
}


void wifi_module_receive( void )
{
    
    if( xSemaphoreTake( xUsartNetSemaphore, 2000 ) == pdTRUE )
    {
        usartReceiveDataType    wifiRxData;
        
        //vTaskSuspendAll();
        memset( &wifiRxData ,0x00 ,  sizeof(usartReceiveDataType));
        memcpy(&wifiRxData , &receiveData , sizeof(usartReceiveDataType));
        memset( &receiveData ,0x00 ,  sizeof(usartReceiveDataType));
        
        wifi_rx_data_analisys(wifiRxData.rxBuff , wifiRxData.len);

       // xTaskResumeAll();
    }

}


void esp12s_task( void const *pvParameters)
{
	esp12s_init();
    
	while(1)
	{
	    switch(wifiRun)
        {
            case WIFI_INIT:
            {
                esp12s_start_init();
            }break;
            case WIFI_RECEIVE:
            {
				wifi_module_receive();
            }break;
            default: log(WARN,"wifi run no this handle = %d\n" , wifiRun); wifiRun = WIFI_INIT; break;
        } 
		sys_delay(10);
        //read_task_stack(__func__,esp12sTask);
	}
}



uint8_t	esp12s_isOK( void )
{
	return ((wifiRun ==WIFI_RECEIVE)?TRUE:FALSE);
}


int8_t esp12s_disconnect( int8_t id )
{
	usartReceiveDataType respone;
    
    uint8_t sendBuff[128] ,sendSize =0;
	
	
	log(DEBUG,"关闭socket = %d \n" , id);

	memset(sendBuff , 0x00 , 128);

	sendSize = sprintf((char *)sendBuff , "AT+CIPCLOSE=%d\r\n" , id);

	if( wifi_send_receive_repeat(sendBuff ,sendSize , 10000 , 1 , &respone) == TRUE)
	{
        if( strstr( respone.rxBuff+2 , "CLOSED\r\n\r\nOK\r\n") != NULL)
        {
            log(DEBUG,"socket [%d] 连接关闭成功\n" , id);
            return TRUE;
        }
	}

	log(DEBUG,"socket [%d] 连接关闭失败\n" , id);


	return TRUE;
}


int8_t esp12s_connect( int8_t id , uint8_t *ip , uint16_t port)
{
    usartReceiveDataType respone;
    uint8_t sendBuff[256] ,sendSize =0 , connectip[100];
    int8_t rId = 0;

    //log(DEBUG,"esp12s 连接服务器, id =%d , ip = %s, port =%d\n" , id , ip , port);

    memset(sendBuff , 0x00 , 256);
    memset(connectip , 0x00 , 100);


    if((strstr((char *)ip,"com")!=NULL)||(strstr((char *)ip,"cn")!=NULL))
    {
        memcpy(connectip , ip , strlen((char *)ip));
    }
    else if((strstr((char *)ip,".")!=NULL))
    {
       log(INFO,"地址是string 192.168.1.2这样 【%s】\n",ip); 
       memcpy(connectip , ip , strlen((char *)ip));
    }
    
    
    else
    {
        log(INFO,"地址是数字\r\n"); 
        sprintf( (char *)connectip , "%d.%d.%d.%d" , ip[0],ip[1],ip[2],ip[3]);
    }

    sendSize = sprintf((char *)sendBuff , "AT+CIPSTART=%d,\"TCP\",\"%s\",%d\r\n" , id ,  connectip , port);
    
    log(DEBUG,"wifi connect data:%s\n" , sendBuff);
    
    if( wifi_send_receive_repeat(sendBuff,sendSize , 30000 , 1 ,&respone) == TRUE)
    {
        if( strstr(respone.rxBuff , "ERROR") != NULL)
        {
            if( strstr(respone.rxBuff , "ALREADY CONNECT") != NULL)
            {
                log(INFO,"已经与服务器建立连接 , socket id = %d\n" , id);
                return SOCKET_OK;
            }
            else
            {
                log(WARN,"1模块返回连接服务器失败 , socket id = %d\n" , id);
                return SOCKET_CONNECT_FAIL;
            }
        }
        else if( strstr(respone.rxBuff , "OK") != NULL)
        {
            rId = respone.rxBuff[0] - '0';

            if( rId == id )
            {
                log(INFO,"连接服务器成功 , socket id = %d\n" , id);
                return SOCKET_OK;
            }
        }
        else if( xSemaphoreTake( xSocketConnectSemaphore, 30000 ) == pdTRUE )
        {
            if ( receiveSocketConnectStatus == TRUE)
            {
                log(INFO,"连接服务器成功 , socket id = %d\n" , id);
                return SOCKET_OK;
            }
            else
            {
                log(WARN,"2模块返回连接服务器失败 , socket id = %d\n" , id);
                return SOCKET_CONNECT_FAIL;
            }
        }
        log(WARN,"服务器连接函数接收到数据, DATA = %s\n" , respone.rxBuff);
        wifi_rx_data_analisys(respone.rxBuff , respone.len);
        memset( &respone ,0x00 ,  sizeof(usartReceiveDataType));
        return SOCKET_CONNECT_TIMEOUT;
    }
    else
    {
        log(WARN,"WIFI发起链接服务器没有响应，串口接受有问题，复位模块");
        return SOCKET_CONNECT_NORES;
    }
 
}

int8_t esp12s_send( uint8_t id , uint8_t *sendData , uint16_t length )
{
    uint8_t comm[64] , cmdLength =0 ;
    
	//log_arry(DEBUG,"esp12s 发送数据," , sendData , length);
	
    sendStartStatus = TRUE;
    memset(comm ,0x00 , sizeof(comm));
	cmdLength = sprintf((char *)comm , "AT+CIPSEND=%d,%d\r\n" , id ,length);
    wifi_send_data(comm , cmdLength);
    
    if( xSemaphoreTake( xSocketCmdSemaphore, 30000 ) == pdTRUE )
    {    	
        if( sendStartStatus == FALSE )
        {            
            log(WARN,"scoket %d 发送数据返回错误\n" , id);
        	return SOCKET_SEND_RETURNERR;
        }
		memset(receiveReturnBuff , 0x00 , 50);
		receiveReturnBuffLeng = 0;
		receiveReturnBuffLeng = sprintf((char *)receiveReturnBuff , "\r\nRecv %d bytes\r\n" , length);
        wifi_send_data(sendData ,length );

        if( xSemaphoreTake( xSocketSendSemaphore, 5000 ) == pdTRUE )
        {
        	if( sendStartStatus == TRUE )
        	{
        		sendStartStatus = FALSE;
               // log(DEBUG,"ESP12 发送完成\n");
        		return SOCKET_OK;
        	}
        	sendStartStatus = FALSE;
        	 log(WARN,"scoket %d 发送数据返回失败\n" , id);
        	return SOCKET_SEND_FAIL;
        }
        sendStartStatus = FALSE;
        log(WARN,"scoket %d 发送数据无接收成功返回\n" , id);
        return SOCKET_OK;//SOCKET_SEND_NORESULT;
    }
    sendStartStatus = FALSE;
    log(WARN,"scoket %d 发送数据无准备返回\n" , id);
    return SOCKET_SEND_NOREADY;

}

void esp12s_close( void )
{

    wifiRunStatus = WIFI_POWER_OFF; 
    wifiRun = WIFI_INIT;
    socket_set_status(SOCKET_CLOSE_STATUS);

}



void creat_esp12s_task( void )
{
    osThreadDef( esp12s, esp12s_task , osPriorityRealtime, 0, configMINIMAL_STACK_SIZE*15);
    esp12sTask = osThreadCreate(osThread(esp12s), NULL);
    configASSERT(esp12sTask);
}

devComType	wifi=
{
    .init = creat_esp12s_task,
    .isOK = esp12s_isOK,
    .connect = esp12s_connect,
    .disconnect = esp12s_disconnect,
    .send = esp12s_send,
    .close = esp12s_close,
};

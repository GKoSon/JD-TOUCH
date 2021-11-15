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
			//log(DEBUG,"WIFI����һ֡����\n");
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
      log(DEBUG,"WIFIģ�鿪ʼ��λ\n");

      pin_ops.pin_write(ESP12S_RESERT_PIN ,PIN_LOW);

      sys_delay(10);

      pin_ops.pin_write(ESP12S_RESERT_PIN ,PIN_HIGH);

      sys_delay(100);

      log(DEBUG,"WIFIģ�鸴λ���\n");
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
                    //log(DEBUG,"����� = %s\n" , receiveData.rxBuff);
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
		    //log(DEBUG,"��������, DATA = %s\r\n" ,respone.rxBuff);		
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
			//log(DEBUG,"����� = %s\n" , receiveData.rxBuff);
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
        log(INFO,"��ȡWIFIģ��汾��Ϣ����\n");
        return FALSE;
    }

    log(DEBUG,"wifi ģ��汾��Ϣ : %s\n" , respone.rxBuff);

    if( wifi_send_command("ATE0\r\n" , strlen("ATE0\r\n") , 1000 , 1, "\r\nOK\r\n") == FALSE )
    {
        log(INFO,"WIFIģ�����û���ʧ��\n");
        return FALSE;
    }

    log(DEBUG,"WIFI�رջ���\n");

    if( wifi_send_command("AT+CWMODE_CUR=1\r\n" , strlen("AT+CWMODE_CUR=1\r\n") , 1000 , 1, "\r\nOK\r\n")  == FALSE )
    {
        log(INFO,"WIFIģ����������ģʽʧ�ܣ�MODE= STATION n");
        return FALSE;
    }

    log(DEBUG,"WIFIģ����������ģʽ�ɹ���MODE=STATION\n");
    
    if( wifi_send_command("AT+CWDHCP_DEF=1,1\r\n" , strlen("AT+CWDHCP_DEF=1,1\r\n") , 1000 , 1, "\r\nOK\r\n")  == FALSE )
    {
        log(INFO,"WIFIģ�������Զ�DHCPʧ��\n");
        return FALSE;
    }

    log(DEBUG,"WIFIģ�������Զ�DHCP�ɹ�\n");
    

    if( wifi_send_command("AT+CWAUTOCONN=1\r\n" , strlen("AT+CWAUTOCONN=1\r\n") , 1000 , 1, "\r\nOK\r\n")  == FALSE )
    {
        log(INFO,"WIFIģ�������Զ�����ʧ��\n");
        return FALSE;
    }

    if( wifi_set_connect_ap() == FALSE)
    {
        log(INFO,"WIFIģ������SSIDʧ��\n");
        return FALSE;
    }
	
    log(INFO,"WIFIģ������SSID�ɹ�\n");
    
    log(DEBUG,"WIFIģ�������Զ����ӳɹ�\n");

    log(DEBUG,"WIFIģ���ʼ�����óɹ�\n");

    return TRUE;

}


uint8_t wifi_check_power_on( void )
{
    if ( wifi_send_command("AT\r\n" , strlen("AT\r\n") , 500 ,20 , "\r\nOK\r\n") == TRUE )
    {          
        log(INFO,"WIFI�����ɹ�\n");
        return TRUE;
    }

	log(INFO,"WIFI����ʧ��\n");
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
            log(INFO,"��ȡWIFIģ��������Ϣ����\n");
            
            wifiRunStatus = WIFI_POWER_OFF;
            
            return FALSE;
        }
        
        connectStatus  = respone.rxBuff[7]-'0';

        if( connectStatus <= WIFI_DISCONNECT_AP )
        {
            log(DEBUG,"WIFIģ������״̬:%d\n" , connectStatus);
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
                log(DEBUG,"WIFI���û��Գɹ�\n");
                wifiRunStatus = WIFI_SET_DEFAULT;
            }
            else
            {
                log(INFO,"WIFI���û���ʧ��\n");
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
				log(INFO,"��ȡWIFIģ��IP��Ϣ����\n");
				wifiRunStatus = WIFI_POWER_OFF;

				return;
			}

			log(DEBUG,"wifi ģ��IP��Ϣ : \n%s\n" , respone.rxBuff);

			wifiRunStatus = WIFI_SET_MUX;
  
			
        }break;

        case WIFI_SET_MUX:
        {
            if ( wifi_send_command("AT+CIPMUX=1\r\n" , strlen("AT+CIPMUX=1\r\n") , 500 ,20 , "\r\nOK\r\n") == TRUE )
            {          
                log(INFO,"WIFI���ö�����ģʽ�ɹ�\n");
                wifiRunStatus = WIFI_INIT_FINSG;
            }
            else
            {
                log(INFO,"WIFI���ö�����ģʽʧ��\n");
                wifiRunStatus =  WIFI_POWER_OFF; 
            }
        }break;
    	case WIFI_INIT_FINSG:
        {
            log(DEBUG,"WIFI��ʼ����ɣ����Խ���tcp����\n");
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

			log(WARN,"�ر�socket = %d\n" , id);

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
				log(WARN,"socketδʹ��\n");
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
                                              log_err("��������̫��\n");
                                          }
					}
					else
					{
						log(WARN,"��ǰsocket��Buffδ����\n");
					}
				}
				else
				{
					log(WARN,"socketδ���� , ���߲��ǵ�ǰId , id=%d , socket id =%d\n" , id , socket->id);
				}
    		}
			otp = pst+1+dlen;
            len = len - ( dlen + strlen("\r\n+IPD,")+i+1);
            continue;
        }
        else if(aiot_strcmp(otp , "\r\nOK\r\n>" , strlen("\r\nOK\r\n>")) == true )
		{
			//printf("׼����������\n");
			otp = otp + strlen("\r\nOK\r\n>");
			len -= strlen("\r\nOK\r\n>");
            xSemaphoreGive( xSocketCmdSemaphore );
			continue;
		}
        else if(strstr((char *)otp, "SEND OK") != NULL )
		{
			//int socket =0;

			//socket = *pst - '0';
			
			//log(DEBUG,"socket ������� \n");
			
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
            log(INFO,"esp12����ʧ��\n");
            return;
        }
	}
	//log(DEBUG,"\n���ݽ������\n");
	
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
	
	
	log(DEBUG,"�ر�socket = %d \n" , id);

	memset(sendBuff , 0x00 , 128);

	sendSize = sprintf((char *)sendBuff , "AT+CIPCLOSE=%d\r\n" , id);

	if( wifi_send_receive_repeat(sendBuff ,sendSize , 10000 , 1 , &respone) == TRUE)
	{
        if( strstr( respone.rxBuff+2 , "CLOSED\r\n\r\nOK\r\n") != NULL)
        {
            log(DEBUG,"socket [%d] ���ӹرճɹ�\n" , id);
            return TRUE;
        }
	}

	log(DEBUG,"socket [%d] ���ӹر�ʧ��\n" , id);


	return TRUE;
}


int8_t esp12s_connect( int8_t id , uint8_t *ip , uint16_t port)
{
    usartReceiveDataType respone;
    uint8_t sendBuff[256] ,sendSize =0 , connectip[100];
    int8_t rId = 0;

    //log(DEBUG,"esp12s ���ӷ�����, id =%d , ip = %s, port =%d\n" , id , ip , port);

    memset(sendBuff , 0x00 , 256);
    memset(connectip , 0x00 , 100);


    if((strstr((char *)ip,"com")!=NULL)||(strstr((char *)ip,"cn")!=NULL))
    {
        memcpy(connectip , ip , strlen((char *)ip));
    }
    else if((strstr((char *)ip,".")!=NULL))
    {
       log(INFO,"��ַ��string 192.168.1.2���� ��%s��\n",ip); 
       memcpy(connectip , ip , strlen((char *)ip));
    }
    
    
    else
    {
        log(INFO,"��ַ������\r\n"); 
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
                log(INFO,"�Ѿ���������������� , socket id = %d\n" , id);
                return SOCKET_OK;
            }
            else
            {
                log(WARN,"1ģ�鷵�����ӷ�����ʧ�� , socket id = %d\n" , id);
                return SOCKET_CONNECT_FAIL;
            }
        }
        else if( strstr(respone.rxBuff , "OK") != NULL)
        {
            rId = respone.rxBuff[0] - '0';

            if( rId == id )
            {
                log(INFO,"���ӷ������ɹ� , socket id = %d\n" , id);
                return SOCKET_OK;
            }
        }
        else if( xSemaphoreTake( xSocketConnectSemaphore, 30000 ) == pdTRUE )
        {
            if ( receiveSocketConnectStatus == TRUE)
            {
                log(INFO,"���ӷ������ɹ� , socket id = %d\n" , id);
                return SOCKET_OK;
            }
            else
            {
                log(WARN,"2ģ�鷵�����ӷ�����ʧ�� , socket id = %d\n" , id);
                return SOCKET_CONNECT_FAIL;
            }
        }
        log(WARN,"���������Ӻ������յ�����, DATA = %s\n" , respone.rxBuff);
        wifi_rx_data_analisys(respone.rxBuff , respone.len);
        memset( &respone ,0x00 ,  sizeof(usartReceiveDataType));
        return SOCKET_CONNECT_TIMEOUT;
    }
    else
    {
        log(WARN,"WIFI�������ӷ�����û����Ӧ�����ڽ��������⣬��λģ��");
        return SOCKET_CONNECT_NORES;
    }
 
}

int8_t esp12s_send( uint8_t id , uint8_t *sendData , uint16_t length )
{
    uint8_t comm[64] , cmdLength =0 ;
    
	//log_arry(DEBUG,"esp12s ��������," , sendData , length);
	
    sendStartStatus = TRUE;
    memset(comm ,0x00 , sizeof(comm));
	cmdLength = sprintf((char *)comm , "AT+CIPSEND=%d,%d\r\n" , id ,length);
    wifi_send_data(comm , cmdLength);
    
    if( xSemaphoreTake( xSocketCmdSemaphore, 30000 ) == pdTRUE )
    {    	
        if( sendStartStatus == FALSE )
        {            
            log(WARN,"scoket %d �������ݷ��ش���\n" , id);
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
               // log(DEBUG,"ESP12 �������\n");
        		return SOCKET_OK;
        	}
        	sendStartStatus = FALSE;
        	 log(WARN,"scoket %d �������ݷ���ʧ��\n" , id);
        	return SOCKET_SEND_FAIL;
        }
        sendStartStatus = FALSE;
        log(WARN,"scoket %d ���������޽��ճɹ�����\n" , id);
        return SOCKET_OK;//SOCKET_SEND_NORESULT;
    }
    sendStartStatus = FALSE;
    log(WARN,"scoket %d ����������׼������\n" , id);
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

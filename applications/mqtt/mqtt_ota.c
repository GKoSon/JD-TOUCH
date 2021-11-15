#include "net.h"
#include "usart.h"
#include "config.h"
#include "string.h"
#include "unit.h"
#include "socket.h"
#include "mqtt_ota.h"
#include "sim800c.h"
#include "stdlib.h"
#include "spi_flash.h"
#include "crc32.h"
#include "sysCfg.h"
#include "tsl_mqtt.h"

typedef enum
{
    CHECH_UPG_FILE,
    DOWMLOAD_FILE,
    SYS_UPG_MODE,
}otaStatusEnum;


typedef struct
{
    otaStatusEnum	otaStatus;
    uint32_t		len;
    uint32_t		ver;
    uint32_t		fileSize;
    uint32_t		crc32;
}otaRecvCmdType;

otaRecvCmdType	ota;

typedef struct
{
	uint8_t cnt;
	uint8_t	calccnt;
	int	binlen;
	int	L;
	int	R;
}_ota;

_ota otamark;

void showota(_ota *p)
{

    log_err("p->cnt     %d\n",p->cnt);
    log_err("p->calccnt %d\n",p->calccnt);
    log_err("p->binlen  %d\n",p->binlen);
    log_err("p->L       %d\n",p->L);
    log_err("p->R       %d\n",p->R);
}
char 			        rxOtaData[2048];

static xTaskHandle		otaTask;
static runStatusEnum	        otaRunStatus = RUN_INIT;
static int8_t			clientId =0;


/////////////////////////////////////////////
//�ļ�����


static uint8_t dev_ota_read_flash(uint32_t addr,uint8_t* buffer,  uint16_t length)
{
	if( flash.get_lock() == TRUE )
	{
		flash.read(addr , buffer , length);

		flash.release_lock();
	}

	 return TRUE;
}

static uint8_t dev_ota_write_flash(uint32_t addr,uint8_t* buffer,  uint16_t length)
{
	if( flash.get_lock() == TRUE )
	{
		flash.write(addr , buffer , length);

		flash.release_lock();
	}

	return TRUE;

}


static uint8_t dev_ota_erase_flash(uint32_t sectorAddr)
{
	if( flash.get_lock() == TRUE )
	{
		flash.earse(sectorAddr);

		flash.release_lock();
	}

	return TRUE;

}

uint8_t ota_write_file(uint8_t *msg , uint32_t len)
{
    
    uint32_t writeAddr =   ota.len + OTA_START_ADDR;
    
    if( writeAddr%FLASH_SPI_BLOCKSIZE == 0)
    {
            dev_ota_erase_flash(writeAddr);
    }

    printf("ͷ0X%02X--β0X%02X---��%d",msg[0],msg[len-1],len);
    dev_ota_write_flash(writeAddr , msg , len);

    return TRUE;
}


uint8_t ota_ver_file( void )
{
	uint32_t crc32 = 0xFFFFFFFF;
	uint32_t crcTbl;
	uint8_t  buff[512];
	uint32_t readAddr = OTA_START_ADDR;
	uint16_t readSize = 512 ;
    __IO int32_t len =0;

	while( len < ota.fileSize )
	{
		if( len + readSize > ota.fileSize)
		{
			readSize = ota.fileSize - len;
		}

		memset(buff , 0x00 , 512);
		dev_ota_read_flash(readAddr+len , buff , readSize);

		for (uint32_t i = 0; i!= readSize; ++i)
		{
			crcTbl = (crc32 ^ buff[i]) & 0xFF;
			crc32 = ((crc32 >> 8) & 0xFFFFFF) ^ gdwCrc32Table[crcTbl];
		}
		len += readSize;

	}

	crc32 = ~crc32;

        log(ERR,"�ļ�У��-�Լ������CRC=%x , ƽ̨����crc = %x  ǿ����ֵ\n" , crc32 , ota.crc32);

        ota.crc32 = crc32;

        return TRUE;

}



///////////////////////////////////////////
//�������

void ota_set_init( void )
{
    SHOWME
    otaRunStatus = RUN_INIT;
    clientId = -1;
    socket.close();
    sys_delay(500);
}

void ota_repert_connect( void )
{
    socket.disconnect(clientId);
    otaRunStatus = RUN_CONNECTING;
    clientId = -1;
    socket.close();
    sys_delay(500);
}


void ota_clear_buffer( void )
{
       memset(&otamark,0,sizeof(_ota));
       memset(&ota,0,sizeof(otaRecvCmdType));
}

void ota_init_buffer( void )
{
        ota.otaStatus = CHECH_UPG_FILE;
        memset(&otamark,0,sizeof(_ota));
        memset(&ota,0,sizeof(otaRecvCmdType));
        
}



/*
 * 200���ɹ��� �������ѳɹ����������� ͨ�������ʾ�������ṩ���������ҳ�� ���������� robots.txt �ļ���ʾ��״̬���룬���ʾ Googlebot �ѳɹ����������ļ���
 * 201���Ѵ����� ����ɹ����ҷ������������µ���Դ��
 * 202���ѽ���,��δ���� �������ѽ������󣬵���δ����
 * 203������Ȩ��Ϣ�� �������ѳɹ����������󣬵����ص���Ϣ����������һ��Դ��
 * 204�������ݣ� �������ɹ����������󣬵�û�з����κ����ݡ�
 * 205���������ݣ� �������ɹ����������󣬵�û�з����κ����ݡ� �� 204 ��Ӧ��ͬ������ӦҪ�������������ĵ���ͼ�����磬��������������������ݣ���
 * 206���������ݣ� �������ɹ������˲��� GET ���� ������ FlashGet ����Ѹ������� HTTP ���ع��߶���ʹ�ô�����Ӧʵ�ֶϵ��������߽�һ�����ĵ��ֽ�Ϊ������ض�ͬʱ���ء�
*/
int ota_check_http(char *msg)
{
  
    char *target = "\r\n\r\n";
    char *pst = NULL;
    int len = 0;


    if(0== (aiot_strcmp((uint8_t *)msg , "HTTP/1.1 20" , strlen("HTTP/1.1 20"))))
    {printf("HTTPͷ����\r\n");return 0;}
    
    else
    {
         pst = strstr(msg, target);
         if(pst==NULL){printf("HTTPβ����\r\n");return 0;}
         
         *pst='\0';/*������ֹΪ\0\n\r\n*/
         
         len = strlen(msg)+4;//��Ҫ����\r\n\r\n Ҳ����4
         
         //printf("������ֹmsg���dellen=%d\r\n",len);
         
         return len;
    
    }
}



int read_file_length(char *msg)
{
  
/*  
HTTP/1.1 206 Partial Content
Date: Wed, 07 Aug 2019 07:23:59 GMT
Server: Apache/2.4.23 (Win32) OpenSSL/1.0.2h PHP/5.6.24
Last-Modified: Wed, 07 Aug 2019 07:09:33 GMT
ETag: "276b4-58f819f81bbff"
Accept-Ranges: bytes
Content-Length: 4
Content-Range: bytes 0-3/161460
Content-Type: application/octet-stream
*/
    char *pst = NULL , lenBuff[10]={0} , i =0;
	int len = 0;
    

	pst = strstr(msg, "Content-Range: bytes 0-3/");
	if (pst != NULL)
	{
		pst += strlen("Content-Range: bytes 0-3/");

		while (*pst != '\r')
		{
			lenBuff[i++] = *pst++;

			if(i>10) {log(WARN,"��ȡ�������쳣\n");return len;}

		}
		len = atoll(lenBuff);
	}
	else
	{
		log(WARN,"û�л�ȡ������\n");
	}

	log(DEBUG,"read_file_length���س��� =  %d \n", len);

	return len;
}



int8_t ota_check_socket( void )
{
    int timeout = 5000;

    while(socket.isOK() != TRUE)
    {
        if(timeout-- == 0) break;
        sys_delay(10);
    }
    if( timeout < 0 )
    {
        log(WARN,"socket����δ�������\n");
        return FALSE;
    }

    sys_delay(5000);
    return TRUE;
}




uint8_t ota_wait_data(uint8_t *pData , uint16_t len)
{
    uint16_t recvLen = 0 , timeout=100;
    int ret =0;
    
    while((recvLen < len)&&(timeout--))
    {
        
        ret = socket.read(clientId  , 10000);
        if (ret > 0)
        {
            memcpy(pData+recvLen , rxOtaData , ret);
            recvLen += ret;
        }
        else
        {
            log(WARN,"OTA�������ݴ���\n");
            return FALSE;
        }
        
    }
    
    if( recvLen == len)
    {
        return TRUE;
    }
    log(WARN,"OTA�������ݳ�ʱ\n");
    return FALSE;
}

uint16_t packsend(char *buf,char*url,char*ip,uint16_t port,int L,int R)
{
  return sprintf(buf,"GET %s HTTP/1.1\r\nHost:%s:%d\r\nRange: bytes=%d-%d\r\n\r\n",url,ip,port,L,R);
}


int8_t ota_download_read_file(void)
{
  
#define ONESTEP 1024
        uint8_t  request[256];
        
        int sendLen = 0, ret = 0,dellen=0,jump=0;
      
        char *url =NULL;
    
        serverAddrType *addr;    
        
          
        uint16_t port = (uint16_t)config.read( CFG_OTA_PORT ,NULL);
          
        config.read(CFG_NET_ADDR , (void **)&addr);
     
        config.read(CFG_OTA_URL , (void **)&url);
        
        showota(&otamark);  
        
        
        if(otamark.cnt==0)
        {
          
          memset(rxOtaData, 0x00, sizeof(rxOtaData));   socket_clear_buffer(clientId);
          
          memset(request, 0x00, sizeof(request));

          //sendLen = sprintf((char *)request, "GET /upload/687775290.bin HTTP/1.1\r\nHost:ibinhub.com\r\nRange: bytes=0-3\r\n\r\n");
          sendLen = packsend((char *)request,url,(char*)addr->ip,port,0,3);
          
          ret = socket.send(clientId , request , sendLen , 3000);

          if( ret != SOCKET_OK)   {printf(" socket.send ERR\r\n"); return ret;  }

        }  

	while (1)
	{
                if(W5500ERR)
                {
                      log(ERR,"#####W5500ERR###### = %d######W5500ERR#######\n" , clientId);
                      W5500ERR = 0;
                      return 44;
                }

                ret = 0 ;

                ret = socket.read(clientId  , 10000);
                              

                if (ret > 0)
                {
                  
                        printf("\r\n�ӵ���Ϣ�ĳ���[%d] otamark.cnt=[%d]-----------\r\n",ret,otamark.cnt);
                        
                        otamark.cnt++;
                                               
                        //if(otasee)log(ERR,"ota recv =%.533s \n" , rxOtaData);
                        
                        if(otamark.cnt==1)
                        {
                          
                            printf("��һ��TXRX��ʼ����û�б��桿\r\n");
                            
                            log(DEBUG,"ota recv =%s \n" , rxOtaData);
                            
                            if( dellen = ota_check_http(rxOtaData) == 0){ log(WARN,"����HTTP����ͷ����\n");break;}
                            
                            printf("��ʵ����dellenҲ������ȫ��-4������ret-4=%d ��������dellenû�������������仯��\r\n",ret-4);
                            
                            if((otamark.binlen = read_file_length(rxOtaData))==0) { log(WARN,"����binlen����\n"); break; }
                            
                            otamark.calccnt = otamark.binlen%ONESTEP ? (otamark.binlen/ONESTEP+1) :otamark.binlen/ONESTEP;
                            otamark.L = 0;
                            otamark.R = 1023;
                            //sendLen = sprintf((char *)request, "GET /upload/687775290.bin HTTP/1.1\r\nHost:ibinhub.com\r\nRange: bytes=%d-%d\r\n\r\n",otamark.L,otamark.R);
                            sendLen = packsend((char *)request,url,(char*)addr->ip,port,otamark.L,otamark.R);
                            log(INFO,"��%d��[%s]",sendLen,request);
                            showota(&otamark); 
                           // memset(rxOtaData, 0x00, sizeof(rxOtaData));   socket_clear_buffer(clientId);
                            ret = socket.send(clientId , request , sendLen , 3000);    
                            printf("��һ��TXRX�Ѿ������׵� \r\n");
                            
                        }
                        
                        else if(otamark.cnt==otamark.calccnt+1)
                        {
                          
                            log(ERR,"���һ��TXRX�������������һ����\r\n");
                            dellen = ota_check_http(rxOtaData);
                            printf("dellen =%d ret=%d\r\n",dellen,ret);
                             if(otasee)log(ERR,"ota recv =%s \n" , rxOtaData);
                             if(otasee)log_arry(ERR,"[OTA recv]" , (uint8_t *)&rxOtaData[dellen] ,  ret-dellen);
                            ota_write_file((uint8_t *)&rxOtaData[dellen],        ret-dellen);
                            
                            ota.fileSize=otamark.binlen;
                              
                            return 0;
                        }
                        else
                       {
                                dellen = ota_check_http(rxOtaData);
                                
                                printf("dellen =%d ret=%d\r\n",dellen,ret);
                                
                                if (dellen + ONESTEP  == ret) 
                                {

                                  ota_write_file((uint8_t *)&rxOtaData[dellen], ONESTEP);

                                  ota.len  +=  ONESTEP;
                                  otamark.L += ONESTEP;
                                  otamark.R += ONESTEP;
                                  ota_log(ERR,"���������桿\n");
                                  if(otasee)log(INFO,"ota recv =%s \n" , rxOtaData);
                                  if(otasee)log_arry(ERR,"[OTA recv]" , (uint8_t *)&rxOtaData[dellen] , ONESTEP);
                                                            
                                  sys_delay(100);
                                  //sendLen = sprintf((char *)request, "GET /upload/687775290.bin HTTP/1.1\r\nHost:ibinhub.com\r\nRange: bytes=%d-%d\r\n\r\n",otamark.L,otamark.R);
                                  sendLen = packsend((char *)request,url,(char*)addr->ip,port,otamark.L,otamark.R);
                                  log(INFO,"��%d��[%s]",sendLen,request);
                                  memset(rxOtaData, 0x00, sizeof(rxOtaData));   socket_clear_buffer(clientId);
                                  ret = socket.send(clientId , request , sendLen , 4000);
                                }
                                else
                                {
                                     
                                  log(ERR,"�������� �����桿\n");SHOWME SHOWME SHOWME
                                  if(otasee)log(ERR,"ota recv =%s \n" , rxOtaData);
                                  if(otasee)log_arry(ERR,"[OTA recv]" , (uint8_t *)&rxOtaData[dellen] , ONESTEP);
                                                            
                                  sys_delay(100);
                                  memset(rxOtaData, 0x00, sizeof(rxOtaData));   socket_clear_buffer(clientId);
								  sendLen = packsend((char *)request,url,(char*)addr->ip,port,otamark.L,otamark.R);
                                  log(INFO,"��%d��[%s]",sendLen,request);
                                  memset(rxOtaData, 0x00, sizeof(rxOtaData));   socket_clear_buffer(clientId);
                                  ret = socket.send(clientId , request , sendLen , 4000);
                                }  
                                              
                        }//else xiao
		}//if
                else
                {
                    ++jump;
                    if(jump==2)
                    {
                                  log(INFO,"���ٴν��� ��ȥ����һ�¡�\r\n");
                                  //sendLen = sprintf((char *)request, "GET /upload/687775290.bin HTTP/1.1\r\nHost:ibinhub.com\r\nConnection: keep-alive\r\nRange: bytes=%d-%d\r\n\r\n",otamark.L,otamark.R);
                                  sendLen = packsend((char *)request,url,(char*)addr->ip,port,otamark.L,otamark.R);
                                  log(INFO,"��%d��[%s]",sendLen,request);
                                  memset(rxOtaData, 0x00, sizeof(rxOtaData));   socket_clear_buffer(clientId);
                                  ret = socket.send(clientId , request , sendLen , 3000);
                    
                    }
                    if(jump==4)
                    {
                      log(INFO,"��û�л�����\r\n"); soft_system_resert(__FUNCTION__);
                      return 44;
                    }
                }

	}//while
        
      log(ERR,"NEVER COM\n");
      return 44;
}


int8_t ota_download_file( void )
{
   int ret = -1;
    
    switch(otaRunStatus)
    {
        case RUN_INIT:
        {
            sys_delay(500);
            if( socket.isOK() == TRUE)
            {
            	otaRunStatus = RUN_CONNECTING;
            }
        }break;
        case RUN_CONNECTING:
        {
            log(ERR,"OTA start connect server\n");
            memset(rxOtaData , 0x00 , sizeof(rxOtaData));

            
             serverAddrType *addr;         
             config.read(CFG_NET_ADDR , (void **)&addr);


             if( (ret = socket.connect(addr->ip , (uint16_t)config.read( CFG_OTA_PORT ,NULL),rxOtaData , sizeof(rxOtaData))) >= 0)
            {
                clientId = ret;
                log(DEBUG,"ota connect server success , client id = %d\n" , clientId);
                otaRunStatus = RUN_CONNECT;
            }
            else
            {
                ota_set_init();
            }

        }break;
        case RUN_CONNECT:
        {
            if( (ret = ota_download_read_file()) == SOCKET_OK )
            {
                ota_ver_file();
                
            	otaType *otaCfg;
                int ver;

                log(INFO,"�ļ����سɹ� ��ʼ����CFG��\n");
                config.read(CFG_OTA_CONFIG , (void **)&otaCfg); 

                otaCfg->crc32 = ota.crc32;
                otaCfg->fileSize = ota.fileSize;
                otaCfg->otaUpgMark = UPG_MARK;

                ver = otaCfg->ver;
                config.write(CFG_SYS_SW_VERSION ,  &ver,1); 
                
                log(INFO,"�ļ����سɹ������Խ�������ver=%d\n",ver);

                return OTA_OK;
            }
            else 
            {
                log(INFO,"�ļ�����ʧ�ܣ��ٴ�����\n");

            	ota_repert_connect();
            }
        }break;
        default:break;
    }

    return -1;
}



static void ota_task( void const *pvParameters)
{

   ota_init_buffer();
   

    for( ; ; )
    {
        switch(ota.otaStatus)
        {
            case CHECH_UPG_FILE:
        	{
        		
                    if( xSemaphoreTake( xMqttOtaSemaphore, portMAX_DELAY ) == pdTRUE )
                    {
                      printf("-----------׼������------------\r\n"); 

                      ota.otaStatus = DOWMLOAD_FILE;
                    }
        	}break;  
            case DOWMLOAD_FILE:
            {
                 if( ota_download_file() == OTA_OK)
                 {
                    ota.otaStatus = SYS_UPG_MODE;
                 }
            }break;
            case SYS_UPG_MODE:
            {
               SHOWME; SHOWME; SHOWME; SHOWME; SHOWME; SHOWME; SHOWME; SHOWME; SHOWME; SHOWME; SHOWME; SHOWME; 
               ota.otaStatus = CHECH_UPG_FILE;
               soft_system_resert(__func__);
            }break;
            default:break;
        }
        sys_delay(1000);
    }
}



void creat_mqtt_ota_task( void )
{
    osThreadDef( ota, ota_task , osPriorityLow, 0, configMINIMAL_STACK_SIZE*15);
    otaTask = osThreadCreate(osThread(ota), NULL);
    configASSERT(otaTask);
}


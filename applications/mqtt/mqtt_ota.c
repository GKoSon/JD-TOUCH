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


extern char 			rxOtaData[2048];
otaRecvCmdType	ota;
static xTaskHandle		otaTask;
static runStatusEnum	otaRunStatus = RUN_INIT;
static int8_t			clientId =0;

static __IO uint8_t     otaStartUpg = FALSE;
static uint8_t			assertCnt = 0;

uint8_t assert_len(uint8_t *name , uint8_t len)
{
	if( ++assertCnt > len )
	{
		log(WARN,"%s cnt is too big , max len = %d\n" , name , len);
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

void ota_start_upg(void)
{
    if(ota.otaStatus == SYS_UPG_MODE)
    {
        otaStartUpg = TRUE;
    }
}
/////////////////////////////////////////////
//文件操作


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

	dev_ota_write_flash(writeAddr , msg , len);

    return TRUE;
}


void printf_file(void)
{

	uint8_t buff[512];
	uint32_t readAddr = OTA_START_ADDR;
	uint16_t readSize = 12 ;
    __IO int32_t len =80;

	while( len < ota.fileSize )
	{
		if( len + readSize > ota.fileSize)
		{
			readSize = ota.fileSize - len;
		}

		memset(buff , 0x00 , 512);
		dev_ota_read_flash(readAddr+len , buff , readSize);

        for(uint16_t i =0 ; i < readSize ; i++)
        {
            printf("%02X " , buff[i]);
        }
        printf("\r\n");
		len += readSize;
        //sys_delay(100);

	}

}


uint8_t ota_ver_file( void )
{
	uint32_t crc32 = 0xFFFFFFFF;
	uint32_t crcTbl;
	uint8_t buff[512];
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

	if( crc32 == ota.crc32)
	{
		log(INFO,"文件校验正确，calc CRC=%x , get crc = %x\n" , crc32 , ota.crc32);
		return TRUE;
	}

	log_err("文件校验失败，calc CRC=%x , get crc = %x\n" , crc32 , ota.crc32);

    /*
    log_err("读取错误文件信息\n");
    printf_file();
    */
    
	return FALSE;
}



///////////////////////////////////////////
//网络操作

void ota_set_init( void )
{
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
    sys_delay(500);
}


void ota_clear_buffer( void )
{
	ota.crc32 = 0;
	ota.fileSize = 0;
	ota.len = 0;
	ota.ver = 0;
	ota.upgFlag = 0;
	memset( ota.fileKey , 0x00 , sizeof(ota.fileKey));
	ota.writeAddr  =  OTA_START_ADDR;
}

void ota_init_buffer( void )
{
	otaType *otaCfg;

	memset(&ota , 0x0 , sizeof(otaRecvCmdType));
	ota.otaStatus = CHECH_UPG_FILE;
	ota.writeAddr  =  OTA_START_ADDR;
	config.read(CFG_OTA_CONFIG,(void **)&otaCfg);
	if( otaCfg->otaUpgMark == UPG_MARK)
	{
		ota.otaStatus = SYS_UPG_MODE;
	}
}

/*
 * 200（成功） 服务器已成功处理了请求。 通常，这表示服务器提供了请求的网页。 如果针对您的 robots.txt 文件显示此状态代码，则表示 Googlebot 已成功检索到该文件。
 * 201（已创建） 请求成功并且服务器创建了新的资源。
 * 202（已接受,但未处理） 服务器已接受请求，但尚未处理。
 * 203（非授权信息） 服务器已成功处理了请求，但返回的信息可能来自另一来源。
 * 204（无内容） 服务器成功处理了请求，但没有返回任何内容。
 * 205（重置内容） 服务器成功处理了请求，但没有返回任何内容。 与 204 响应不同，此响应要求请求者重置文档视图（例如，清除表单内容以输入新内容）。
 * 206（部分内容） 服务器成功处理了部分 GET 请求。 类似于 FlashGet 或者迅雷这类的 HTTP 下载工具都是使用此类响应实现断点续传或者将一个大文档分解为多个下载段同时下载。
*/
uint8_t ota_check_http(char *msg)
{
    return (aiot_strcmp((uint8_t *)msg , "HTTP/1.1 2" , strlen("HTTP/1.1 2")));
}

void ota_write_data(uint8_t *msg , uint16_t length)
{
	//int8_t ret = SOCKET_OK;

    if( otaRunStatus == RUN_CONNECT)
    {
    	sys_delay(300);
    	socket.send(clientId , msg , length , 3000);
        ota_repert_connect();
    }
    else
    {
       log_err("OTA网络未链接\n");
       
    }
}



uint16_t ota_upg_puck(uint8_t *msg)
{
	uint16_t boadyLen =0 , headLen = 0;
	char projectMsg[256] ;
	uint8_t sw = DEVICE_SW_VERSION;
   // uint8_t prodectNum[10]=PRODUCT_NUM;
    uint8_t *deviceModel;
    uint8_t devicePn[10]=DEVICE_PN;

	memset(projectMsg , 0x00 , 256);

	//config.read(CFG_SYS_DEVICE_MODULE , (void **)&deviceModel);
	//boadyLen = sprintf(projectMsg, "Product=%s&Model=%s&Manufactory=STM&DeviceType=%d&DevicePn=%s&DeviceIndex=0&VersionApp=%d.%d.%d",
			//prodectNum,deviceModel ,6 ,devicePn , (sw/100) , (sw/10)%10 ,(sw%10));


	headLen = sprintf((char *)msg , "POST /Domain/Common/Fw/Check HTTP/1.1\nContent-Type: application/x-www-form-urlencoded\nHost: api.tslsmart.com:80\nContent-Length:%d\nConnection: keep-alive\n\n" , boadyLen);

    strcat((char *)msg , projectMsg);

    log(DEBUG,"len = %d , post message = %s\n" , boadyLen+headLen,msg);

    return (boadyLen+headLen);

}

int read_file_length(char *msg)
{
	char *pst = NULL , lenBuff[10] , i =0;
	int len = 0;
    

	pst = strstr(msg, "Content-Length:");
	if (pst != NULL)
	{
		pst += strlen("Content-Length:");
		assertCnt = 0;
		while (*pst != '\n')
		{
			lenBuff[i++] = *pst++;

			if( assert_len("ota read length " , 8))	return len;

		}
		len = atoll(lenBuff);
	}
	else
	{
		log(WARN,"没有获取到长度\n");
	}

	//log(DEBUG,"返回长度 =  %d \n", len);

	return len;
}

int8_t ota_upg_analysis(char *str, int len)
{
	char lenBuff[20] , crcBuff[20],  i =0 , version[10];
	char *pst = NULL;


	memset(lenBuff, 0x00, 20);
	memset(crcBuff, 0x00, 20);
	memset(version, 0x00, 10);

	//获取文件长度
	if ((pst = strstr(str, "Size\":\"")) == NULL)
	{
		log(WARN,"Not find size\n");
		return FALSE;
	}
	pst = pst + strlen("Size\":\"");

	assertCnt = 0;
	while (*pst != '\"')
	{
		lenBuff[i++] = *pst++;
		if( assert_len("read file size" , 10))	return FALSE;
	}

	ota.fileSize = atoi(lenBuff);

	//获取CRC32
	if ((pst = strstr(str, "CRC32\":\"")) == NULL)
	{
		return FALSE;
	}
	pst = pst + strlen("CRC32\":\"");

	assertCnt = 0;
	i = 0;
	while (*pst != '\"')
	{
		crcBuff[i++] = *pst++;
		if( assert_len("read crc32" , 20))	return FALSE;
	}
	ota.crc32 = atoi(crcBuff);

	//获取版本号
	if ((pst = strstr(str, "\"Version\":\"")) == NULL)
	{
		return FALSE;
	}
	pst = pst + strlen("\"Version\":\"");

	assertCnt = 0;
	i = 0;
	while (*pst != '\"')
	{
		version[i++] = *pst++;
		if( assert_len("read version" , 10))	return FALSE;
	}
	ota.ver = ((version[0] - '0') * 100) + ((version[2] - '0') * 10) + (version[4] - '0');


	//获取下载文件名
	if ((pst = strstr(str, "\"FileKey\":\"")) == NULL)
	{
		log(WARN,"Not find filename\n");
		return FALSE;
	}
	pst = pst + strlen("\"FileKey\":\"");

	assertCnt = 0;
	i = 0;
	while (*pst != '\"')
	{
		ota.fileKey[i++] = *pst++;
		if( assert_len("read fileKey" , 50))	return FALSE;
	}


    return TRUE;

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
        log(WARN,"socket驱动未加载完成\n");
        return FALSE;
    }

    sys_delay(5000);
    return TRUE;
}



int8_t ota_check_upg( void )
{
    uint8_t msg[1024] ;
    uint16_t len =0 ;
    int ret = 0;
    int8_t upgId = -1;
    serverAddrType *addr;
    int8_t	result = OTA_UPG_INIT;

    memset(msg , 0x00 , 1024);
    memset(rxOtaData , 0x00 , sizeof(rxOtaData));

   // config.read(CFG_UPG_ADDR , (void **)&addr);
    if( ota_check_socket() == FALSE)
    {
    	return OTA_UPG_INIT;
    }
    log(DEBUG,"upg start connect server:%s:%d\n" , addr->ip , addr->port);
    if( (upgId = socket.connect(addr->ip, addr->port , (char *)rxOtaData , sizeof(rxOtaData))) < 0)
    {
        return OTA_UPG_CONNECTERR;
    }
    log(DEBUG,"connect server success , client id = %d\n" , upgId);

    len = ota_upg_puck(msg);

    if( len > 0)
    {
        if( socket.send(upgId , msg , len , 10000) != SOCKET_OK)
        {
        	result =  OTA_UPG_SENDERR;
        	goto end;
        }
    }
    else
    {
    	return OTA_UPG_PUCKERR;
    }
    ota_clear_buffer();
    ret = socket.read(upgId  , 10000);
    if( ret > 0)
    {
        log(DEBUG,"Return len = %d , data = %s \n" , ret , rxOtaData);
        if( ota_check_http(rxOtaData) == FALSE)
        {
            log(WARN,"返回HTTP数据头错误\n");
            result =  OTA_UPG_DATA_ERR;
        	goto end;
            
        }
        if( ota_upg_analysis( (char *)rxOtaData , ret) == FALSE)
        {
        	result =  OTA_UPG_ANALYSISERR;
        	goto end;
        }
    }
    else
    {
    	result =  OTA_UPG_READERR;
    	goto end;
    }

    if( ota.ver <= DEVICE_SW_VERSION)
    {
    	log(WARN , "软件版本号小于等于目前版本号不需要升级  , ver = %d\n" , ota.ver);
    	result =  OTA_UPG_NOVER;
    	goto end;
    }

    log(INFO ,"检测到升级文件 file size = %d , crc32 = %u , version = %d\n", ota.fileSize, ota.crc32 , ota.ver);
    log(INFO ,"检测到升级文件 file name  %s\n", ota.fileKey);
    result = OTA_OK;
end:
    socket.disconnect(upgId);
    upgId = -1;
    return result;
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
            log(WARN,"OTA接收数据错误\n");
            return FALSE;
        }
        
    }
    
    if( recvLen == len)
    {
        return TRUE;
    }
    log(WARN,"OTA接收数据超时\n");
    return FALSE;
}
int8_t ota_download_read_file(void)
{
	uint8_t  request[256];
	int sendLen = 0, ret = 0,len = 0,heardlen = 0 ,dataLen=0;
	char *dataPoint = NULL;
    serverAddrType *addr;
	//uint8_t netType = config.read(CFG_SYS_NET_TYPES,NULL);

    //config.read(CFG_OTA_ADDR , (void **)&addr);
    
	while (ota.len  < ota.fileSize)
	{

        ret = 0 ;
		len = 0 ;
		heardlen = 0;
        dataLen = 0;
		dataPoint = NULL;
        memset(request, 0x00, sizeof(request));
        memset(rxOtaData, 0x00, sizeof(rxOtaData));
        socket_clear_buffer(clientId);

        log(DEBUG,"文件下载进度:%d%\n" ,  (ota.len*100)/ota.fileSize);
        
		if (ota.len  + 512 >= ota.fileSize)
		{
			ota_log(DEBUG,"获取数据, %d-%d \n" , ota.len , ota.fileSize);
			sendLen = sprintf((char *)request, "GET %s HTTP/1.1\r\nHost: %s\r\nConnection: keep-alive\r\nRange: bytes=%d-%d\r\n\r\n", ota.fileKey, addr->ip, ota.len , ota.fileSize);
            dataLen = ota.fileSize - ota.len;
        }
		else
		{
			ota_log(DEBUG,"获取数据, %d-%d \n" , ota.len ,  ota.len  + 511);
			sendLen = sprintf((char *)request, "GET %s HTTP/1.1\r\nHost: %s\r\nConnection: keep-alive\r\nRange: bytes=%d-%d\r\n\r\n", ota.fileKey, addr->ip ,ota.len ,ota.len  + 511);
            dataLen = 512;
        }

		ret = socket.send(clientId , request , sendLen , 3000);
		if( ret != SOCKET_OK)
		{
			return ret;
		}
        
		ret = socket.read(clientId  , 10000);
		ota_log(DEBUG,"接到消息的长度:  %d\n", ret);
		if (ret > 0)
		{
			//log(DEBUG,"ota recv =%s \n" , rxOtaData);
            if( ota_check_http(rxOtaData) == FALSE)
            {
                log(WARN,"返回HTTP数据头错误\n");
                continue;
                
            }
			//解析数据长度
			len  = read_file_length(rxOtaData);
			ota_log(DEBUG,"解析HTTP头，数据长度:%d\n", len);
			if(len == 0)
			{
				ota_log(WARN,"数据长度为0 ，重新读取 , 当前接受长度=%d\n" , ota.len);
				continue;
			}

			//解析Http协议头，获取头的长度及数据起始地址
			dataPoint = strstr(rxOtaData , "\r\n\r\n");
			if(dataPoint == NULL)
			{
				log_err("heard data is error\n");
				return SOCKER_READ_ERR;
			}
			dataPoint += strlen("\r\n\r\n");
			heardlen = (char *)dataPoint - (char*)rxOtaData;

            if( (heardlen + len == ret)&&(len == dataLen) )
            {
                ota_log(DEBUG,"接收包中包含数据，heard len =%d , len = %d , datalen = %d\n" , heardlen , len , dataLen);
                ota_write_file((uint8_t *)(dataPoint), len);

				ota.len  += len;
				ota_log(DEBUG,"接收到总长度:%d\n", ota.len );
            }
            else
            {
                
                uint8_t dBuff[1024];
                uint16_t buffTemLen = 0 ,lastLen =0;
                
                ota_log(DEBUG,"接收数据中不包含完整的头+数据，继续接收数据，heard len =%d , len = %d , datalen = %d\n" , heardlen , len , dataLen);
                  
                memset(dBuff , 0x00 , 1024);
                
                buffTemLen = ret - heardlen;
                lastLen = dataLen-buffTemLen;
                ota_log(DEBUG,"当前已接收数据长度=%d , 剩余长度=%d\n" , buffTemLen , lastLen);
                memcpy(dBuff , dataPoint , buffTemLen);
                
                if( ota_wait_data(dBuff+buffTemLen ,  lastLen) ==TRUE)
                {
                    ota_log(DEBUG,"数据接收完成\n");
                    ota_write_file((uint8_t *)dBuff,dataLen);
					ota.len  += dataLen;
					ota_log(DEBUG,"接收到总长度:%d\n", ota.len );
                }
            }           
		}
		else
		{
			ota_log(DEBUG,"OTA 读取数据失败 ，ret = %d\n" , ret);
			return SOCKER_READ_ERR;
		}
		//if( netType != TSLNetType_TSLEthernet)
		//{
		//	sys_delay(1000);
		//}
	}

	printf("OTA接收到总长度 = %d \n", ota.len );
    
	if( ota.len ==  ota.fileSize)
	{
		log(INFO,"文件接收完成\n");
		if( ota_ver_file() == TRUE )
		{
			return SOCKET_OK;
		}
        else
        {
            socket.disconnect(clientId);
            ota_init_buffer();
        }
	}

	return OTA_DOWNLOAD_FAIL;
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
            log(DEBUG,"ota start connect server\n");
            memset(rxOtaData , 0x00 , sizeof(rxOtaData));
            if( (ret = socket.connect("sto.tslsmart.com" , 80 ,rxOtaData , sizeof(rxOtaData))) >= 0)
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
            	otaType otaCfg;

                log(INFO,"文件下载成功\n");

                otaCfg.ver = ota.ver;
                otaCfg.crc32 = ota.crc32;
                otaCfg.fileSize = ota.fileSize;
                otaCfg.otaUpgMark = UPG_MARK;
                config.write(CFG_OTA_CONFIG ,&otaCfg , TRUE);
                log(INFO,"文件下载成功，可以进行升级\n");

                return OTA_OK;
            }
            else
            {
                if( ota.len > 8192)
                {
                	 ota.len =   ( ota.len - ota.len%4096) - 4096;
                }
                else
                {
                	ota.len = 0;
                }

                log(WARN , "网络错误 ，回退部分字节， 重新下载开始位置=%d\n" , ota.len);
            	ota_repert_connect();
            }
        }break;
        default:break;
    }

    return OTA_DOWNLOAD_FAIL;
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
					if( ota.otaStatus != DOWMLOAD_FILE )
					{
						ota.otaStatus = DOWMLOAD_FILE;
					}
				}
        	}break;  
            case DOWMLOAD_FILE:
            {
                 if( ota_download_file() == OTA_OK)
                 {
                	 ota.otaStatus = SYS_UPG_MODE;
		//			 tsl_mqtt_return_ota_result(3 , ota.id , SERVICE_TYPE_COMMON , CMD_TYPE_UPGRADE_STATUS_REQUEST , SEQ_ID_NULL);
                 }
            }break;
            case SYS_UPG_MODE:
			{
                //if( otaStartUpg)
                {
                    //soft_system_resert(__func__);
                }
				//log(DEBUG,"等待升级\n");
				sys_delay(5000);
			}break;

            default:break;
        }

        //read_task_stack(__func__,otaTask);
        sys_delay(1000);
    }
}



void creat_mqtt_ota_task( void )
{
    osThreadDef( ota, ota_task , osPriorityLow, 0, configMINIMAL_STACK_SIZE*10);
    otaTask = osThreadCreate(osThread(ota), NULL);
    configASSERT(otaTask);
}


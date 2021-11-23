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


extern char              rxOtaData[2048];
static char              *rxBuf = rxOtaData;
#define                    OTARXBUF_SIZE    2048
otaRecvCmdType             ota;
static xTaskHandle        otaTask;
static runStatusEnum      otaRunStatus = RUN_INIT;
static int8_t             clientId =0;
static uint8_t            assertCnt = 0;



/*
*********************************** 独立打印开关 ***********************************
*/
#define OTA_DEBUG_LOG(type, message)                                           \
do                                                                            \
{                                                                             \
    if (type)                                                                 \
        printf message;                                                   \
}                                                                             \
while (0)
#define OTA_DEBUG                                                      1


/*
*********************************** 处理string的一些小函数 ***********************************
*/
static uint8_t assert_len(uint8_t *name , uint8_t len)
{
    if( ++assertCnt > len )
    {
        OTA_DEBUG_LOG(OTA_DEBUG, ("【OTA】%s  is too big , max len = %d\n" , name , len));
        return TRUE;
    }
    return FALSE;
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
static uint8_t ota_check_http(char *msg)
{
    return (aiot_strcmp((uint8_t *)msg , "HTTP/1.1 2" , strlen("HTTP/1.1 2")));
    //https://blog.csdn.net/weixin_42381351/article/details/121474307
}

/*
TX如下
GET /upload/193599818.bin HTTP/1.1
Host:ibinhub.com
Connection:keep-alive

RX如下
HTTP/1.1 206 Partial Content

Server: nginx

Date: Tue, 23 Nov 2021 03:07:49 GMT

Content-Type: application/octet-stream

Content-Length: 10

Last-Modified: Mon, 22 Nov 2021 08:20:33 GMT

Connection: keep-alive

ETag: "619b52d1-a"

X-Frame-Options: SAMEORIGIN

Content-Range: bytes 0-9/10



1234567890 

*/
static int analysis_file_length(char *msg)
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

            if( assert_len("analysis_file_length" , 8))    
              return len;

        }
        len = atoll(lenBuff);
    }
    else
    {
       OTA_DEBUG_LOG(OTA_DEBUG, ("【OTA】没有获取到长度特征Content-Length\n"));
    }

    OTA_DEBUG_LOG(OTA_DEBUG, ("【OTA】返回文件长度Content-Length =  %d \n", len));

    return len;
}

/*
*********************************** 文件操作 ***********************************
*/
static uint8_t dev_ota_read_flash(uint32_t addr,uint8_t* buffer,  uint16_t length)
{
    if( flash.get_lock() == TRUE )
    {
        flash.read(addr , buffer , length);

        flash.release_lock();

        return TRUE;
    }

     return FALSE;
}

static uint8_t dev_ota_write_flash(uint32_t addr,uint8_t* buffer,  uint16_t length)
{
    if( flash.get_lock() == TRUE )
    {
        flash.write(addr , buffer , length);

        flash.release_lock();
        
        return TRUE;
    }

     return FALSE;

}


static uint8_t dev_ota_erase_flash(uint32_t sectorAddr)
{
    if( flash.get_lock() == TRUE )
    {
        flash.earse(sectorAddr);

        flash.release_lock();
        
        return TRUE;
    }

     return FALSE;

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
/*
*********************************** 文件校验 和BOOT里面同名函数几乎一样 这里是读SPIFLASH 那里是读CHIPFLASH ***********************************
*/
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
        OTA_DEBUG_LOG(OTA_DEBUG, ("【OTA】文件校验正确,calc CRC=%x , get crc = %x\n" , crc32 , ota.crc32));
        return TRUE;
    }

    OTA_DEBUG_LOG(OTA_DEBUG, ("【OTA】文件校验失败,calc CRC=%x , get crc = %x\n" , crc32 , ota.crc32));

    /*
    log_err("读取错误文件信息\n");
    printf_file();
    */
    
    return FALSE;
}



/*
*********************************** 网络操作 ***********************************
*/
static void ota_set_init( void )
{
    otaRunStatus = RUN_INIT;
    clientId = -1;
    socket.close();
}

static void ota_repert_connect( void )
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
        NEVERSHOW
          NEVERSHOW
            NEVERSHOW
          
    }
    
    
    
    
memcpy(ota.fileKey,"/upload/193599818.bin" ,strlen("/upload/193599818.bin")  );
//memcpy(ota.fileKey,"/upload/485982176.bin" ,strlen("/upload/485982176.bin")  );
ota.len=       0;
//ota.fileSize=  51484;
ota.fileSize=  10;
show_OTA(otaCfg);
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
            memcpy(pData+recvLen , rxBuf  , ret);
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
#define ONESTEP 1024
    uint8_t  httprequest[256];
    int httpsendLen = 0, ret = 0,len = 0,heardlen = 0 ,dataLen=0;
    char *dataPoint = NULL;
    serverAddrType *addr;
    config.read(CFG_OTA_ADDR , (void **)&addr);
    
    while (ota.len  < ota.fileSize)
    {
        ret = 0 ;
        len = 0 ;
        heardlen = 0;
        dataLen = 0;
        dataPoint = NULL;
        memset(httprequest, 0x00, sizeof(httprequest));
        memset(rxBuf , 0x00, sizeof(OTARXBUF_SIZE));
        socket_clear_buffer(clientId);

        OTA_DEBUG_LOG(OTA_DEBUG, ("【OTA】文件下载进度:%d%\n" ,  (ota.len*100)/ota.fileSize));
        if (ota.len  + ONESTEP >= ota.fileSize)
        {
            OTA_DEBUG_LOG(OTA_DEBUG, ("【OTA】获取数据（最后一包）, %d-%d \n" , ota.len , ota.fileSize));
            httpsendLen = sprintf((char *)httprequest, "GET %s HTTP/1.1\r\nHost: %s\r\nConnection: keep-alive\r\nRange: bytes=%d-%d\r\n\r\n", ota.fileKey, addr->ip, ota.len , ota.fileSize);
            dataLen = ota.fileSize - ota.len;
        }
        else
        {
            OTA_DEBUG_LOG(OTA_DEBUG, ("【OTA】获取数据（日常一包）, %d-%d \n" , ota.len ,  ota.len  + (ONESTEP-1)));
            httpsendLen = sprintf((char *)httprequest, "GET %s HTTP/1.1\r\nHost: %s\r\nConnection: keep-alive\r\nRange: bytes=%d-%d\r\n\r\n", ota.fileKey, addr->ip ,ota.len ,ota.len  + (ONESTEP-1));
            dataLen = ONESTEP;
        }
        
        OTA_DEBUG_LOG(OTA_DEBUG, ("【OTA】准备发送 %s\n" , httprequest));
        ret = socket.send(clientId , httprequest , httpsendLen , 3000);
        if( ret != SOCKET_OK)
        {
            OTA_DEBUG_LOG(OTA_DEBUG, ("【OTA】socket.send FAIL = %s \n", httprequest));
            return ret;
        }
        
        ret = socket.read(clientId  , 10000);
        OTA_DEBUG_LOG(OTA_DEBUG, ("【OTA】接到消息的长度:  %d\n", ret));
        if (ret == 0)  
        {
            OTA_DEBUG_LOG(OTA_DEBUG, ("【OTA】接到消息的长度0 读取数据失败"));
            return SOCKER_READ_ERR;
        }
                                      
        OTA_DEBUG_LOG(OTA_DEBUG, ("【OTA】接到消息的内容:%s \n" , rxBuf ));
        if( ota_check_http(rxBuf ) == FALSE)
        {
            OTA_DEBUG_LOG(OTA_DEBUG, ("【OTA】接到返回HTTP的数据头错误\n"));
            continue;
            
        }
        
        //解析数据长度
        len  = analysis_file_length(rxBuf );
        OTA_DEBUG_LOG(OTA_DEBUG, ("【OTA】解析HTTP头，数据长度:%d\n", len));
        if(len == 0)
        {
            OTA_DEBUG_LOG(OTA_DEBUG, ("【OTA】数据长度为0 ，重新读取 , 当前接受长度=%d\n" , ota.len));
            continue;
        }

        //解析Http协议头，获取头的长度及数据起始地址
        dataPoint = strstr(rxBuf  , "\r\n\r\n");
        if(dataPoint == NULL)
        {
            OTA_DEBUG_LOG(OTA_DEBUG, ("【OTA】heard data is error\n"));
            return SOCKER_READ_ERR;
        }
        
        dataPoint += strlen("\r\n\r\n");
        heardlen = (char *)dataPoint - (char*)rxBuf ;

        if( (heardlen + len == ret)&&(len == dataLen) )
        {/*和约定的TX一样的*/
            OTA_DEBUG_LOG(OTA_DEBUG, ("【OTA-GOOD】接收包中包含数据，heard len =%d , len = %d , datalen = %d\n" , heardlen , len , dataLen));
            ota_write_file((uint8_t *)(dataPoint), len);

            ota.len  += len;
            OTA_DEBUG_LOG(OTA_DEBUG, ("【OTA-GOOD】接收到总长度:%d\n", ota.len ));
        }
        else
        {
            
            uint8_t dBuff[1024];
            uint16_t buffTemLen = 0 ,lastLen =0;
            
            OTA_DEBUG_LOG(OTA_DEBUG, ("【OTA-BAD】接收数据中不包含完整的头+数据，继续接收数据，heard len =%d , len = %d , datalen = %d\n" , heardlen , len , dataLen)); 
              
            memset(dBuff , 0x00 , 1024);
            
            buffTemLen = ret - heardlen;
            lastLen = dataLen-buffTemLen;
            OTA_DEBUG_LOG(OTA_DEBUG, ("【OTA-BAD】当前已接收数据长度=%d , 剩余长度=%d\n" , buffTemLen , lastLen)); 
            memcpy(dBuff , dataPoint , buffTemLen);
            
            if( ota_wait_data(dBuff+buffTemLen ,  lastLen) ==TRUE)
            {
                OTA_DEBUG_LOG(OTA_DEBUG, ("【OTA-BAD】数据接收完成\n")); 
                ota_write_file((uint8_t *)dBuff,dataLen);
                ota.len  += dataLen;
                OTA_DEBUG_LOG(OTA_DEBUG, ("【OTA-BAD】接收到总长度:%d\n", ota.len )); 
            }
        }           
 
        sys_delay(1000);

    }

    
    
    OTA_DEBUG_LOG(OTA_DEBUG, ("【OTA-DONE】接收到总长度 = %d \n", ota.len )); 
    if( ota.len ==  ota.fileSize)
    {
        OTA_DEBUG_LOG(OTA_DEBUG, ("【OTA-DONE】文件接收完成\n"));
        if( ota_ver_file() == TRUE )
        {
            OTA_DEBUG_LOG(OTA_DEBUG, ("【OTA-DONE】文件验证非常好\n"));
            return SOCKET_OK;
        }
        else
        {
            OTA_DEBUG_LOG(OTA_DEBUG, ("【OTA-DONE】文件验证糟糕白忙活了\n"));
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
            if( socket.isOK() == TRUE) {
                otaRunStatus = RUN_CONNECTING;
            } else {
                sys_delay(600);
                OTA_DEBUG_LOG(OTA_DEBUG, ("【OTA】RUN_INIT socket.isOK() != TRUE\n"));
            }
        }break;
        case RUN_CONNECTING:
        {
            OTA_DEBUG_LOG(OTA_DEBUG, ("【OTA】RUN_CONNECTING ota start connect server\n"));
            memset(rxBuf  , 0x00 , OTARXBUF_SIZE);
            //if( (ret = socket.connect("34.73.14.154" , 80 ,rxBuf  , OTARXBUF_SIZE)) >= 0)
            if( (ret = socket.connect("ibinhub.com" , 80 ,rxBuf  , OTARXBUF_SIZE)) >= 0)
            {
                clientId = ret;
                OTA_DEBUG_LOG(OTA_DEBUG, ("【OTA】RUN_CONNECTING ota connect server success , client id = %d\n" , clientId));
                otaRunStatus = RUN_CONNECT;
            }
            else
            {
                ota_set_init();
                OTA_DEBUG_LOG(OTA_DEBUG, ("【OTA】RUN_CONNECTING ota connect server fail , client id = %d\n" , clientId));
            }

        }break;
        case RUN_CONNECT:
        {
            if( (ret = ota_download_read_file()) == SOCKET_OK )
            {
                otaType otaCfg;

                OTA_DEBUG_LOG(OTA_DEBUG, ("【OTA】文件下载成功\n"));

                otaCfg.ver = ota.ver;
                otaCfg.crc32 = ota.crc32;
                otaCfg.fileSize = ota.fileSize;
                otaCfg.otaUpgMark = UPG_MARK;
                config.write(CFG_OTA_CONFIG ,&otaCfg , TRUE);

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
                        ota.otaStatus = DOWMLOAD_FILE;
                }
                OTA_DEBUG_LOG(OTA_DEBUG, ("【OTA】------CHECH_UPG_FILE SLEEPING---------\n")); 
            }break;  
            
            case DOWMLOAD_FILE:
            {
                 if( ota_download_file() == OTA_OK)
                 {
                     ota.otaStatus = SYS_UPG_MODE;
                 }
            }break;
            
            case SYS_UPG_MODE:
                 soft_system_resert(__func__);
                 break;

            default:break;
        }
        sys_delay(1000);
    }
}


void creat_mqtt_ota_task( void )
{
    osThreadDef( ota, ota_task , osPriorityLow, 0, configMINIMAL_STACK_SIZE*10);
    otaTask = osThreadCreate(osThread(ota), NULL);
    configASSERT(otaTask);
}


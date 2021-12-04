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
#include "mqtt_task.h"

extern char              rxOtaData[4096];
static char              *rxBuf = rxOtaData;
#define                    OTARXBUF_SIZE    4096
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
#define OTA_DEBUG                                                     0


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
    
    uint32_t writeAddr = ota.len + OTA_START_ADDR;
    
    if( writeAddr%FLASH_SPI_BLOCKSIZE == 0)
    {
        if(dev_ota_erase_flash(writeAddr)==FALSE)
        {
             OTA_DEBUG_LOG(OTA_DEBUG, ("【OTA】本地写的时候 反馈不能擦\n"));
             return FALSE;
        }        

    }

    if(dev_ota_write_flash(writeAddr , msg , len)==FALSE)OTA_DEBUG_LOG(OTA_DEBUG, ("【OTA】本地写的时候失败了\n"));
    OTA_DEBUG_LOG(1, ("【OTA】本地写好 在地址 0X%08X 后面写了这么长%d \n", writeAddr,len ));
    
    return TRUE;
}

#define ONE_FILE_LEN       FLASH_SPI_BLOCKSIZE

#include "mbedtls/md5.h"
uint32_t file_MD5(void)
{
    int i,allsteps,lastlen;
    uint32_t readAddr = OTA_START_ADDR;
     /*考虑到BOOT/APP两个程序同时使用定义512吧 每次读出来这么多*/    
    //unsigned char encrypt[ONE_FILE_LEN];
    unsigned char *encrypt = fb;
    unsigned char decrypt[16];
    mbedtls_md5_context md5;
    mbedtls_md5_starts(&md5);  
    uint32_t fileSize = ota.fileSize ;
    /*全部的file分割为多少个ONE_FILE_LEN*/  
    if( (fileSize)%ONE_FILE_LEN !=0)
        allsteps = fileSize/ONE_FILE_LEN+1;
    else
        allsteps = fileSize/ONE_FILE_LEN;

    /*前面N-1个都是正正好好的一块一块*/ 
    for(i=0;i<allsteps-1;i++){
        memset(encrypt , 0x00 , ONE_FILE_LEN);
        dev_ota_read_flash(readAddr , encrypt , ONE_FILE_LEN);
        mbedtls_md5_update(&md5,encrypt,ONE_FILE_LEN);
        printf("readAddr 0X%08X\r\n",readAddr);
        readAddr += ONE_FILE_LEN;
    }
    /*最后一块可能不是完整的*/ 
    lastlen = fileSize - ((allsteps-1)*ONE_FILE_LEN);
    if(lastlen)
    {
      memset(encrypt , 0x00 , lastlen);
      printf("[readAddr] 0X%08X\r\n",readAddr);
      dev_ota_read_flash(readAddr , encrypt , lastlen);
      mbedtls_md5_update(&md5,encrypt,lastlen);
    }
    /*最后结束*/
    mbedtls_md5_finish(&md5,decrypt);   
    
    printf("file_MD5文件长度是%d,分成的小块是每块长度%d,得到%d个整块+最后长度是%d\r\n",fileSize,ONE_FILE_LEN,allsteps-1,lastlen);
    log_arry(DEBUG,"结果计算MD5是:" ,decrypt, 16);
    
    uint16_t crc = CRC16_CCITT(decrypt,16);/*把最后16个HEX算一下 丢出去*/

    return crc;

}



uint8_t ota_ver_file( void )
{

    if( ota.crc32 == file_MD5() )
    {
        OTA_DEBUG_LOG(1, ("【OTA】文件校验正确\n" ));

        return TRUE;
    }

    OTA_DEBUG_LOG(OTA_DEBUG, ("【OTA】文件校验失败" ));
 
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

    memset( ota.fileKey , 0x00 , sizeof(ota.fileKey));

}

void ota_init_buffer( void )
{
    otaType *otaCfg;

    memset(&ota , 0x0 , sizeof(otaRecvCmdType));
    ota.otaStatus = CHECH_UPG_FILE;
 
    config.read(CFG_OTA_CONFIG,(void **)&otaCfg);
    if( otaCfg->otaUpgMark == UPG_MARK)
    {
        ota.otaStatus = SYS_UPG_MODE;
        NEVERSHOW
          NEVERSHOW
            NEVERSHOW
          
    }
/*挨个赋值*/    

/*1*/  
char *fileKey;
config.read(CFG_OTA_URL , (void **)&fileKey); 
memcpy(ota.fileKey,fileKey ,strlen(fileKey)  );
printf("ota.fileKey ---------%s------------\r\n",ota.fileKey);
//memcpy(ota.fileKey,"/upload/1487627177.bin" ,strlen("/upload/1487627177.bin")  );
 


/*2*/     
ota.len=       0;

/*3*/  
ota.fileSize= otaCfg->fileSize;
//ota.fileSize=  142430;

/*4*/  
ota.ver=otaCfg->ver;

//ota.ver=444;

/*5*/  
ota.crc32=otaCfg->crc32;
//char md5[33]={"2d5b4efd001049a67f7cd5e1e5da4c66"};
//uint8_t Md5[16]={0};
//G_strsTobytes(md5,Md5,32);
//ota.crc32=CRC16_CCITT(Md5,16);
//log_arry(DEBUG,"平台计算MD5是" ,Md5, 16);
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
    uint8_t  httprequest[400];
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

        OTA_DEBUG_LOG(1, ("【OTA】文件下载进度:%d\n" ,  (ota.len*100)/ota.fileSize));
        if (ota.len  + ONESTEP >= ota.fileSize)
        {
            OTA_DEBUG_LOG(1, ("【OTA】获取数据（最后一包）, %d-%d \n" , ota.len , ota.fileSize));
            httpsendLen = sprintf((char *)httprequest, "GET %s HTTP/1.1\r\nHost: %s\r\nConnection: keep-alive\r\nRange: bytes=%d-%d\r\n\r\n", ota.fileKey, addr->ip, ota.len , ota.fileSize);
            dataLen = ota.fileSize - ota.len;
        }
        else
        {
            OTA_DEBUG_LOG(1, ("【OTA】获取数据（日常一包）, %d-%d \n" , ota.len ,  ota.len  + (ONESTEP-1)));
            httpsendLen = sprintf((char *)httprequest, "GET %s HTTP/1.1\r\nHost: %s\r\nConnection: keep-alive\r\nRange: bytes=%d-%d\r\n\r\n", ota.fileKey, addr->ip ,ota.len ,ota.len  + (ONESTEP-1));
            dataLen = ONESTEP;
        }
        
        OTA_DEBUG_LOG(OTA_DEBUG, ("【OTA】准备发送 %s\n" , httprequest));
        ret = socket.send(clientId , httprequest , httpsendLen , 3000);
        if( ret != SOCKET_OK)
        {
            OTA_DEBUG_LOG(1, ("【OTA】socket.send FAIL = %d \n", ret));
            return SOCKER_READ_ERR;
        }
        ret = socket.read(clientId  , 10000);
        OTA_DEBUG_LOG(OTA_DEBUG, ("【OTA】接到消息的长度: [ %d]\n", ret));

        if (ret < 0)  
        {
            OTA_DEBUG_LOG(1, ("【OTA】接到消息的长度<0 socket.read读取数据失败[ %d]\n", ret));
            return SOCKER_READ_ERR;
        }
                                      
        //OTA_DEBUG_LOG(OTA_DEBUG, ("【OTA】接到消息的内容【打印这个会死机 兄弟！】:%s \n" , rxBuf ));
        if( ota_check_http(rxBuf ) == FALSE)
        {
            OTA_DEBUG_LOG(1, ("【OTA】接到返回HTTP的数据头错误\n"));
            memset(httprequest, '\0', sizeof(httprequest));
            memcpy(httprequest,rxBuf,100);
            OTA_DEBUG_LOG(1, ("【%s】\n",httprequest));
            return SOCKER_READ_ERR;          
        }
        
        //解析数据长度
        len  = analysis_file_length(rxBuf );
        OTA_DEBUG_LOG(OTA_DEBUG, ("【OTA】解析HTTP头，数据长度:%d\n", len));
        if(len == 0)
        {
            OTA_DEBUG_LOG(OTA_DEBUG, ("【OTA】数据长度为0 ，重新读取 , 当前接受长度=%d\n" , ota.len));
            return SOCKER_READ_ERR;
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
    }

    
    
    OTA_DEBUG_LOG(OTA_DEBUG, ("【OTA-DONE】接收到总长度 = %d \n", ota.len )); 
    if( ota.len ==  ota.fileSize)
    {
        OTA_DEBUG_LOG(OTA_DEBUG, ("【OTA-DONE】文件接收完成\n"));
        socket.disconnect(clientId);
        
        if( ota_ver_file() == TRUE )
        {
            OTA_DEBUG_LOG(1, ("【OTA-DONE】文件验证非常好\n"));

            return SOCKET_OK;
        }
        else
        {
            OTA_DEBUG_LOG(1, ("【OTA-DONE】文件验证糟糕白忙活了\n"));

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
          
            if(mqtt_network_normal() != TRUE) 
            {
              OTA_DEBUG_LOG(1, ("RUN_CONNECTING 当前MQTT异常 啥也不干了\n"));
              break;
            }
            OTA_DEBUG_LOG(OTA_DEBUG, ("【OTA】RUN_CONNECTING ota start connect server\n"));
            memset(rxBuf  , 0x00 , OTARXBUF_SIZE);
            serverAddrType *addr;         
            config.read(CFG_OTA_ADDR , (void **)&addr);
            
            //if( (ret = socket.connect("34.73.14.154" , 80 ,rxBuf  , OTARXBUF_SIZE)) >= 0)
            //if( (ret = socket.connect("ibinhub.com" , 80 ,rxBuf  , OTARXBUF_SIZE)) >= 0)
            if( (ret = socket.connect(addr->ip , addr->port ,rxBuf  , OTARXBUF_SIZE)) >= 0)
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
taskDISABLE_INTERRUPTS();
            ret = ota_download_read_file();
            if( ret == SOCKET_OK )
            {
                otaType otaCfg;

                OTA_DEBUG_LOG(OTA_DEBUG, ("【OTA】文件下载成功\n"));

                otaCfg.ver = ota.ver;
                otaCfg.crc32 = ota.crc32;
                otaCfg.fileSize = ota.fileSize;
                otaCfg.otaUpgMark = UPG_MARK;
                //config.write(CFG_SYS_SW_VERSION ,&ota.ver , 0);
                config.write(CFG_OTA_CONFIG     ,&otaCfg , TRUE);
                return OTA_OK;
            }
            else if(ret == SOCKER_READ_ERR)
            {
              OTA_DEBUG_LOG(1, ("【OTA】文件下载遇到SOCKER_READ_ERR说明模组意外关闭\n"));
            
              ota_repert_connect();
            }
            else
            {
                if( ota.len > 8192)/*随便写的不要浪费了前面下载的*/
                {
                     ota.len =   (ota.len - ota.len%4096) - 4096;
                }
                else
                {
                    ota.len = 0;
                }

                OTA_DEBUG_LOG(OTA_DEBUG,("【OTA】网络下载错误 ，回退部分字节， 重新下载开始位置=%d\n" , ota.len));
                ota_repert_connect();
            }
taskENABLE_INTERRUPTS();            
        }break;
        default:break;
    }

    return OTA_DOWNLOAD_FAIL;
}



static void ota_task( void const *pvParameters)
{

    for( ; ; )
    {
        switch(ota.otaStatus)
        {
            case CHECH_UPG_FILE:
            {
                
                if( xSemaphoreTake( xMqttOtaSemaphore, portMAX_DELAY ) == pdTRUE )
                {
                      ota_init_buffer();
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
        sys_delay(100);
    }
}


void creat_mqtt_ota_task( void )
{
    osThreadDef( ota, ota_task , osPriorityLow, 0, configMINIMAL_STACK_SIZE*10);
    otaTask = osThreadCreate(osThread(ota), NULL);
    configASSERT(otaTask);
}


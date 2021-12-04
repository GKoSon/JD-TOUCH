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
*********************************** ������ӡ���� ***********************************
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
*********************************** ����string��һЩС���� ***********************************
*/
static uint8_t assert_len(uint8_t *name , uint8_t len)
{
    if( ++assertCnt > len )
    {
        OTA_DEBUG_LOG(OTA_DEBUG, ("��OTA��%s  is too big , max len = %d\n" , name , len));
        return TRUE;
    }
    return FALSE;
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
static uint8_t ota_check_http(char *msg)
{
    return (aiot_strcmp((uint8_t *)msg , "HTTP/1.1 2" , strlen("HTTP/1.1 2")));
    //https://blog.csdn.net/weixin_42381351/article/details/121474307
}

/*
TX����
GET /upload/193599818.bin HTTP/1.1
Host:ibinhub.com
Connection:keep-alive

RX����
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
       OTA_DEBUG_LOG(OTA_DEBUG, ("��OTA��û�л�ȡ����������Content-Length\n"));
    }

    OTA_DEBUG_LOG(OTA_DEBUG, ("��OTA�������ļ�����Content-Length =  %d \n", len));

    return len;
}

/*
*********************************** �ļ����� ***********************************
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
             OTA_DEBUG_LOG(OTA_DEBUG, ("��OTA������д��ʱ�� �������ܲ�\n"));
             return FALSE;
        }        

    }

    if(dev_ota_write_flash(writeAddr , msg , len)==FALSE)OTA_DEBUG_LOG(OTA_DEBUG, ("��OTA������д��ʱ��ʧ����\n"));
    OTA_DEBUG_LOG(1, ("��OTA������д�� �ڵ�ַ 0X%08X ����д����ô��%d \n", writeAddr,len ));
    
    return TRUE;
}

#define ONE_FILE_LEN       FLASH_SPI_BLOCKSIZE

#include "mbedtls/md5.h"
uint32_t file_MD5(void)
{
    int i,allsteps,lastlen;
    uint32_t readAddr = OTA_START_ADDR;
     /*���ǵ�BOOT/APP��������ͬʱʹ�ö���512�� ÿ�ζ�������ô��*/    
    //unsigned char encrypt[ONE_FILE_LEN];
    unsigned char *encrypt = fb;
    unsigned char decrypt[16];
    mbedtls_md5_context md5;
    mbedtls_md5_starts(&md5);  
    uint32_t fileSize = ota.fileSize ;
    /*ȫ����file�ָ�Ϊ���ٸ�ONE_FILE_LEN*/  
    if( (fileSize)%ONE_FILE_LEN !=0)
        allsteps = fileSize/ONE_FILE_LEN+1;
    else
        allsteps = fileSize/ONE_FILE_LEN;

    /*ǰ��N-1�����������úõ�һ��һ��*/ 
    for(i=0;i<allsteps-1;i++){
        memset(encrypt , 0x00 , ONE_FILE_LEN);
        dev_ota_read_flash(readAddr , encrypt , ONE_FILE_LEN);
        mbedtls_md5_update(&md5,encrypt,ONE_FILE_LEN);
        printf("readAddr 0X%08X\r\n",readAddr);
        readAddr += ONE_FILE_LEN;
    }
    /*���һ����ܲ���������*/ 
    lastlen = fileSize - ((allsteps-1)*ONE_FILE_LEN);
    if(lastlen)
    {
      memset(encrypt , 0x00 , lastlen);
      printf("[readAddr] 0X%08X\r\n",readAddr);
      dev_ota_read_flash(readAddr , encrypt , lastlen);
      mbedtls_md5_update(&md5,encrypt,lastlen);
    }
    /*������*/
    mbedtls_md5_finish(&md5,decrypt);   
    
    printf("file_MD5�ļ�������%d,�ֳɵ�С����ÿ�鳤��%d,�õ�%d������+��󳤶���%d\r\n",fileSize,ONE_FILE_LEN,allsteps-1,lastlen);
    log_arry(DEBUG,"�������MD5��:" ,decrypt, 16);
    
    uint16_t crc = CRC16_CCITT(decrypt,16);/*�����16��HEX��һ�� ����ȥ*/

    return crc;

}



uint8_t ota_ver_file( void )
{

    if( ota.crc32 == file_MD5() )
    {
        OTA_DEBUG_LOG(1, ("��OTA���ļ�У����ȷ\n" ));

        return TRUE;
    }

    OTA_DEBUG_LOG(OTA_DEBUG, ("��OTA���ļ�У��ʧ��" ));
 
    return FALSE;  
}
/*
*********************************** ������� ***********************************
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
/*������ֵ*/    

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
//log_arry(DEBUG,"ƽ̨����MD5��" ,Md5, 16);
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

        OTA_DEBUG_LOG(1, ("��OTA���ļ����ؽ���:%d\n" ,  (ota.len*100)/ota.fileSize));
        if (ota.len  + ONESTEP >= ota.fileSize)
        {
            OTA_DEBUG_LOG(1, ("��OTA����ȡ���ݣ����һ����, %d-%d \n" , ota.len , ota.fileSize));
            httpsendLen = sprintf((char *)httprequest, "GET %s HTTP/1.1\r\nHost: %s\r\nConnection: keep-alive\r\nRange: bytes=%d-%d\r\n\r\n", ota.fileKey, addr->ip, ota.len , ota.fileSize);
            dataLen = ota.fileSize - ota.len;
        }
        else
        {
            OTA_DEBUG_LOG(1, ("��OTA����ȡ���ݣ��ճ�һ����, %d-%d \n" , ota.len ,  ota.len  + (ONESTEP-1)));
            httpsendLen = sprintf((char *)httprequest, "GET %s HTTP/1.1\r\nHost: %s\r\nConnection: keep-alive\r\nRange: bytes=%d-%d\r\n\r\n", ota.fileKey, addr->ip ,ota.len ,ota.len  + (ONESTEP-1));
            dataLen = ONESTEP;
        }
        
        OTA_DEBUG_LOG(OTA_DEBUG, ("��OTA��׼������ %s\n" , httprequest));
        ret = socket.send(clientId , httprequest , httpsendLen , 3000);
        if( ret != SOCKET_OK)
        {
            OTA_DEBUG_LOG(1, ("��OTA��socket.send FAIL = %d \n", ret));
            return SOCKER_READ_ERR;
        }
        ret = socket.read(clientId  , 10000);
        OTA_DEBUG_LOG(OTA_DEBUG, ("��OTA���ӵ���Ϣ�ĳ���: [ %d]\n", ret));

        if (ret < 0)  
        {
            OTA_DEBUG_LOG(1, ("��OTA���ӵ���Ϣ�ĳ���<0 socket.read��ȡ����ʧ��[ %d]\n", ret));
            return SOCKER_READ_ERR;
        }
                                      
        //OTA_DEBUG_LOG(OTA_DEBUG, ("��OTA���ӵ���Ϣ�����ݡ���ӡ��������� �ֵܣ���:%s \n" , rxBuf ));
        if( ota_check_http(rxBuf ) == FALSE)
        {
            OTA_DEBUG_LOG(1, ("��OTA���ӵ�����HTTP������ͷ����\n"));
            memset(httprequest, '\0', sizeof(httprequest));
            memcpy(httprequest,rxBuf,100);
            OTA_DEBUG_LOG(1, ("��%s��\n",httprequest));
            return SOCKER_READ_ERR;          
        }
        
        //�������ݳ���
        len  = analysis_file_length(rxBuf );
        OTA_DEBUG_LOG(OTA_DEBUG, ("��OTA������HTTPͷ�����ݳ���:%d\n", len));
        if(len == 0)
        {
            OTA_DEBUG_LOG(OTA_DEBUG, ("��OTA�����ݳ���Ϊ0 �����¶�ȡ , ��ǰ���ܳ���=%d\n" , ota.len));
            return SOCKER_READ_ERR;
        }

        //����HttpЭ��ͷ����ȡͷ�ĳ��ȼ�������ʼ��ַ
        dataPoint = strstr(rxBuf  , "\r\n\r\n");
        if(dataPoint == NULL)
        {
            OTA_DEBUG_LOG(OTA_DEBUG, ("��OTA��heard data is error\n"));
            return SOCKER_READ_ERR;
        }
        
        dataPoint += strlen("\r\n\r\n");
        heardlen = (char *)dataPoint - (char*)rxBuf ;

        if( (heardlen + len == ret)&&(len == dataLen) )
        {/*��Լ����TXһ����*/
            OTA_DEBUG_LOG(OTA_DEBUG, ("��OTA-GOOD�����հ��а������ݣ�heard len =%d , len = %d , datalen = %d\n" , heardlen , len , dataLen));
            ota_write_file((uint8_t *)(dataPoint), len);

            ota.len  += len;
            OTA_DEBUG_LOG(OTA_DEBUG, ("��OTA-GOOD�����յ��ܳ���:%d\n", ota.len ));
        }
        else
        {
            
            uint8_t dBuff[1024];
            uint16_t buffTemLen = 0 ,lastLen =0;
            
            OTA_DEBUG_LOG(OTA_DEBUG, ("��OTA-BAD�����������в�����������ͷ+���ݣ������������ݣ�heard len =%d , len = %d , datalen = %d\n" , heardlen , len , dataLen)); 
              
            memset(dBuff , 0x00 , 1024);
            
            buffTemLen = ret - heardlen;
            lastLen = dataLen-buffTemLen;
            OTA_DEBUG_LOG(OTA_DEBUG, ("��OTA-BAD����ǰ�ѽ������ݳ���=%d , ʣ�೤��=%d\n" , buffTemLen , lastLen)); 
            memcpy(dBuff , dataPoint , buffTemLen);
            
            if( ota_wait_data(dBuff+buffTemLen ,  lastLen) ==TRUE)
            {
                OTA_DEBUG_LOG(OTA_DEBUG, ("��OTA-BAD�����ݽ������\n")); 
                ota_write_file((uint8_t *)dBuff,dataLen);
                ota.len  += dataLen;
                OTA_DEBUG_LOG(OTA_DEBUG, ("��OTA-BAD�����յ��ܳ���:%d\n", ota.len )); 
            }
        }           
    }

    
    
    OTA_DEBUG_LOG(OTA_DEBUG, ("��OTA-DONE�����յ��ܳ��� = %d \n", ota.len )); 
    if( ota.len ==  ota.fileSize)
    {
        OTA_DEBUG_LOG(OTA_DEBUG, ("��OTA-DONE���ļ��������\n"));
        socket.disconnect(clientId);
        
        if( ota_ver_file() == TRUE )
        {
            OTA_DEBUG_LOG(1, ("��OTA-DONE���ļ���֤�ǳ���\n"));

            return SOCKET_OK;
        }
        else
        {
            OTA_DEBUG_LOG(1, ("��OTA-DONE���ļ���֤����æ����\n"));

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
                OTA_DEBUG_LOG(OTA_DEBUG, ("��OTA��RUN_INIT socket.isOK() != TRUE\n"));
            }
        }break;
        case RUN_CONNECTING:
        {
          
            if(mqtt_network_normal() != TRUE) 
            {
              OTA_DEBUG_LOG(1, ("RUN_CONNECTING ��ǰMQTT�쳣 ɶҲ������\n"));
              break;
            }
            OTA_DEBUG_LOG(OTA_DEBUG, ("��OTA��RUN_CONNECTING ota start connect server\n"));
            memset(rxBuf  , 0x00 , OTARXBUF_SIZE);
            serverAddrType *addr;         
            config.read(CFG_OTA_ADDR , (void **)&addr);
            
            //if( (ret = socket.connect("34.73.14.154" , 80 ,rxBuf  , OTARXBUF_SIZE)) >= 0)
            //if( (ret = socket.connect("ibinhub.com" , 80 ,rxBuf  , OTARXBUF_SIZE)) >= 0)
            if( (ret = socket.connect(addr->ip , addr->port ,rxBuf  , OTARXBUF_SIZE)) >= 0)
            {
                clientId = ret;
                OTA_DEBUG_LOG(OTA_DEBUG, ("��OTA��RUN_CONNECTING ota connect server success , client id = %d\n" , clientId));
                otaRunStatus = RUN_CONNECT;
            }
            else
            {
                ota_set_init();
                OTA_DEBUG_LOG(OTA_DEBUG, ("��OTA��RUN_CONNECTING ota connect server fail , client id = %d\n" , clientId));
            }

        }break;
        case RUN_CONNECT:
        {
taskDISABLE_INTERRUPTS();
            ret = ota_download_read_file();
            if( ret == SOCKET_OK )
            {
                otaType otaCfg;

                OTA_DEBUG_LOG(OTA_DEBUG, ("��OTA���ļ����سɹ�\n"));

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
              OTA_DEBUG_LOG(1, ("��OTA���ļ���������SOCKER_READ_ERR˵��ģ������ر�\n"));
            
              ota_repert_connect();
            }
            else
            {
                if( ota.len > 8192)/*���д�Ĳ�Ҫ�˷���ǰ�����ص�*/
                {
                     ota.len =   (ota.len - ota.len%4096) - 4096;
                }
                else
                {
                    ota.len = 0;
                }

                OTA_DEBUG_LOG(OTA_DEBUG,("��OTA���������ش��� �����˲����ֽڣ� �������ؿ�ʼλ��=%d\n" , ota.len));
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
                OTA_DEBUG_LOG(OTA_DEBUG, ("��OTA��------CHECH_UPG_FILE SLEEPING---------\n")); 
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


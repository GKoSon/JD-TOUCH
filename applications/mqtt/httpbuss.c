#include "net.h"
#include "usart.h"
#include "config.h"
#include "string.h"
#include "unit.h"
#include "socket.h"
#include "stdlib.h"
#include "spi_flash.h"
#include "crc32.h"
#include "sysCfg.h"
#include "tsl_mqtt.h"
#include "httpbuss.h"
#include "beep.h"

static char             httptxData[1024];
extern char             rxOtaData[2048];

        
typedef enum
{
    SLEEP,
    HTTPCONNECT,
    STRATEGY,
    OFFNETDEL,
}httpStatusEnum;

typedef struct
{
    httpStatusEnum    status;
    int8_t          scoketId;

}HttpType;
HttpType http;



void cleanup(void)
{
   memset(&http,0,sizeof(HttpType));
}

void retry(void)
{
  

   socket.close();
   http.status = HTTPCONNECT;

     
}



static void http_task( void const *pvParameters)
{ 
    httpRecvMsgType p;
    int sendLen = 0;
    char level=0,ack=0;
    int ret;
    serverAddrType *addr;
    uint16_t port  = 0; 
    cleanup();

    for( ; ; )
    {
        switch(http.status)
        {
            case SLEEP:
            {             
                    if(xQueueReceive(xHttpRecvQueue, &p, 1000) == pdTRUE) 
                    {
                      log(INFO,"HTTP׼������ ִ��rule��Ϣ%c \n", p.rule); 
                      http.status = HTTPCONNECT;
                    }
                   // else
                   //   printf("-----------HTTP˯��------------\r\n"); 
            }break;  
            case HTTPCONNECT:  
              
                       if(p.rule=='A')
                       {
                   
                                         if( (port  =  config.read(CFG_HTTP_ADDR , (void **)&addr)) == 0 )
                                          {
                                             printf("HTTP port=0 ��δ��װ�� ֱ��ȥ˯��\r\n");
                                             cleanup();
                                          }
                                          else if( (ret = socket.connect(addr->ip , port ,rxOtaData , sizeof(rxOtaData))) >= 0)
                                          {
                                              http.scoketId = ret;
                                              log(INFO,"HTTP connect server success , client id = %d \n" , http.scoketId);
                                              http.status = STRATEGY;
                                          }
                                          else 
                                          {
                                            retry();
                                          }
                       
                       }
                       else if(p.rule=='B')
                       {
                                          port  =  config.read(CFG_HTTP_ADDR , (void **)&addr);
                                          if( (ret = socket.connect(addr->ip , port ,rxOtaData , sizeof(rxOtaData))) >= 0)
                                          {
                                              http.scoketId = ret;
                                              log(INFO,"HTTP connect server success , client id = %d \n" , http.scoketId);
                                              http.status = STRATEGY;/*��Ҫ�ǿ��Կ�������DEL ��Ȼֻ��elseҲ����*/
                                          }
                                          else  
                                            http.status = OFFNETDEL;                   
                       
                       }
                  break;
            case STRATEGY:/* ��ʼ���� */
                  level = config.read(CFG_DEV_LEVEL , NULL);/*ÿ�ζ��ж�һ��*/
                  printf("-----------��ʼ����%c LEV=%d------------\r\n",p.rule,level); 
                  
                  if(p.rule=='A')/*ע����level=0���豸*/
                  {
/*��仰 ͦ��*/   
memset(httptxData,0,sizeof(httptxData));
sendLen = HTTP_packDEL(httptxData);
ret = socket.send(http.scoketId , (uint8_t *)httptxData , sendLen , 3000);
ret = 0 ;
ret = socket.read(http.scoketId  ,100000);
if (ret > 0)
{           
log(INFO,"[HTTP recv] =%s \n" , rxOtaData); 
}                    
                    
                      memset(httptxData,0,sizeof(httptxData));
                      sendLen = HTTP_packCANREG(httptxData);
                      ret = socket.send(http.scoketId ,(uint8_t *)httptxData , sendLen , 3000);
                      if( ret != SOCKET_OK)   {printf(" socket.send ERR\r\n"); retry();break;  }      
                      ret = 0 ;
                      ret = socket.read(http.scoketId  , 100000);
                      if (ret > 0)
                      {           
                        log(INFO,"[HTTP recv] =%s \n" , rxOtaData);
                        ack = HTTP_checkack(rxOtaData);
                        printf("-----ack=%d---\r\n",ack);
                        if(ack == (char)-1) { log(INFO,"�Լ����ʧ��\n" );cleanup();break;};
                        if(ack == 10)
                          { /*û��ע���*/   
                            
                                Elog(ERR,"A�����豸��δע�� ����ȥע��\r\n");
                                memset(httptxData,0,sizeof(httptxData));
                                sendLen = HTTP_packREG(httptxData);
                                Elog(INFO,"TX:��%s��\r\n",httptxData);
                                ret = socket.send(http.scoketId , (uint8_t *)httptxData , sendLen , 3000);
                                if( ret != SOCKET_OK)   {printf(" socket.send ERR\r\n");  retry();break;  }
                                ret = 0 ;
                                ret = socket.read(http.scoketId  ,100000);
                                if (ret > 0)
                                {           
                                    log(INFO,"[HTTP recv] =%s \n" , rxOtaData);
                                    ack = HTTP_checkack(rxOtaData);
                                    printf("-----ack=%d---\r\n",ack);
                                      if(ack == 0)
                                      {
                                        beep.write(1);
                                        Elog(ERR,"ע��� ������������\r\n");
                                        ret=1;
                                        config.write(CFG_DEV_LEVEL , &ret,TRUE);
                                        set_clear_flash(FLASH_ALL_BIT_BUT_CFG);
                                        cleanup();break;
                                      }
                                    else {Elog(ERR,"Aע��ʧ�� ����ѭ��\r\n");break;}
                                } 
                          }//if(ack == 10)
                        else if(ack == 9)
                          { /*û��ע���*/   
                                Elog(ERR,"A�����豸�Ѿ�ע�� ȥִ��UPDATE\r\n");
                                sendLen = HTTP_packUP(httptxData);
                                ret = socket.send(http.scoketId , (uint8_t *)httptxData , sendLen , 3000);
                                if( ret != SOCKET_OK)   {printf(" socket.send ERR\r\n");  retry();break;  }
                                ret = 0 ;
                                ret = socket.read(http.scoketId  ,100000);
                                if (ret > 0)
                                {           
                                    log(INFO,"[HTTP recv] =%s \n" , rxOtaData);
                                    ack = HTTP_checkack(rxOtaData);
                                    printf("-----ack=%d---\r\n",ack);
                                    if(ack == 0)
                                    {
                                        beep.write(1);
                                        Elog(ERR,"����������������\r\n");
                                        ret=1;
                                        config.write(CFG_DEV_LEVEL , &ret,TRUE);
                                        set_clear_flash(FLASH_ALL_BIT_BUT_CFG);
                                        cleanup();
                                    }
                                    else {Elog(ERR,"Aע��ʧ�� ����ѭ��\r\n");break;}
                                }
                          }//else if(ack == 9)
                      }//if (ret > 0)
                            
                     else {Elog(ERR,"REG FAIL �������� ����ѭ��\r\n");break;}

                  
                  }//if(p.rule=='A')
                  
                  else if(p.rule=='B')/*level������0 1 2 ������ ��������*/
                  {
                      sendLen = HTTP_packDEL(httptxData);
                      ret = socket.send(http.scoketId , (uint8_t *)httptxData , sendLen , 3000);
                      if( ret != SOCKET_OK)   {printf(" socket.send ERR\r\n");  retry();break;  }
                      ret = 0 ;
                      ret = socket.read(http.scoketId  ,100000);

                      if (ret > 0)
                      {           
                        log(INFO,"[HTTP recv] =%s \n" , rxOtaData);
                        ack = HTTP_checkack(rxOtaData);
                        printf("-----ack=%d---\r\n",ack);
                        if(ack == 0)
                          {
                            beep.write(1);
                            Elog(ERR,"DEL OK ������������\r\n");
                          }
                        else
                          log(ERR,"DEL û�лش� Ӧ����û����ŵ ���뱣֤���߰�װ�´μ��� ����levΪ0\r\n");
                      }
                      
                      set_clear_flash(FLASH_ALL_DATA);/*��ʵ���� ������ һ�� DEL ����ֵ����Ҫ�� �Ҷ��鵽0*/
                      
                      cleanup(); break; 

                  }//if(p.rule=='B')


              break;
            case OFFNETDEL:
            {             
                      set_clear_flash(FLASH_ALL_DATA);
                      
                      cleanup();  
            }break;          
            default:break;
        }
        sys_delay(200);
    }
}

char Get_HTTPStatus(void)
{
  return (char)http.status; 

}

void Reset_HTTPStatus(void)
{
   cleanup();
}

void creat_http_task( void )
{
    static xTaskHandle        http_task_;
    osThreadDef( http,http_task, osPriorityLow, 0, configMINIMAL_STACK_SIZE*10);
    http_task_= osThreadCreate(osThread(http), NULL);
    configASSERT(http_task_);
}


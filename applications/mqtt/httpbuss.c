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
                      log(INFO,"HTTP准备启动 执行rule消息%c \n", p.rule); 
                      http.status = HTTPCONNECT;
                    }
                   // else
                   //   printf("-----------HTTP睡觉------------\r\n"); 
            }break;  
            case HTTPCONNECT:  
              
                       if(p.rule=='A')
                       {
                   
                                         if( (port  =  config.read(CFG_HTTP_ADDR , (void **)&addr)) == 0 )
                                          {
                                             printf("HTTP port=0 从未安装过 直接去睡觉\r\n");
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
                                              http.status = STRATEGY;/*主要是可以看到立刻DEL 当然只有else也可以*/
                                          }
                                          else  
                                            http.status = OFFNETDEL;                   
                       
                       }
                  break;
            case STRATEGY:/* 开始策略 */
                  level = config.read(CFG_DEV_LEVEL , NULL);/*每次都有读一读*/
                  printf("-----------开始策略%c LEV=%d------------\r\n",p.rule,level); 
                  
                  if(p.rule=='A')/*注定是level=0的设备*/
                  {
/*多句话 挺好*/   
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
                        if(ack == (char)-1) { log(INFO,"自己打包失败\n" );cleanup();break;};
                        if(ack == 10)
                          { /*没有注册的*/   
                            
                                Elog(ERR,"A反馈设备尚未注册 继续去注册\r\n");
                                memset(httptxData,0,sizeof(httptxData));
                                sendLen = HTTP_packREG(httptxData);
                                Elog(INFO,"TX:【%s】\r\n",httptxData);
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
                                        Elog(ERR,"注册好 完美结束流程\r\n");
                                        ret=1;
                                        config.write(CFG_DEV_LEVEL , &ret,TRUE);
                                        set_clear_flash(FLASH_ALL_BIT_BUT_CFG);
                                        cleanup();break;
                                      }
                                    else {Elog(ERR,"A注册失败 重新循环\r\n");break;}
                                } 
                          }//if(ack == 10)
                        else if(ack == 9)
                          { /*没有注册的*/   
                                Elog(ERR,"A反馈设备已经注册 去执行UPDATE\r\n");
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
                                        Elog(ERR,"更新完美结束流程\r\n");
                                        ret=1;
                                        config.write(CFG_DEV_LEVEL , &ret,TRUE);
                                        set_clear_flash(FLASH_ALL_BIT_BUT_CFG);
                                        cleanup();
                                    }
                                    else {Elog(ERR,"A注册失败 重新循环\r\n");break;}
                                }
                          }//else if(ack == 9)
                      }//if (ret > 0)
                            
                     else {Elog(ERR,"REG FAIL 结束流程 重新循环\r\n");break;}

                  
                  }//if(p.rule=='A')
                  
                  else if(p.rule=='B')/*level可能是0 1 2 都可以 暴力重置*/
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
                            Elog(ERR,"DEL OK 完美结束流程\r\n");
                          }
                        else
                          log(ERR,"DEL 没有回答 应该是没有网诺 必须保证离线安装下次继续 设置lev为0\r\n");
                      }
                      
                      set_clear_flash(FLASH_ALL_DATA);/*其实这里 和上面 一样 DEL 返回值不重要了 我都归到0*/
                      
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


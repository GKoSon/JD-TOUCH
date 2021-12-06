#include "open_log.h"
#include "unit.h"
#include "component.h"
#include "spi_flash.h"
#include "bsp_rtc.h"
#include "net.h"
#include "timer.h"
#include "sysCfg.h"
#include "swipeTag.h"
#include "beep.h"
#include "mqtt_task.h"
#include "tsl_mqtt.h"


static xTaskHandle     logTask;

static uint8_t    timerHandle = 0xFF;
static __IO uint16_t    journalSn;
static int      sendPos = 0;
static uint32_t logEarseCnt = 0;
static uint32_t logEarsePos = 0;


static uint32_t openLogUseTime = 0;

void journal_write_flash(uint32_t pos , openLogType *openLog);



void journal_clear_all( void )
{
    uint32_t addr = 0;
    uint32_t page = LOG_MAX*LOG_SIZE/FLASH_SPI_BLOCKSIZE;

    log(INFO,"开始清空开门日志\n");
    
    if( (LOG_MAX*LOG_SIZE)%FLASH_SPI_BLOCKSIZE !=0)
    {
         page = LOG_MAX*LOG_SIZE/FLASH_SPI_BLOCKSIZE+1;
    }
    else
    {
        page = LOG_MAX*LOG_SIZE/FLASH_SPI_BLOCKSIZE;
    }

     flash.get_lock();
        
    for(uint16_t i = 0 ; i < page ; i++)
    {
        addr = OPEN_LOG_DATA_ADDRE + i*FLASH_SPI_BLOCKSIZE;
        
        //log(INFO,"擦除日志，擦除地址 = %x , 擦除页 = %d\n" , addr , addr/FLASH_SPI_BLOCKSIZE);
        printf("...");
        flash.earse(addr);
        
        HAL_IWDG_Refresh(&hiwdg);
        
    }

    flash.release_lock();
    
    log(INFO,"清空开门日志完成\n");

}


static uint8_t is_empty( openLogType *openLog ,uint8_t leng)
{
    uint8_t *pst = (uint8_t *)openLog;
    for( uint32_t i = 0 ;  i < leng ; i++)
    {
        if(*pst != 0xFF)
        {
            return FALSE;
        }
        pst++;
    }
    return TRUE;
}

void journal_write_flash(uint32_t pos , openLogType *openLog)
{
    uint32_t addr = 0 , page =0;
    uint8_t resert = FALSE;

    
    if(logEarseCnt%LOG_MAX != logEarsePos)
    {
        log_err("日志POS和CNT位置有问题，在位置0写入数据后,复位系统\n");
        openLog->hdr.eraseCnt = 0;
        openLog->hdr.cnt = 0;
        resert = TRUE;               
    }
    else
    {
   
        openLog->hdr.eraseCnt = ++logEarseCnt;
        openLog->hdr.cnt = ++logEarsePos;
    }
    if( openLog->hdr.cnt >= LOG_MAX)
    {
        openLog->hdr.cnt = 0;
        logEarsePos =0;
    }


    log(DEBUG,"在%d写入日志， 当前擦写次数=%d,下一次存储位置=%d\n" , pos , logEarseCnt ,logEarsePos); 
    
    addr  = OPEN_LOG_DATA_ADDRE+pos*LOG_SIZE;

    page = addr/FLASH_SPI_BLOCKSIZE;
    
    flash.get_lock();
    
    flash.read( page*FLASH_SPI_BLOCKSIZE , fb , FLASH_SPI_BLOCKSIZE);

    memcpy(fb + (pos*LOG_SIZE)%FLASH_SPI_BLOCKSIZE , (uint8_t *)openLog, LOG_SIZE);

    flash.earse(page*FLASH_SPI_BLOCKSIZE);

    flash.write( page*FLASH_SPI_BLOCKSIZE, fb, FLASH_SPI_BLOCKSIZE);
    
    flash.release_lock();
    
    if( resert == TRUE )
    {
        soft_system_resert(__func__);
    }
    
}

int32_t journal_write(openLogType *openLog)
{
    int32_t pos = 0;

    journal_write_flash(logEarsePos , openLog);
    
    return pos;
}

int journal_read( openLogType *aopenLog )
{
    openLogType openLog;
    uint16_t i = 0;

    flash.get_lock();
    for( i = 0 ; i < LOG_MAX ; i++)
    {
        flash.read(OPEN_LOG_DATA_ADDRE+i*LOG_SIZE , (uint8_t *)&openLog , LOG_SIZE);
        if( openLog.hdr.effective  ==  EFFECTIVE)
        {
            flash.release_lock();
            log(DEBUG,"找到一个未发送的日志， pos = %d \n" , i);
            memcpy(aopenLog , &openLog , LOG_SIZE);
            return i;
        }
    }
    flash.release_lock();
    return LOG_FIND_NULL;
}


void journal_del( int32_t pos)
{
    uint32_t addr = 0 , page =0 ;
    openLogType openLog;
    
    addr  = OPEN_LOG_DATA_ADDRE+pos*LOG_SIZE;

    page = addr/FLASH_SPI_BLOCKSIZE;

    flash.get_lock();
    flash.read(OPEN_LOG_DATA_ADDRE+pos*LOG_SIZE , (uint8_t *)&openLog , LOG_SIZE);
    flash.read( page*FLASH_SPI_BLOCKSIZE , fb , FLASH_SPI_BLOCKSIZE);

    openLog.hdr.effective = INVALID;
    memcpy(fb + (pos*LOG_SIZE)%FLASH_SPI_BLOCKSIZE , &openLog , LOG_SIZE);

    flash.earse(page*FLASH_SPI_BLOCKSIZE);

    flash.write( page*FLASH_SPI_BLOCKSIZE, fb, FLASH_SPI_BLOCKSIZE);

    flash.release_lock();
}


void journal_clear_pos( int32_t pos)
{
    uint32_t addr = 0 , page =0 ;
    openLogType openLog;
    
    memset(&openLog ,0x00 , sizeof(openLogType));
    addr  = OPEN_LOG_DATA_ADDRE+pos*LOG_SIZE;

    page = addr/FLASH_SPI_BLOCKSIZE;

    flash.read( page*FLASH_SPI_BLOCKSIZE , fb , FLASH_SPI_BLOCKSIZE);

    openLog.hdr.effective = INVALID;
    memcpy(fb + (pos*LOG_SIZE)%FLASH_SPI_BLOCKSIZE , &openLog , LOG_SIZE);

    flash.earse(page*FLASH_SPI_BLOCKSIZE);

    flash.write( page*FLASH_SPI_BLOCKSIZE, fb, FLASH_SPI_BLOCKSIZE);
}


void open_log_init( void )
{
    openLogType openLog;
    uint32_t checkNum = 0;

    logEarseCnt = 0;
    logEarsePos =0;    
        
    flash.get_lock();
    
    for(int32_t i = 0 ; i < LOG_MAX ; i++)
    {        
        flash.read(OPEN_LOG_DATA_ADDRE+i*LOG_SIZE , (uint8_t *)&openLog , sizeof(openLogHeardType));
        
        if(is_empty(&openLog , sizeof(openLogHeardType)) != TRUE)
        {
            if(( openLog.hdr.cnt > LOG_MAX) || (openLog.hdr.eraseCnt%LOG_MAX != openLog.hdr.cnt))
            {
                log(WARN,"该位置存储信息有问题， 清空改位置存储信息\n");
                journal_clear_pos(i);
            }
            else
            {
                if( logEarseCnt <  openLog.hdr.eraseCnt)
                {
                    logEarseCnt = openLog.hdr.eraseCnt;
                    logEarsePos = openLog.hdr.cnt;
                } 
            }
                     
        }

        memset(&openLog , 0x00 , sizeof(openLogType));
    }

    checkNum = logEarsePos;
    if( logEarseCnt > LOG_MAX )
    {
        checkNum = LOG_MAX;
    }
    for(int32_t i = 0 ; i < checkNum ; i++)
    {        
        flash.read(OPEN_LOG_DATA_ADDRE+i*LOG_SIZE , (uint8_t *)&openLog , sizeof(openLogHeardType));
        
        if(is_empty(&openLog , sizeof(openLogHeardType)) == TRUE)
        {
            log(WARN,"位置 %d 存储为空\n" , i);
            journal_clear_pos(i);
            
        }
        memset(&openLog , 0x00 , sizeof(openLogType));
    }
    
    flash.release_lock();

    log(DEBUG,"当前存储日志的flash擦写次数 = %u，当前存储位置 POS=%d \n" ,  logEarseCnt,logEarsePos);
    

}



void journal_save_log( openlogDataType *saveLog )
{
      journalTaskQueueType message;

      memset(&message , 0x00 , sizeof(journalTaskQueueType));
      message.cmd = LOG_ADD;
      message.sn = 0;
      memcpy(&message.openlog , saveLog , sizeof(openlogDataType));

      xQueueSend( xLogQueue, ( void* )&message, NULL );    
}


void journal_send_queue( journalCmdEnum cmd , uint16_t sn )
{
    journalTaskQueueType message;

    message.cmd = cmd;
    message.sn = sn;

    xQueueSend( xLogQueue, ( void* )&message, NULL );
}




void journal_add_into_card (openlogDataType *pkt )
{
    openLogType openLog;
    openLogUseCardDataType openCardLog;
        tagBufferType *tag =  (tagBufferType *)pkt->data;
        uint8_t openResult= pkt->length;

        memset(&openLog  , 0x00 , sizeof(openLogType));
    openLog.hdr.effective = EFFECTIVE;
    openLog.hdr.logType = 0;
    openLog.hdr.openResult = ((openResult == TRUE)?0:1);
    openLog.hdr.openTime = rtc.read_stamp();
    openLog.hdr.openType = OPEN_FOR_CARD;
    openCardLog.cardIssueType = tag->tagPower;
    openCardLog.cardType = tag->type;
    openCardLog.cardNumberLength = tag->UIDLength;
    memcpy(openCardLog.cardNumber , tag->UID , openCardLog.cardNumberLength);
    memcpy(openLog.data , (&openCardLog) ,  sizeof(openLogUseCardDataType));

        //for(int m=0;m<openCardLog.cardNumberLength;m++)printf("%02x-",openCardLog.cardNumber[m]);
        
    if( journal_write(&openLog) ==LOG_FIND_NULL)
        {
            log(WARN,"日志没有写入成功\n");
            return ;
        }

    if(mqtt_network_normal() ==  TRUE )
    {
        journal_send_queue(LOG_SEND  , 0 );
    }
    else
    {
         log(DEBUG,"MQTT离线，暂不发送，存储记录\n");
    }

}



void journal_add_into_pwd(openlogDataType *pkt )
{
    openLogType openLog;
    openLogUsePwdDataType usePwd;
    uint8_t *pwd = pkt->data;
    uint8_t pwdLength = pkt->length;

    memset(&openLog  , 0x00 , sizeof(openLogType));
    openLog.hdr.effective = EFFECTIVE;
    openLog.hdr.logType = 0;
    openLog.hdr.openResult = 0;    
    openLog.hdr.openTime = rtc.read_stamp();
    openLog.hdr.openType =  ((pwdLength==4)?OPEN_FOR_ONCE_PWD: OPEN_FOR_PWD);

    usePwd.pwdLength = pwdLength;
    memcpy(usePwd.password , pwd, usePwd.pwdLength);

    memcpy(openLog.data , (&usePwd) ,  sizeof(openLogUsePwdDataType));

    if( journal_write(&openLog) ==LOG_FIND_NULL)
    {
        log(WARN,"日志没有写入成功\n");
        return ;
    }


    if(mqtt_network_normal() ==  TRUE )
    {
        journal_send_queue(LOG_SEND  , 0 );
    }
    else
    {

                log(DEBUG,"MQTT离线，暂不发送，存储记录\n");
    }

}



void journal_add_into_qrcode(openlogDataType *pkt )
{

}


void journal_add_into_remote(openlogDataType *pkt )
{


}


void journal_add_into_key( openlogDataType *pkt )
{
    openLogType openLog;

    memset(&openLog  , 0x00 , sizeof(openLogType));
    openLog.hdr.effective = EFFECTIVE;
    openLog.hdr.logType = 0;
    openLog.hdr.openResult = 0;    
    openLog.hdr.openTime = rtc.read_stamp();
    openLog.hdr.openType = OPEN_FOR_IN_DOOR;

    memset(openLog.data , 0x00 ,  sizeof(openLog.data));

    if( journal_write(&openLog) ==LOG_FIND_NULL)
    {
      log(WARN,"日志没有写入成功\n");
      return ;
    }

    if(mqtt_network_normal() ==  TRUE )
    {
      journal_send_queue(LOG_SEND  , 0 );
    }
    else
    {
      log(ERR,"MQTT离线，暂不发送，存储记录\n");
    }

}


void journal_time_callback( void )
{

    journalTaskQueueType message;

    message.cmd = LOG_SEND;
    message.sn = 0;

    xQueueSendFromISR( xLogQueue, ( void* )&message, NULL );

}

void journal_stop_send_timer( void )
{
    timer.stop(timerHandle);
}

void journal_start_send_timer( void )
{
    timer.start(timerHandle);
}


extern void upuploadAccessLog_indoor(long openTime) ;
extern void upuploadAccessLog_card(long openTime,char lockStatus,char openResult,    char *cardNo,int cardType,int cardIssueType) ;
uint8_t journal_puck_string(openLogType *openlog , uint8_t *sendBuff)
{
    uint32_t openTime = openlog->hdr.openTime;
    switch(openlog->hdr.openType)
    {
 
    case OPEN_FOR_CARD:
    {
            char lockStatus=0, openResult=0;
            openLogUseCardDataType cardData;
            memcpy((uint8_t *)&cardData , openlog->data , sizeof(openLogUseCardDataType));

            if(openlog->hdr.openResult == 0)
            {
              openResult = 0;
              lockStatus = 0;
            }
            else
            {
              openResult = 2;
              lockStatus = 1;
            }

            printf("==打包OPEN_FOR_CARD==\r\n");
            
            if(cardData.cardNumberLength == 4)
              sprintf((char*)cardData.cardNumber,"%02X%02X%02X%02X",cardData.cardNumber[0],cardData.cardNumber[1],cardData.cardNumber[2],cardData.cardNumber[3]);  
            else   if(cardData.cardNumberLength == 8)         
              sprintf((char*)cardData.cardNumber,"%02X%02X%02X%02X%02X%02X%02X%02X",cardData.cardNumber[0],cardData.cardNumber[1],cardData.cardNumber[2],cardData.cardNumber[3],cardData.cardNumber[4],cardData.cardNumber[5],cardData.cardNumber[6],cardData.cardNumber[7]);
            
            upuploadAccessLog_card(openTime,lockStatus,openResult,(char*)cardData.cardNumber,cardData.cardType,cardData.cardIssueType); 
            
         }break;
      case OPEN_FOR_PWD:
      case OPEN_FOR_ONCE_PWD:
    {

    }break;
    case OPEN_FOR_APP_REMOTE:
    {
        
    }break;
    case OPEN_FOR_IN_DOOR:
    {
        
          printf("==打包OPEN_FOR_IN_DOOR==\r\n");
          upuploadAccessLog_indoor(openTime); 

    }break;
    case OPEN_FOR_FACE:

    case OPEN_FOR_FINGER:
    {
        log_err("不支持解析指纹日志\n");
    }break;
    default:
        log_err("没有处理这个开门方式\n");
        break;
    }
                                            


    return 0;
}



void journal_add_log(journalTaskQueueType *pkt)
{
    switch(pkt->openlog.type)
    {
        case OPENLOG_FORM_KEY:
        {
            journal_add_into_key(&pkt->openlog);
        }break;
        case OPENLOG_FORM_CARD:
        {
            journal_add_into_card(&pkt->openlog);
         }break;
        case OPENLOG_FORM_REMOTE:
        {
            journal_add_into_remote(&pkt->openlog);
        }break;
        case OPENLOG_FORM_PWD:
        {
            journal_add_into_pwd(&pkt->openlog);
        }break;
        case OPENLOG_FORM_QRCODE:
        {
            journal_add_into_qrcode(&pkt->openlog);
        }break;
        default:
        {
            beep.write(BEEP_ALARM);
            log(WARN,"没有这个选项\n");
        }break;
    }
}


uint16_t journal_read_send_log( uint8_t *msg )
{
    openLogType openLog;
    uint16_t length = 0;
    
    if( (sendPos = journal_read(&openLog)) != LOG_FIND_NULL)
    {
        length = journal_puck_string(&openLog , msg );
    timer.start(timerHandle);
    }
    else
    {
    log(DEBUG,"日志全部发送完成\n");
    timer.stop(timerHandle);
    }
    
    return length;
}


void journal_start_send()
{

      uint8_t openmsg[256] ;

      memset( openmsg , 0x00 , 256);
     
      if( (journal_read_send_log(openmsg)) > 0)
      {           
       openLogUseTime = HAL_GetTick();
      }
        
}    


   

void log_task( void const *pvParameters )
{

    journalTaskQueueType pst;

    open_log_init();
    
    timerHandle = timer.creat(600000 , FALSE , journal_time_callback);

    while(1)
    {
        if(xQueueReceive( xLogQueue, &pst, 1000) == pdTRUE)
        {
            switch(pst.cmd)
            {
            case LOG_DEL:
                log(DEBUG,"接到删除LOG命令， get sn =%d ,device sn = %d\n" , pst.sn , journalSn);
                if( 1 || pst.sn == (journalSn))
                {
                    log(ERR,"日志已发送，删除，POS=%d , use time=%dms\n",sendPos , HAL_GetTick()-openLogUseTime);
                    
                    journal_del(sendPos);
                    journal_send_queue(LOG_SEND,0);
                }
                else {
                  /*当初设计是 TX RX 序号一致 才执行del  现在我们只有TX 没有RX 也就无所谓了 一旦TX 就可DEL*/
                  /*journalSn 没有意义*/
                  // log(WARN,"sequenceId  和 return id错误 , sequenceId=%d ,get id=%d\n" , sequenceId , pst.sn);
                  // sequenceId = 0;
                }
                break;
                case LOG_SEND:
                        log(DEBUG,"接到发送LOG命令， get sn =%d ,device sn = %d\n" , pst.sn , journalSn);
                        journal_start_send();
                        break;
                case LOG_ADD:
                        log(DEBUG,"接到增加LOG命令， get sn =%d ,device sn = %d\n" , pst.sn , journalSn);
                        journal_add_log(&pst);
                break;
                    default:
                        log_err("%s错误的命令, CMD=%x\n" , __func__ , pst.cmd);
                    }
            memset(&pst , 0x00 , sizeof(journalTaskQueueType));
        }
        task_keep_alive(TASK_LOG_BIT);
    }
}



void creat_open_log_task( void )
{

    osThreadDef( log, log_task , osPriorityLow, 0, configMINIMAL_STACK_SIZE*10);
    logTask = osThreadCreate(osThread(log), NULL);
    configASSERT(logTask);
    
}


open_log_type journal = 
{
    .save_log =   journal_save_log,
    .clear =      journal_clear_all,
    .stop_timer = journal_stop_send_timer,
    .send_queue = journal_send_queue,
};


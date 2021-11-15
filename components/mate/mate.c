#if defined (USE_QRCODE)

#include "mate.h"
#include "SN75176BDR.h"
#include "component.h"
#include "crc16.h"
#include "string.h"
#include "serial.h"
#include "beep.h"
#include "qrcode.h"
#include "config.h"
#include "unit.h"
#include "timer.h"
QueueHandle_t        xMateQueue;
static xTaskHandle 	mateTask;
SemaphoreHandle_t    xMateSendSemaphore;

void *matePort = NULL;
mateRecvStatusEnum  mateReceStatus = MATE_HEARD1;
static uint8_t mateSn = 0;
uint8_t mateReceiveMode = FALSE;
static __IO uint8_t mateReceiveFinsh = FALSE;

mateDataType    mateData;


uint8_t receiveTimeoutTimerHandle = 0xFF;

void mate_set_en( uint8_t status )
{
	pin_ops.pin_write(RS485_EN_PIN , status);
}



void mate_send_to_uart( uint8_t *pData , uint16_t len)
{
	mate_set_en(PIN_HIGH);

	//sys_delay(5);

	serial.puts(matePort , pData , len);
	
	//sys_delay(5);

	mate_set_en(PIN_LOW);
}



uint8_t mate_puts(uint8_t *pData , uint16_t len , uint8_t ack)
{
    uint8_t cnt = 50;

    if( ack == TRUE )
    {
        mateReceiveMode = TRUE ;
        
        for(uint8_t i = 0 ; i < 3 ; i++)
        {
            mate_send_to_uart(pData , len);
            
            while(cnt--)
            {
                if( mateReceiveFinsh == TRUE )
                {
                    if( mateData.type ==  ACK_TYPE )
                    {
                        memset(&mateData , 0x00 , sizeof(mateDataType));
                        mateReceStatus = MATE_HEARD1;
                        mateReceiveMode = FALSE;
                        return TRUE;
                    }
                    else
                    {
                        //send queuq;
                    }
                }
                HAL_Delay(2);
            }
        }
        
        mateReceiveMode = FALSE;
    }
    else
    {
        mate_send_to_uart(pData , len);
        return TRUE;
    }
    
    return FALSE;
}


void mate_receive_timeout_isr(void)
{
    timer.stop(receiveTimeoutTimerHandle);
    mateReceStatus = MATE_HEARD1;
    memset(&mateData , 0x00 , sizeof(mateDataType));
}
                             


void mate_getchar( uint8_t ch)
{
    //printf("%x " ,ch);
    timer.start(receiveTimeoutTimerHandle);
    switch(mateReceStatus)
    {
        case MATE_HEARD1:
        {
            if(ch == 0x10)
            {
                mateReceStatus = MATE_HEARD2;
            }
        }break;
        case MATE_HEARD2:
        {
            if(ch == 0x23)
            {
                mateReceStatus = MATE_SN;
            }
        }break;
        case MATE_SN:
        {
            mateData.sn = ch;
            mateReceStatus = MATE_TYPE;
        }break;
        case MATE_TYPE:
        {
            mateData.type = ch;
            mateReceStatus = MATE_CMD;
        }break;
        case MATE_CMD:
        {
            mateData.cmd = ch;
            mateReceStatus = MATE_LENGTH;
        }break;
        case MATE_LENGTH:
        {
            mateData.length = ch;
            if(mateData.length > 0 )
            {
                mateReceStatus = MATE_DATA;
            }
            else
            {
                 mateReceStatus = MATE_CRC1;
            }
        }break;
        case MATE_DATA:
        {
            mateData.data[mateData.cnt] = ch;
            if(++mateData.cnt  == mateData.length)
            {
                mateReceStatus = MATE_CRC1;
            }
        }break;
        case MATE_CRC1:
        {
            mateData.crc16 = ch << 8;
            mateReceStatus = MATE_CRC2;
        }break;
        case MATE_CRC2:
        {
            uint16_t crc16 = 0;
            
            mateData.crc16 |= ch;
            
            crc16 = crc16_ccitt(mateData.data , mateData.length);
            
            if( mateData.crc16 == crc16 )
            {
                //log(INFO,"MATE接收正确\n");
                
                if( mateData.type != ACK_TYPE)
                {
                    //log(INFO,"send ack\n");
                    mate_send_ack();    //接收一个正确的非ACK包，返回ACK
                }
                if( mateReceiveMode == FALSE)
                {
                    //send queue
                    if(mateData.type == QRCODE_TYPE)
                    {
                        xQueueSendFromISR( xQRCodeQueue, ( void* )&mateData, NULL );
                        mateReceStatus = MATE_HEARD1;
                        memset(&mateData , 0x00 , sizeof(mateDataType));  
                        timer.stop(receiveTimeoutTimerHandle);
                    }
                }
                else
                {
                    mateReceiveFinsh = TRUE;
                }
                
            }

            mateReceStatus = MATE_HEARD1;
            
        }break;
        default:log(WARN,"no this status into mate\n");break;
    }
}


void mate_send(uint8_t type ,uint8_t cmd ,uint8_t *msg ,uint8_t length)
{
    mateSendBufferType    pb;
    
    uint16_t crc = 0;
    
    memset(&pb ,0x00 , sizeof(mateSendBufferType));
    
    if( xSemaphoreTake( xMateSendSemaphore, 100 ) != pdTRUE )
    {
        log_err("mate 发送失败，有数据正在发送\n");
        return;
    }
                
    pb.data[pb.len++] = 0x10;
    pb.data[pb.len++] = 0x23;
    pb.data[pb.len++] = mateSn++;
    pb.data[pb.len++] = type;
    pb.data[pb.len++] = cmd;
    pb.data[pb.len++] = length;
    if(length > 0 )
    {
        memcpy( pb.data+pb.len , msg , length);
        pb.len += length;
    }
    
    crc = crc16_ccitt(msg , length);
    
    pb.data[pb.len++] = crc >> 8;
    pb.data[pb.len++] = crc &0xff;
    
    pb.type = type;
    xQueueSend( xMateQueue, ( void* )&pb, NULL );
    
    xSemaphoreGive( xMateSendSemaphore );
   
}

void mate_send_ack( void )
{
    uint8_t data[256] , cnt = 0 ;
    uint16_t crc = 0;
    
    memset(data ,0x00 , 256);
    
    data[cnt++] = 0x10;
    data[cnt++] = 0x23;
    data[cnt++] = mateSn++;
    data[cnt++] = ACK_TYPE;
    data[cnt++] = 0;
    data[cnt++] = 0;

    crc = 0;
    
    data[cnt++] = crc >> 8;
    data[cnt++] = crc &0xff;
    
    mate_send_to_uart( data , cnt);
    

}



static void mate_task( void const *pvParameters)
{
    mateSendBufferType    msg;
    configASSERT( ( ( unsigned long ) pvParameters ) == 0 );
    
    xMateQueue = xQueueCreate( QUEUE_MATE_LENGTH, sizeof( mateSendBufferType ) );
    configASSERT(xMateQueue);
    
    
    configASSERT((xMateSendSemaphore = xSemaphoreCreateMutex()));
    
    memset(&msg , 0x00 , sizeof(mateSendBufferType));
    
    receiveTimeoutTimerHandle = timer.creat( 30 , FALSE ,  mate_receive_timeout_isr );
    
    while(1)
    { 
        if(xQueueReceive( xMateQueue, &msg, 1000 ) == pdTRUE)
        {
            if( xSemaphoreTake( xMateSendSemaphore, 60000 ) == pdTRUE )
            {
                if( mate_puts( msg.data , msg.len , ((msg.type == ACK_TYPE)?FALSE:TRUE)) == FALSE)
                {
                    log_arry(WARN,"mate send pb.data error",msg.data ,msg.len);
                }
                xSemaphoreGive( xMateSendSemaphore );
            }
            memset(&msg , 0x00 , sizeof(mateSendBufferType));
        }
        
        task_keep_alive(TASK_MATE_BIT);
        //read_task_stack(__func__,mateTask);
    }
}


void creat_mate_task( void )
{
    osThreadDef( mate, mate_task , osPriorityNormal, 0, configMINIMAL_STACK_SIZE*3);
    mateTask = osThreadCreate(osThread(mate), NULL);
    configASSERT(mateTask);
}




void mate_init( void )
{	
    
    matePort = serial.open("serial5");
    
    if( matePort == NULL)
    {
        beep.write(BEEP_ALARM);
        return ;
    }

	serial.init(matePort  , 115200 , mate_getchar);
    
    pin_ops.pin_mode(RS485_EN_PIN , PIN_MODE_OUTPUT);
	mate_set_en(PIN_LOW);
}



INIT_EXPORT(mate_init , "mate");



#endif



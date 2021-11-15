#include "config.h"
#include "bm77.h"
#include "unit.h"
#include "bsp.h"
#include "cmsis_os.h"
#include "modules_init.h"
#include "sysCfg.h"
#include "crc16.h"
#include "timer.h"

#include "beep.h"

#if (BULETOOTH_MODE == SINGLE_LINK)

static void *bm77_port = NULL;
extern BleUserMsgType       BleUserMsg;

static __IO uint8_t timerStart = FALSE;
static __IO uint8_t timerCnt = 0;


typedef enum
{
    BM77_DATA_MODE,
    BM77_CMD_MODE
}bm77ReceoveModeEnum;

uint8_t __IO receiveFlag = FALSE;
uint8_t commandBuffer[128];
uint8_t commandLength = 0;


__IO uint8_t bm77ReceiveMode  = BM77_DATA_MODE;

void bm77_resert( void )
{
	pin_ops.pin_write(BM77_RST , PIN_LOW);
	BM77_DELAY(200);
	pin_ops.pin_write(BM77_RST , PIN_HIGH);
	BM77_DELAY(100);
}

void bm77_cmd_mode(void)
{
        pin_ops.pin_write(BM77_P20 , PIN_LOW);
        bm77_resert();
        HAL_IWDG_Refresh(&hiwdg);
        BM77_DELAY(500);
        HAL_IWDG_Refresh(&hiwdg);
}

void bm77_normal_mode(void)
{
        pin_ops.pin_write(BM77_P20 , PIN_HIGH);

        bm77_resert();
        HAL_IWDG_Refresh(&hiwdg);
        BM77_DELAY(100);
        HAL_IWDG_Refresh(&hiwdg);
}



static void bm77_clear_buffer(void)
{
	timerCnt = 0;
	timerStart = FALSE;
	memset(&BleUserMsg , 0x00 ,  sizeof(BleAppMsgType));
}



uint8_t check_crc( void )
{
    if( (BleUserMsg.Msg[0].Data[BleUserMsg.Msg[0].DataLength-1] == 0x7E)&&
        (BleUserMsg.Msg[0].Data[BleUserMsg.Msg[0].DataLength-2] == 0x7D)&&
        (BleUserMsg.Msg[0].Data[BleUserMsg.Msg[0].DataLength-3] == 0x7C)&&
        (BleUserMsg.Msg[0].Data[BleUserMsg.Msg[0].DataLength-4] == 0x0B))
    {

        static BaseType_t xHigherPriorityTaskWoken =  pdFALSE;;

		BleUserMsg.Msg[0].Flag = TRUE;	
 		xSemaphoreGiveFromISR( xBtSemaphore, &xHigherPriorityTaskWoken ); 
                                
		portEND_SWITCHING_ISR(xHigherPriorityTaskWoken );

        return TRUE;
    }

    return FALSE;
}


void bm77_receive_usart_byte(uint8_t data)
{
    //printf("%x " ,data);
    if( bm77ReceiveMode == BM77_DATA_MODE)
    {
        BleUserMsg.Msg[0].Data[BleUserMsg.Msg[0].DataLength++] = data; /*可以用指针来玩漂移*/
        if( BleUserMsg.Msg[0].DataLength > 4)
        {
           if( check_crc() == TRUE )
		   {
		   		timerCnt = 0;
				timerStart = false;
		   }
		   else
		   {
		   		timerCnt = 0;
				timerStart = TRUE;
		   }
        }
    }
    else
    {
        commandBuffer[commandLength++] = data;
		timerCnt = 0;
		timerStart = TRUE;
    }
    
}

void bm77_receive_timer(void)
{
	if(timerStart)
	{
		if(timerCnt++ > 100)
		{
			bm77_clear_buffer();
            log(WARN,"蓝牙串口接受超时，清空BUFFER\n");
            receiveFlag = TRUE;
		}
	}
}

void bm77_clear_command_buffer( void )
{
    memset(commandBuffer , 0 , 128);
    commandLength = 0;
    receiveFlag = FALSE;
}

void bm77_into_command_mode( void )
{
    bm77_clear_command_buffer();
    
    bm77ReceiveMode = BM77_CMD_MODE;
    
    bm77_cmd_mode();
}

void bm77_out_command_mode( void )
{
  
    bm77_clear_buffer();
    
    bm77ReceiveMode = BM77_DATA_MODE;
    
    bm77_normal_mode();
}



void bm77_send_data(uint8_t *data , uint16_t length)
{
	serial.puts(bm77_port , data , length);
}


uint8_t bm77_read_parma(uint16_t cmd , uint8_t *respone ,uint8_t length)
{
    uint8_t buffer[128] = {0x00} , cnt = 0 ;
    uint16_t reCmd=0 , timeoutCnt = 700;
    
    buffer[cnt++] =  0x01;
    buffer[cnt++] =  0x29;
    buffer[cnt++] =  0xfc;
    buffer[cnt++] =  0x03;
    buffer[cnt++] =  ((cmd>>8)&0xff);
    buffer[cnt++] =  (cmd&0xff);
    buffer[cnt++] =  length;

    bm77_send_data(buffer , cnt);
    
    while(( receiveFlag == FALSE) && --timeoutCnt)
    {
        BM77_DELAY(2);
        HAL_IWDG_Refresh(&hiwdg);
    }
    if( timeoutCnt == 0)
    {
        log(ERR,"BM77 读取数据超时\n");
        return BLE_RECV_TIMEOUT_ERR;
    }
    
    if( commandLength == (length+ 10) )
    {
        if( commandBuffer[6] == 0x00) 
        {
            reCmd = commandBuffer[7]<<8|commandBuffer[8];
            if(( reCmd == cmd) && length == commandBuffer[9])
            {
                for(uint8_t i = 0 ; i < length ; i++)
                {
                    respone[i] = commandBuffer[10+i];
                }
            }
        }
        else
        {
            log(WARN,"BM77读取数据操作失败\n");
            return BLE_COMMAND_NOBACK_ERR;
        }
    }
    else
    {
        log(WARN,"BM77返回的数据长度出错，LENGTH= %d \n" , commandLength);
        return BLE_RECV_LENGTH_ERR;
    }
    
    bm77_clear_command_buffer();
    
    return BLE_OK;
    
}

uint8_t bm77_read_data(uint16_t cmd , uint8_t *respone ,uint8_t length)
{
    uint8_t i = 3 , resault = BLE_INIT_ERR;
    
    while(( i-- ) && (resault != BLE_OK))
    {
        resault = bm77_read_parma( cmd , respone ,length );
        if( resault != BLE_OK)
        {
            bm77_resert();
            sys_delay(500);
        }
    }
  
    return resault;
}


void bm77_set_parma(uint16_t cmd , uint8_t *data ,uint8_t length)
{
	uint8_t buffer[128] = {0x00} , cnt = 0;
    uint16_t timeoutCnt = 1000;
    
	buffer[cnt++] =  0x01;
	buffer[cnt++] =  0x27;
	buffer[cnt++] =  0xfc;
	buffer[cnt++] =  length+3;
	buffer[cnt++] =  ((cmd>>8)&0xff);
	buffer[cnt++] =  (cmd&0xff);
	buffer[cnt++] =  length;
	memcpy(buffer+cnt , data , length);
    cnt += length;
    
	bm77_send_data(buffer , cnt);

    while(( receiveFlag == FALSE) && timeoutCnt--)
    {
        BM77_DELAY(2);
        HAL_IWDG_Refresh(&hiwdg);
    }
    if( timeoutCnt == 0xFFFF)
    {
        log(ERR,"BM77 写入数据超时\n");
    }
    if( commandBuffer[6] == 0x00) 
    {
        log(WARN,"BM77写入操作成功 ,CMD =%x \n" , cmd);
        if(cmd==0)
        {
          SHOWME  SHOWME  SHOWME  SHOWME SHOWME
        }
    }
    else
    {
        log(WARN,"BM77写入操作失败 ,CMD =%x \n" , cmd);
    }
}
#define NORMAL 1
//3481F42EE3B2
uint8_t fake[6]={0X34,0X81,0XF4,0X2E,0XE3,0XB2};
//uint8_t fake[6]={0X11,0X22,0X33,0X44,0X55,0X66};
void bm77_write_mac(uint8_t *mac)
{
	#define BM77_MAC_CMD2 0
	 bm77_set_parma(BM77_MAC_CMD2 , mac , 6);
}

uint8_t bm77_read_mac(uint8_t *mac)
{
#if   NORMAL
	uint8_t tempMac[6] = {0x00};

	bm77_into_command_mode();

	if( bm77_read_data(BM77_MAC_CMD , tempMac , 6) != BLE_OK)
        {
        return BLE_INIT_ERR;
        }

	for(uint8_t i =0 ; i< 6 ; i++)
	{
		mac[i] = tempMac[5-i];
	}

	log_arry(DEBUG,"BM77 MAC is " , mac , 6 );

	bm77_out_command_mode();
	
	return BLE_OK;
#else

        
        bm77_write_mac(fake);
        
        for(uint8_t i =0 ; i< 6 ; i++)
	{
		mac[i] = fake[i];
	}

	log_arry(DEBUG,"BM77 MAC is " , mac , 6 );
        
        return BLE_OK;
#endif        
}

uint8_t bm77_read_version(uint8_t *pVersion)
{
    uint8_t mac[6];
      
    if( bm77_read_mac(mac) == BLE_OK)
    {
        pVersion[0] = 1;
        pVersion[1] = 4;
        pVersion[2] = 0;
	}
    else
    {
        pVersion[0] = 0;
        pVersion[1] = 0;
        pVersion[2] = 0;
    }
	return BLE_OK;
}

uint8_t bm77_set_default( void )
{

    uint8_t *deviceName;

    uint8_t bm77Name[DEVICE_NAME_LENG];
    

    bm77_into_command_mode();

    config.read(CFG_SYS_DEVICE_NAME, (void **)&deviceName);
    

    
    
    memcpy(bm77Name , deviceName , DEVICE_NAME_LENG);
    bm77_set_parma(BM77_NAME_CMD ,  bm77Name , DEVICE_NAME_LENG);
	


    bm77_out_command_mode();  
	
    bm77_resert();
	  
    return BLE_OK;
}


uint8_t bm77_write_msg(uint8_t *ptAddr , uint8_t *ptSenddata , uint16_t uLegnth , uint16_t dHandle)
{
	serial.puts(bm77_port , ptSenddata , uLegnth);
	return BLE_OK;
}

void bm77_init(void)
{
    bm77_port = serial.open("serial3");
    
    if( bm77_port == NULL)
    {
        beep.write(BEEP_ALARM);
        return ;
    }
    
    serial.init(bm77_port  , 115200 , bm77_receive_usart_byte);

	
    pin_ops.pin_mode(BM77_RST , PIN_MODE_OUTPUT);
    pin_ops.pin_mode(BM77_P20 , PIN_MODE_OUTPUT);

    pin_ops.pin_write(BM77_RST , PIN_HIGH);
    
    timer.creat(1 , TRUE ,bm77_receive_timer );
	
    bm77_normal_mode();

}
void bm77_force_normal(void)
{
	printf("\n%s\n",__FUNCTION__);
	sys_delay(50);
	bm77_normal_mode();

}

//MODULES_INIT_EXPORT(bm77_init , "bm77 module");


btDrvType	BM77Drv=
{
	.resert = bm77_resert,
	.read_mac = bm77_read_mac,
	.read_ver = bm77_read_version,
	.send = bm77_write_msg,
	.set_default = bm77_set_default,
	.force_normal = bm77_force_normal,
};

#endif
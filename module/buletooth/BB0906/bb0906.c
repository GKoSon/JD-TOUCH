#include "bb0906.h"
#include "bb0906_protocol.h"
#include "config.h"
#include "timer.h"
#include "beep.h"

static uint8_t prefix[] ={0x4C ,0X00,0X02,0X15};
static uint8_t UUID[] = {0x00 ,0x00,0x18,0xF0,0x00,0x00,0x10,0x00,0x80,0x00,0x00,0x80,0x5F,0x9B,0x34,0xFB};

static void *bb0906_port = NULL;


static uint8_t  responeTypeMsg[BLE_MODULE_CALSEE_LENG]="itaz";

static bleReceiveModeEnum ble_receive_mode = BLE_DATA_MODE;

static bleModuleReceiveCmdType     bleModuleReceiveCmd;
static bleModuleReceiveDataType    bleModuleReceiveData;


static uint32_t bleTimerCnt = 0;
static uint8_t bleTimerStart = 0;

static __IO uint8_t BleConnectCnt = 0;

/*****************************************************************************
** 
Internal variable declaration
*
*/ 
static uint8_t ble_write_mac( void *pucdata ,void *pucParma);
static uint8_t ble_get_mac( void *pucdata ,void *pucParma);
static uint8_t ble_get_version( void *pucdata ,void *pucParma);
static uint8_t ble_get_work_data( void *pucdata ,void *pucParma);
static uint8_t ble_get_raw_data( void *pucdata ,void *pucParma);
static void pb_clear_protocol( void );
void bb0906_resert( void );


uint8_t ble_module_check_crc( uint8_t *Data , uint8_t Length)
{
    uint8_t i =0,ucSum = 0;
    
    for( i = 0;i < Length; i++)
    {
        ucSum +=  Data[i];
    }
    
    return ((0xFF-ucSum)+0x01);
}

void ble_clear_timerflag( void )
{
    bleTimerCnt = 0 ; 
    bleTimerStart = 0;
}

void ble_recvive_timer( void )
{
    if( bleTimerStart )
    {
        if(bleTimerCnt++ > 10)
        {
            pb_clear_protocol();
            ble_clear_buffer();
            memset(&bleModuleReceiveCmd , 0x00 , sizeof(bleModuleReceiveCmdType));
            log(WARN,"[BLE]ble_recvive_timer out clear all\n");
        }
    }
}



void ble_receive_cmd_process( uint8_t usart_data)
{
    switch(bleModuleReceiveCmd.pos)
    {
        //message heard must be "itaz"
        case BLE_MODULE_HEARD1_POS:
        {
            
            bleModuleReceiveCmd.class[bleModuleReceiveCmd.cnt++] =  usart_data;
            if( bleModuleReceiveCmd.cnt >= BLE_MODULE_CALSEE_LENG)
            {
                if(aiot_strcmp(bleModuleReceiveCmd.class,responeTypeMsg , BLE_MODULE_CALSEE_LENG) == TRUE)
                {
                    bleModuleReceiveCmd.pos = BLE_MODULE_COMMAND_H_POS;
                }
                else
                {
                    bleModuleReceiveCmd.pos = BLE_MODULE_HEARD1_POS;
                    memset(&bleModuleReceiveCmd , 0x00 , sizeof(bleModuleReceiveCmdType));
                }
            }
        }break;
        //Get frame command
        case BLE_MODULE_COMMAND_H_POS:
        {
            bleModuleReceiveCmd.command = usart_data << 8 ;
            bleModuleReceiveCmd.pos = BLE_MODULE_COMMAND_L_POS;
        }break;
        case BLE_MODULE_COMMAND_L_POS:
        {
            bleModuleReceiveCmd.command |= usart_data;
            bleModuleReceiveCmd.pos = BLE_MODULE_RESPONSE_POS;
        }break; 
        //
        case BLE_MODULE_RESPONSE_POS:
        {
            bleModuleReceiveCmd.response = usart_data;
            bleModuleReceiveCmd.pos = BLE_MODULE_LENGTH_H_POS;
        }break;
        case BLE_MODULE_LENGTH_H_POS:
        {
            bleModuleReceiveCmd.length = usart_data << 8 ;
            bleModuleReceiveCmd.pos = BLE_MODULE_LENGTH_L_POS;
        }break;
        case BLE_MODULE_LENGTH_L_POS:
        {
            bleModuleReceiveCmd.length |= usart_data;
            if( bleModuleReceiveCmd.length > 0 )
            {
                bleModuleReceiveCmd.pos = BLE_MODULE_DATA_POS;
            }
            else
            {
                bleModuleReceiveCmd.pos = BLE_MODULE_CRC_POS;
            }
            bleModuleReceiveCmd.cnt = 0;
        }break; 
        case BLE_MODULE_DATA_POS:
        {
            bleModuleReceiveCmd.data[bleModuleReceiveCmd.cnt++] = usart_data ;
            if( bleModuleReceiveCmd.cnt ==bleModuleReceiveCmd.length)
            {
                bleModuleReceiveCmd.pos = BLE_MODULE_CRC_POS;
            } 
            else if( bleModuleReceiveCmd.cnt > bleModuleReceiveCmd.length)
            {
                bleModuleReceiveCmd.cnt = 0;
                bleModuleReceiveCmd.pos = BLE_MODULE_HEARD1_POS;
            }
            
        }break;    
        case BLE_MODULE_CRC_POS:
        {   
            uint8_t ucCheckCrc = 0;
            
            bleModuleReceiveCmd.crc = usart_data ;
            ucCheckCrc = ble_module_check_crc((uint8_t *)&bleModuleReceiveCmd.data , bleModuleReceiveCmd.length);
            if( ucCheckCrc == bleModuleReceiveCmd.crc)
            {
                bleModuleReceiveCmd.flag = TRUE;
                ble_clear_timerflag();
            }
            else
            {
                log(WARN,"Get frame crc is err, get crc=%x,calc crc =%x. \r\n" ,bleModuleReceiveCmd.crc , ucCheckCrc);  
                bleModuleReceiveCmd.pos = BLE_MODULE_HEARD1_POS;
            }
        }break;         
        
    }
}



static void pb_clear_protocol( void )
{
    memset(&bleModuleReceiveData , 0x00 , sizeof(bleModuleReceiveDataType));
    bleModuleReceiveData.pos =BLE_MODULE_HEARD1_POS;
    ble_clear_timerflag();
}




void bleDrv_receive_usart_byte( uint8_t usartData)
{

    bleTimerCnt = 0;
    bleTimerStart = 1;
    if( ble_receive_mode == BLE_DATA_MODE)
        BleReceiveUsartByteHandle(usartData);
    else
        ble_receive_cmd_process(usartData);

}


static bleCmdHandleArryType bleCmdTaskArry[]=
{
  {CMD_STATE            , ble_get_work_data},  
  {CMD_SETUP_BEACON     , ble_get_raw_data},
  {CMD_BEACON_ONOFF     , ble_get_raw_data},
  {CMD_ADV_SWITCHTIME   , ble_get_raw_data},
  {CMD_READADDR         , ble_get_mac},
  {CMD_RENAME           , ble_get_raw_data},
  {CMD_LE_DISCONNECT    , ble_get_raw_data},
  {CMD_LECONPARAMS      , ble_get_raw_data},
  {CMD_ORGL             , ble_get_raw_data},
  {CMD_VERSION          , ble_get_version},
  {CMD_ATTR_INDEX       , ble_get_raw_data},
  {CMD_LEADVPARAMS      , ble_get_raw_data},
  {CMD_MODBTADDR        , ble_write_mac},
  
};
const uint8_t bleCmdTasksCnt = sizeof (bleCmdTaskArry) / sizeof (bleCmdTaskArry[0]);

static uint8_t ble_write_mac( void *pucdata ,void *pucParma)
{
    bleModuleReceiveCmdType *BlePkt = (bleModuleReceiveCmdType *)pucdata;
    
    log(DEBUG,"Recv command is =%x ##\r\n" , BlePkt->command);
    memcpy(pucParma , BlePkt ,sizeof(bleModuleReceiveCmdType));

    return BLE_OK;
}

static uint8_t ble_get_mac( void *pucdata ,void *pucParma)
{
    bleModuleReceiveCmdType *BlePkt = (bleModuleReceiveCmdType *)pucdata;
    
    if(BlePkt->command ==  CMD_READADDR)
    {
        memcpy(pucParma , BlePkt->data , BlePkt->length);
    }
    
    return BLE_OK;
}

static uint8_t ble_get_version( void *pucdata ,void *pucParma)
{
    bleModuleReceiveCmdType *BlePkt = (bleModuleReceiveCmdType *)pucdata;
    memcpy(pucParma , BlePkt->data ,BlePkt->length);
    
    return BLE_OK;
}

static uint8_t ble_get_work_data( void *pucdata ,void *pucParma)
{
    bleModuleReceiveCmdType *BlePkt = (bleModuleReceiveCmdType *)pucdata;
    memcpy(pucParma , BlePkt->data ,2);
    
    return BLE_OK;
}

static uint8_t ble_get_raw_data( void *pucdata ,void *pucParma)
{
    bleModuleReceiveCmdType *BlePkt = (bleModuleReceiveCmdType *)pucdata;
    
    //log(DEBUG,"Recv command is =%x \r\n" , BlePkt->command);
    memcpy(pucParma , BlePkt ,sizeof(bleModuleReceiveCmdType));

    return BLE_OK;
}

uint8_t ble_command_process(bleModuleReceiveCmdType *BlePkt , void *pucParma)
{
    uint8_t idx =0;
    
    for(idx = 0; idx < bleCmdTasksCnt; idx++)
    {
        if( BlePkt->command == bleCmdTaskArry[idx].Id)
        {
            if(bleCmdTaskArry[idx].EventHandlerFn != NULL)
            {
                return (bleCmdTaskArry[idx].EventHandlerFn(BlePkt , pucParma));               
            }
            else
            {
                log(INFO,"This command has no callback  , CMD = %x.\r\n" , BlePkt->command);
                return BLE_NO_COMMAND_ERR;
            }
        }
    }
    log(INFO,"This command has no register. cmd = %x.\r\n" ,BlePkt->command);
    return BLE_COMMAND_NOBACK_ERR;
}

static uint8_t ble_wait_command_data( void *pucParma)
{
    uint8_t uRt = BLE_INIT_ERR;
    
    if(bleModuleReceiveCmd.flag == TRUE)
    {
        uRt = ble_command_process(&bleModuleReceiveCmd , pucParma);
        memset(&bleModuleReceiveCmd , 0x00 , sizeof(bleModuleReceiveCmdType));
        bleModuleReceiveCmd.pos =BLE_MODULE_HEARD1_POS;
    }
    
    return uRt;
}



static uint8_t ble_write_data(uint16_t dcommand,uint8_t *ptSenddata,uint16_t dLength)
{
    uint8_t PackBuff[BLEMODE_FARM_MAX+BLE_HEARD_SIZE];
    uint16_t Seq=0;
    
    if( dLength > BLEMODE_FARM_MAX)
    {
        log(WARN,"[BLE]ble_write_data dLength > BLEMODE_FARM_MAX, length = %d.\r\n" , dLength);
        return BLE_SEND_LENGTH_ERR;
    }
    
    PackBuff[Seq++] = BLE_CMD_TYPE1;  
    PackBuff[Seq++] = BLE_CMD_TYPE2;  
    PackBuff[Seq++] = BLE_CMD_TYPE3;  
    PackBuff[Seq++] = BLE_CMD_TYPE4;  
    PackBuff[Seq++] = MSB(dcommand);  
    PackBuff[Seq++] = LSB(dcommand);  
    PackBuff[Seq++] = MSB(dLength);  
    PackBuff[Seq++] = LSB(dLength);  
    
    if( ptSenddata != NULL)
    {
        memcpy(PackBuff+Seq , ptSenddata , dLength);
        Seq += dLength;
    }
    
    PackBuff[Seq++] = ble_module_check_crc(ptSenddata , dLength);

    serial.puts(bb0906_port , PackBuff , Seq);

    return 0;
}


uint8_t ble_sendRecv_data(uint16_t dcommand,uint8_t *ptSenddata,uint16_t dLength,uint8_t *ptReaddata , uint8_t ucReturnAck)
{
    uint8_t ucReptry = 3;  
    uint8_t ucOsCnt=30;
    uint8_t uRt = BLE_INIT_ERR;
    
      if( ucReturnAck )
    {   

        while(( ucReptry-- ) && (uRt != BLE_OK))
        {
            if(ble_receive_mode == BLE_CMD_MODE)
            {
                memset(&bleModuleReceiveCmd , 0x00 , sizeof(bleModuleReceiveCmdType));
            }

            uRt = ble_write_data(dcommand ,ptSenddata , dLength );
            if( uRt != BLE_OK)      //If return is no BLE_OK, it's means send message length is too long.
            {
                log(ERR,"[BLE]Send message is too long .Err = %d . \r\n" , uRt );
                return uRt;
            }
            uRt = BLE_INIT_ERR;

            //If time out ,or get right data. out of while.
            //while(( SendTimeOut.Cnt < 300) && (uRt != BLE_OK))
            while(( ucOsCnt--) && (uRt != BLE_OK))   // If use os.
            {
                uRt = ble_wait_command_data(ptReaddata);   
                // If the return is not BLE_OK and BLE_INIT_ERR that the data received a problem
                if((uRt != BLE_OK)&&( uRt != BLE_INIT_ERR)) 
                {            
                    log(WARN,"Err = %d ,Time = %d , Reptry = %d. \r\n" , uRt ,10*(30-ucOsCnt) , 30-ucOsCnt);
                }
                sys_delay(10); //If use OS.
            }
            HAL_IWDG_Refresh(&hiwdg);
           
        }
        //Off time out isr/
        //SetSendTimeOut(FALSE);
        //If repeat 5 times and no information is returned
        // that received is time out.
        if((ucReptry == 0xFF) && (uRt == BLE_INIT_ERR))
        {     
            bb0906_resert();
            uRt = BLE_RECV_TIMEOUT_ERR;
            log(WARN,"Ble return data time out.\n");           
        }
        
    }
    //Pass through data, don't need to return.
    else
    {
        uRt = ble_write_data(dcommand ,ptSenddata , dLength );
    }
    
    return  uRt;
}


uint8_t ble_write_command(uint16_t dcommand,uint8_t *ptSenddata,uint16_t dLength,uint8_t *ptReaddata)
{
    uint8_t uRt = BLE_INIT_ERR;
    
    ble_receive_mode = BLE_CMD_MODE;
    uRt = ble_sendRecv_data(dcommand , ptSenddata , dLength , ptReaddata , TRUE);
    ble_receive_mode = BLE_DATA_MODE;
    return uRt;
}

uint8_t ble_write_msg(uint8_t *ptAddr , uint8_t *ptSenddata , uint8_t uLegnth , uint16_t dHandle)
{
    uint8_t PackBuff[BLEMODE_FARM_MAX+BLE_HEARD_SIZE];
    uint8_t ReadBuff[BLEMODE_FARM_MAX+BLE_HEARD_SIZE];
    uint16_t Seq=0;
    uint8_t uRt = FALSE;

    //log(DEBUG,"[BLE]ble_write_msg %d %s\n" , uLegnth,ptSenddata);

    memcpy(PackBuff , (void const *)ptAddr , 6);
    Seq+=6;
    
    PackBuff[Seq++] = MSB(dHandle);  
    PackBuff[Seq++] = LSB(dHandle);
    PackBuff[Seq++] = 0x00;
 
    memcpy(PackBuff+Seq,ptSenddata, uLegnth );
    Seq += uLegnth;
    
    uRt = ble_sendRecv_data(0x004D , PackBuff , Seq , ReadBuff , FALSE);
    
    if(dHandle ==  0xFEC7)
    {
        log(DEBUG,"use wecat open .\r\n");
        PackBuff[8] = 0x01;    
        uRt = ble_sendRecv_data(0x004D , PackBuff , Seq , ReadBuff , FALSE);
    }
    return uRt;
}

uint8_t ble_read_version(uint8_t *pVersion)
{
    uint8_t BtVersion[BLE_VERSION_LENGTH]={0x00,0x00,0x00};
    uint8_t uRt = BLE_INIT_ERR;
    
    uRt = ble_write_command(CMD_VERSION ,NULL , 0 ,  BtVersion);
    if( uRt == BLE_OK)
    {
        memcpy(pVersion ,BtVersion , BLE_VERSION_LENGTH ) ;
        log_arry(DEBUG,"[BLE]Ble version" ,pVersion , BLE_VERSION_LENGTH);
    }
    else
    {
        log(WARN,"***************read version error*************\r\n");
        soft_system_resert(__FUNCTION__);
    }
    return uRt;
}

uint8_t ble_set_beacon(uint8_t *Prefix, uint8_t *UUID)
{
    bleModuleReceiveCmdType Bledata;
    uint8_t *mac;
    uint8_t SetLeConnectTime[8]={0x00 ,0x02 ,0x00 ,0x02 ,0x00 ,0x00 ,0x01 ,0x2c};
    uint8_t SendMsg[BLEMODE_FARM_MAX];
    uint8_t SendSize = 0;
    uint8_t uRt = BLE_INIT_ERR;
    uint16_t Major = 0 ,Minor = 0;
    char rssi = 70;
    uint8_t BeaconFlag = 1;         //Open beacon
    uint8_t BeaconAdvTime = 24;    //Set switch adv time
    
    config.read(CFG_BLE_MAC , (void **)&mac);
    Major = mac[2]<<8|mac[3];
    Minor = mac[4]<<8|mac[5];
    
    memset(SendMsg , 0x00 , BLEMODE_FARM_MAX);
    memset(&Bledata , 0x00 , sizeof(bleModuleReceiveCmdType));
    
    memcpy(SendMsg ,Prefix,BEACON_DEFAULT_PREFIX_LENG );
    SendSize+=BEACON_DEFAULT_PREFIX_LENG;
    memcpy(SendMsg+SendSize ,UUID,BEACON_DEFAULT_UUID_LENG );
    SendSize+=BEACON_DEFAULT_UUID_LENG;
    SendMsg[SendSize++]  =Major>>8;
    SendMsg[SendSize++]=Major;
    SendMsg[SendSize++]=Minor>>8;
    SendMsg[SendSize++]=Minor;
    SendMsg[SendSize++]= (uint8_t )rssi*-1;
    log(DEBUG,"[BLE]Major=%x , Minor=%x\r\n" ,Major, Minor);
    
    uRt = ble_write_command(CMD_SETUP_BEACON ,SendMsg , SendSize ,  (uint8_t *)&Bledata);
    if((Bledata.response == 1)&&(uRt == BLE_OK))
    {
        log(DEBUG,"[BLE]beacon is right.\r\n");
    }
    else
    {
        uRt = BLE_SET_BEACON_ERR;
        return uRt;
    }
    
    memset(&Bledata , 0x00 , sizeof(bleModuleReceiveCmdType));
    uRt = ble_write_command(CMD_BEACON_ONOFF ,&BeaconFlag , 1 ,  (uint8_t *)&Bledata);
    if((Bledata.response == 1)&&(uRt == BLE_OK))
    {
        log(DEBUG,"[BLE]beacon is open.\r\n");
    }
    else
    {
        uRt = BLE_OPEN_BEACON_ERR;     
        return uRt;
    }

    if(BeaconFlag == 1)
    {
        memset(&Bledata , 0x00 , sizeof(bleModuleReceiveCmdType));
        uRt = ble_write_command(CMD_ADV_SWITCHTIME ,&BeaconAdvTime , 1 ,  (uint8_t *)&Bledata);
        if((Bledata.response == 1)&&(uRt == BLE_OK))
        {
            log(DEBUG,"[BLE]beacon adv hasbeen set %d:%d ms.\r\n" , BeaconAdvTime,(BeaconAdvTime*125));
        }
         else
        {
            uRt = BLE_SET_BEACON_SWITCH_TIME_ERR;
            return uRt;
        }
        
        memset(&Bledata , 0x00 , sizeof(bleModuleReceiveCmdType));
        uRt = ble_write_command(CMD_LECONPARAMS ,SetLeConnectTime , 8 ,  (uint8_t *)&Bledata);
        if((Bledata.response == 1)&&(uRt == BLE_OK))
        {
            log(DEBUG,"[BLE]SetLeConnectTime success.\r\n");
        }
        else
        {
            uRt = BLE_SET_BEACON_SWITCH_TIME_ERR;
            return uRt;
        }

    }

    return uRt;
}

//SIM800 [DC 2C 26 00 2D 80 ]
uint8_t ble_set_mac(void)
{
    bleModuleReceiveCmdType Bledata;

    uint8_t uRt = BLE_INIT_ERR;

    uint8_t NewMac[6]={0x01,0x02,0x03,0x04,0x05,0x06};

    memset(&Bledata , 0x00 , sizeof(bleModuleReceiveCmdType));

    uRt = ble_write_command(CMD_MODBTADDR ,NewMac , 6,  (uint8_t *)&Bledata);

    if(( Bledata.data[0] == 0x00) &&( Bledata.response == 0x01))
    {
      log(DEBUG,"[BLE]ble_set_mac OK\n" );
    }
    else
    {
      log(DEBUG,"[BLE]ble_set_mac Fail\n" );
    }

    return uRt;

}


uint8_t ble_read_mac(uint8_t *pMac)
{
    uint8_t BtMac[BLE_MAC_LENGTH]={0x00,0x00,0x00,0x00,0x00,0x00};
    uint8_t uRt = BLE_INIT_ERR;


    //ble_set_mac(); 

    uRt = ble_write_command(CMD_READADDR ,NULL , 0 ,  BtMac);
    if( uRt == BLE_OK)
    {
        memcpy(pMac ,BtMac , BLE_MAC_LENGTH ) ;
        log_arry(DEBUG,"[BLE]module mac" , pMac , BLE_MAC_LENGTH);
    }
    else
    {
        log_err("[BLE]read mac error\n");
    }
    return uRt;
}

uint8_t ble_set_name( uint8_t *pName )
{
    bleModuleReceiveCmdType Bledata;

    uint8_t uRt = BLE_INIT_ERR;
    
    memset(&Bledata , 0x00 , sizeof(bleModuleReceiveCmdType));
    
    uRt = ble_write_command(CMD_RENAME ,pName , DEVICE_NAME_LENG,  (uint8_t *)&Bledata);
    
	if(( Bledata.data[0] == 0x00) &&( Bledata.response == 0x01))
	{
		log(DEBUG,"[BLE]named ok = %s\n" ,pName);
	}
    else
    {
        log(WARN,"[BLE]named fail =  %s\n" , pName);
    }

    return uRt;
    
}


uint8_t ble_set_mode( uint8_t BleMode )
{
    bleModuleReceiveCmdType Bledata;
    uint8_t uRt = BLE_INIT_ERR;
    
    uRt = ble_write_command(CMD_ATTR_INDEX ,&BleMode , 1 ,  (uint8_t *)&Bledata);
    if((Bledata.response == 1)&&(uRt == BLE_OK))
    {
        log(DEBUG,"[BLE]Ble mode set ok =%d.\r\n" , BleMode);
    }
    else
    {
        log(WARN,"[BLE]Ble mode set fail=%d.\r\n" , BleMode);
        uRt = BLE_SET_BEACON_SWITCH_TIME_ERR;
    }
    return uRt;
}

uint8_t ble_set_default( void )
{
    bleModuleReceiveCmdType Bledata;
    uint8_t uRt = BLE_INIT_ERR;
    
    ble_write_command(CMD_ORGL , NULL , 0 , (uint8_t *)&Bledata);
    if((Bledata.response == 1)&&(uRt == BLE_OK))
    {
        log(DEBUG,"[BLE]set default success.\r\n");
    }
    else
    {
        uRt = BLE_SET_BEACON_SWITCH_TIME_ERR;
        return uRt;
    }
    return uRt;  
  
}

void bb0906_resert( void )
{
    log(DEBUG,"[BLE]resert start\r\n");
    pin_ops.pin_write(BT_RESERT_PIN , PIN_HIGH);
    HAL_Delay(50);
    pin_ops.pin_write(BT_RESERT_PIN , PIN_LOW);
    HAL_Delay(100);
    HAL_IWDG_Refresh(&hiwdg);
    pin_ops.pin_write(BT_RESERT_PIN , PIN_HIGH);
    HAL_Delay(100);
    log(DEBUG,"[BLE]resert end\n");
}

uint8_t bb0906_set_default( void )
{
    uint8_t *deviceName ;

    config.read(CFG_SYS_DEVICE_NAME , (void **)&deviceName);

    ble_set_name(deviceName);
    
    ble_set_mode(5);

    ble_set_beacon(prefix ,UUID);

    bb0906_resert();
    
    return BLE_OK;
}

void bb0906_init( void )
{
    bb0906_port = serial.open("serial3");
    
    if( bb0906_port == NULL)
    {
        beep.write(BEEP_ALARM);
        log(ERR,"[BLE]]serial3 fail \r\n" );
        return ;
    }
    serial.init(bb0906_port  , 115200 ,bleDrv_receive_usart_byte);
    
    pin_ops.pin_mode(BT_RESERT_PIN , PIN_MODE_OUTPUT);
    
    timer.creat(1000 , TRUE ,ble_recvive_timer );
    
    bb0906_resert();
        
}

btDrvType    BB0906Drv=
{
    .resert = bb0906_resert,
    .read_mac = ble_read_mac,
    .read_ver = ble_read_version,
    .send = ble_write_msg,
    .set_default = bb0906_set_default,

};


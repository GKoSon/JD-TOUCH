//#if (BULETOOTH_MODE == MULTI_LINK)
#include "bb0906_protocol.h"
#include "config.h"

//Usart accept data buffer
static BleModuleAppDateType     BleModuleAppData;
//Accept app data buff
extern BleUserMsgType           BleUserMsg;


static uint8_t BleModuleCheckSum( uint8_t *Data , uint8_t Length)
{
    uint8_t i =0,ucSum = 0;
    
    for( i = 0;i < Length; i++)
    {
        ucSum +=  Data[i];
    }
    
    return ((0xFF-ucSum)+0x01);
}

void ble_clear_buffer( void )
{
    memset(&BleModuleAppData ,0x00 , sizeof(BleModuleAppDateType));
    memset(&BleUserMsg ,0x00 , sizeof(BleUserMsgType));
    ble_cleal_timer();
}

uint8_t BleCheckEndByte(BleAppMsgType *BleUsartData)
{
    if( ( BleUsartData->Data[BleUsartData->DataLength-1] == 0x7E)&&
        ( BleUsartData->Data[BleUsartData->DataLength-2] == 0x7D)&&
        ( BleUsartData->Data[BleUsartData->DataLength-3] == 0x7C)&&
        ( BleUsartData->Data[BleUsartData->DataLength-4] == 0x0B))
        {
             //printf("BleUsartData->DataLength%d\r\n",BleUsartData->DataLength);
             //log_arry(ERR,"蓝牙模块END "  , BleUsartData->Data ,BleUsartData->DataLength);
             ble_cleal_timer();
             return TRUE;
        }
                                        
    return FALSE;
}

/*
msb                             lsb   
+---+---+---+---+---+---+---+---+   
| 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |   
+---+---+---+---+---+---+---+---+   
|CommandClass_t (ACK_TYPE1)     | byte 0   
+-------------------------------+   
|CommandClass_t (ACK_TYPE2)     | byte 1   
+-------------------------------+   
|CommandClass_t (ACK_TYPE3)     | byte 2   
+-------------------------------+   
|CommandClass_t (ACK_TYPE4)     | byte 3   
+-------------------------------+   
|Command_t (CMD_MSB)            | byte 4   
+-------------------------------+   
|Command_t (CMD_LSB)            | byte 5   
+-------------------------------+   
| Response_t (RSP_FOR_CMD)      | byte 6   
+-------------------------------+   
|Packet Length (LEN_MSB)        | byte 7   
+-------------------------------+   
|Packet Length (LEN_LSB)        | byte 8   
+-------------------------------+   
|               .               |   
|               .               |   
|          Payload Data         | byte 9 to N - 1   
|               .               |   
|               .               |   
+-------------------------------+   
|        Payload Checksum       | byte N   
+-------------------------------+ 
*/

void BleReceiveUsartByteHandle( uint8_t ucData)
{

    switch(BleModuleAppData.BytePos)
    {
        case HEARD1_POS:
        {
            memset(&BleModuleAppData , 0x00 , sizeof(BleModuleAppDateType));
            if(ucData == 'i')
            {
                BleModuleAppData.BytePos = HEARD2_POS;
            }
        }break;
        case HEARD2_POS:
        {
            if(ucData == 't')
            {
                BleModuleAppData.BytePos = HEARD3_POS;
            }
        }break;
        case HEARD3_POS:
        {
            if(ucData == 'a')
            {
                BleModuleAppData.BytePos = HEARD4_POS;
            }
        }break;
        case HEARD4_POS:
        {
            if(ucData == 'z')
            {
                BleModuleAppData.BytePos = COMMAND_H_POS;
            }
        }break;
        case COMMAND_H_POS:
        {
            BleModuleAppData.Command = ucData << 8 ;
            BleModuleAppData.BytePos = COMMAND_L_POS;
        }break;
        case COMMAND_L_POS:
        {
            BleModuleAppData.Command |= ucData;
            BleModuleAppData.BytePos = RESPONSE_POS;
        }break;  
        case RESPONSE_POS:
        {
            BleModuleAppData.Response = ucData;
            BleModuleAppData.BytePos = LENGTH_H_POS;
        }break;
        case LENGTH_H_POS:
        {
            BleModuleAppData.Length = ucData << 8 ;
            BleModuleAppData.BytePos = LENGTH_L_POS;
        }break;
        case LENGTH_L_POS:
        {
            BleModuleAppData.Length |= ucData;
            BleModuleAppData.BytePos = ADDR_POS;
            if( BleModuleAppData.Length > BLEMODE_FARM_MAX)
            {
                memset(&BleModuleAppData , 0x00 , sizeof(BleModuleAppDateType));
            }
            BleModuleAppData.cnt = 0;
        }break; 
        case ADDR_POS:
        {
            BleModuleAppData.Msg.FormAddr[BleModuleAppData.cnt++] = ucData;
            if( BleModuleAppData.cnt == BLE_ADDR_SIZE)
            {
                BleModuleAppData.BytePos = HANDLE_H_POS;
            }
        }break; 
        case HANDLE_H_POS:
        {
            BleModuleAppData.Msg.Handle = ucData << 8;
            BleModuleAppData.BytePos = HANDLE_L_POS;
        }break;  
        case HANDLE_L_POS:
        {
            BleModuleAppData.Msg.Handle |= ucData ;
            BleModuleAppData.BytePos = WRITE_TYPE_POS;
        }break;   
        case WRITE_TYPE_POS:
        {
            BleModuleAppData.Msg.WriteType = ucData ;
            if(BleModuleAppData.Length >0 )
            {
                BleModuleAppData.BytePos = DATA_POS;
            }
            else
            {
                BleModuleAppData.BytePos = CRC_POS;
            }
            BleModuleAppData.cnt = 0;
        }break;     
        
        case DATA_POS:
        {
            BleModuleAppData.Msg.Data[BleModuleAppData.cnt++] = ucData ;
            if( BleModuleAppData.cnt ==BleModuleAppData.Length - BLE_DATA_HEARD_LENG)
            {
                BleModuleAppData.Msg.DatLength = BleModuleAppData.Length - BLE_DATA_HEARD_LENG;
                BleModuleAppData.BytePos = CRC_POS;
            } 
            else if( BleModuleAppData.cnt > BleModuleAppData.Length - BLE_DATA_HEARD_LENG)
            {
                BleModuleAppData.cnt = 0;
                BleModuleAppData.BytePos = HEARD1_POS;
            }
        }break;    
        case CRC_POS:
        {   
            uint8_t i,ucCheckCrc = 0;       
            BleModuleAppData.Crc = ucData ;
            ucCheckCrc = BleModuleCheckSum((uint8_t *)&BleModuleAppData.Msg , BleModuleAppData.Length);
            if( ucCheckCrc == BleModuleAppData.Crc)
            {
                if( ( BleModuleAppData.Command == CMD_LE_XFER2H) && ( BleModuleAppData.Response == 1))
                {
                    for( i = 0 ; i < BLEMODE_PHONE_MAX; i++ )
                    {
                        if(BleUserMsg.Msg[i].Flag != TRUE)
                        {
                            if(aiot_strcmp(BleModuleAppData.Msg.FormAddr,BleUserMsg.Msg[i].FormAddr , BLE_ADDR_SIZE) == TRUE)
                            {
                                if(BleUserMsg.Msg[i].DataLength + BleModuleAppData.Msg.DatLength < BLEMODE_FARM_MAX )
                                {
                                    memcpy(BleUserMsg.Msg[i].Data+BleUserMsg.Msg[i].DataLength, BleModuleAppData.Msg.Data , BleModuleAppData.Msg.DatLength);
                                    BleUserMsg.Msg[i].DataLength += BleModuleAppData.Msg.DatLength;
                                    
                                    //log(ERR,"BleUserMsg.Msg[%d].DataLength%d\r\n",i,BleUserMsg.Msg[i].DataLength);
                                    //log_arry(INFO,"蓝牙模块IN "  , BleModuleAppData.Msg.Data ,BleModuleAppData.Msg.DatLength);

                                    if( BleUserMsg.Msg[i].DataLength>60 && BleCheckEndByte(&BleUserMsg.Msg[i]) == TRUE)
                                    {
                                          BleUserMsg.Msg[i].Flag = TRUE;  
                                          static BaseType_t xHigherPriorityTaskWoken =  pdFALSE;;
                                          xSemaphoreGiveFromISR( xBtSemaphore, &xHigherPriorityTaskWoken );
                                          portEND_SWITCHING_ISR(xHigherPriorityTaskWoken );
                                    }
                                }
                                else
                                {
                                    memset(&BleUserMsg.Msg[i] , 0x00 , sizeof(BleAppMsgType));
                                }
                                break;
                            }
                        }
                        else
                        {
                            log(INFO,"蓝牙数据接收完成，正常处理\n");
                        }
                    }
                    if( i == BLEMODE_PHONE_MAX)
                  /*这是第一次的入口 因为BleUserMsg.Msg[i].Flag 虽然都没有拉起 
                  但是你进去以后发现无处安放 FormAddr 不对 第一次到这里赋予地址*/
                    {
                        //log(ERR,"1>\n");
                        memcpy(&BleUserMsg.Msg[BleUserMsg.Length] , &BleModuleAppData.Msg , sizeof(BleModuleAppMsgType));
                        BleUserMsg.Msg[BleUserMsg.Length].DataLength = BleModuleAppData.Msg.DatLength;
                        //log_arry(INFO,"蓝牙模块IN1 "  , BleModuleAppData.Msg.Data ,BleModuleAppData.Msg.DatLength);
                        if( BleCheckEndByte(&BleUserMsg.Msg[BleUserMsg.Length]) == TRUE)
                        {   /*有可能短短的一包就结束战斗了 可以无视*/
                             log(ERR,"1>万分之一的短包\n");
                             static BaseType_t xHigherPriorityTaskWoken =  pdFALSE;;                                
                             BleUserMsg.Msg[i].Flag = TRUE;   
                             xSemaphoreGiveFromISR( xBtSemaphore, &xHigherPriorityTaskWoken );
                             portEND_SWITCHING_ISR(xHigherPriorityTaskWoken );
                        }
                        BleUserMsg.Length++;
                        if( BleUserMsg.Length >= BLEMODE_PHONE_MAX)
                        {
                            BleUserMsg.Length = 0;
                        }
                    }
                }
                
            }
            else
            {
                log(WARN,"Get frame crc is err, get crc=%x,calc crc =%x. \r\n" ,BleModuleAppData.Crc , ucCheckCrc);  
            }
            memset(&BleModuleAppData , 0x00 , sizeof(BleModuleAppDateType));
            BleModuleAppData.BytePos =HEARD1_POS;
        }break;         
        
    }
}
//#endif

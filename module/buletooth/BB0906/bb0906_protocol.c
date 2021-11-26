#include "bb0906_protocol.h"
#include "config.h"


BleModuleAppDateType     BleModuleAppData;




void ble_clear_buffer( void )
{
    memset(&BleModuleAppData ,0x00 , sizeof(BleModuleAppDateType));
    memset(pag ,0x00 , 4*sizeof(ProtData_T));
    //ble_cleal_timer();
}

void ble_easyclear_buffer( char i )
{
    memset(&BleModuleAppData ,0x00 , sizeof(BleModuleAppDateType));
    memset(&pag[i] , 0x00 , sizeof(ProtData_T));
   // ble_cleal_timer();
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
    char i=0;
    static char ch =0;
    static char Rlen=0;
    switch(BleModuleAppData.BytePos)
    {
        case HEARD1_POS:
        {
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
            if( BleModuleAppData.Length > 40)
            {   NEVERSHOW
                ble_clear_buffer();
            }

        }break; 
        
        case ADDR_POS:/*开始进入数据壳子*/
        {
	
            BleModuleAppData.Msg.hdr.FormAddr[Rlen++] = ucData;
            if( Rlen == BLE_ADDR_SIZE)
            {
                BleModuleAppData.BytePos = HANDLE_H_POS;
                Rlen = 0;
            }
        }break; 
        case HANDLE_H_POS:
        {
            BleModuleAppData.Msg.hdr.Handle = ucData << 8;
            BleModuleAppData.BytePos = HANDLE_L_POS;
        }break;  
        case HANDLE_L_POS:
        {
            BleModuleAppData.Msg.hdr.Handle |= ucData ;
            BleModuleAppData.BytePos = WRITE_TYPE_POS;
        }break;   
        case WRITE_TYPE_POS:
        {
 
            if(BleModuleAppData.Length >0 )
            {
                BleModuleAppData.BytePos = DATA_POS;
            }
            else
            {
                BleModuleAppData.BytePos = CRC_POS;
                NEVERSHOW
            }

        }break;     
        
        case DATA_POS:/*开始进入数据*/
        {     	
            BleModuleAppData.Msg.Data[Rlen++] = ucData ;
            if( Rlen == BleModuleAppData.Length - BLE_DATA_HEARD_LENG)
            {
                BleModuleAppData.Msg.DatLength = Rlen;
                BleModuleAppData.BytePos = CRC_POS;
                Rlen = 0;

            } 

        }break;    
        case CRC_POS:/*all in   BleModuleAppData.Msg.Data*/
        {   
            BleModuleAppData.BytePos = DATA_TRANS;
        };   
        case DATA_TRANS:
        {   
              
              for( i = 0 ; i < BLEMODE_PHONE_MAX; i++ )
              {
                      if(is_arr_same(BleModuleAppData.Msg.hdr.FormAddr,pag[i].hdr.FormAddr , BLE_ADDR_SIZE) == TRUE)
                      {
                        
                        if(pag[i].hdr.WriteType!=0XFF)
                        {
                          
                              memcpy(pag[i].POS25V + pag[i].POS25Vlen,    BleModuleAppData.Msg.Data  , BleModuleAppData.Msg.DatLength );
                              pag[i].POS25Vlen += BleModuleAppData.Msg.DatLength;
   

                              if(pag[i].POS25Vlen +2  >= pag[i].POS1415_len)
                              {
                                SHOWME 
                                pag[i].hdr.WriteType=0XFF; 
                                release_sig();
                             
                              }
                        }
                        else
                        { memset(&BleModuleAppData ,0x00 , sizeof(BleModuleAppDateType));NEVERSHOW}
                        
                        i=100;
                        break;
                     }
              }/*第一次过来这个for进去马上出来  因为if进不去的 此时pag全是0* 所以第一次就是进下面的*/
              if( i == BLEMODE_PHONE_MAX)
              {
                      memset( &pag[ch]      ,0 ,sizeof(ProtData_T));
                      memcpy( &pag[ch].hdr  ,&BleModuleAppData.Msg.hdr  ,sizeof(AppDataHeardType));
                      memcpy( &pag[ch]      ,&BleModuleAppData.Msg.Data ,BleModuleAppData.Msg.DatLength);
                      
                      if( pag[ch].POS11_head != 0XAA )
                      {
                        NEVERSHOW ble_clear_buffer();break;
                      }
                      else
                      {
                          //pag[ch].POS1415_len =    exchangeBytes(  pag[ch].POS1415_len);//全包长度 设定值
                         // pag[ch].POS2122T    =    exchangeBytes(  pag[ch].POS2122T);
                          //pag[ch].POS2324L    =    exchangeBytes(  pag[ch].POS2324L);//小包 不管

                          pag[ch].POS25Vlen   =     BleModuleAppData.Msg.DatLength - 9;//POS25Vlen 包含了CRC
                          
                        
                          if(pag[ch].POS25Vlen + 4 == pag[ch].POS1415_len + 2)
                          { pag[ch].hdr.WriteType=0XFF; release_sig();}
                                                     
                           ch = (ch + 1)% BLEMODE_PHONE_MAX ;
                      }
              }
              
              
          memset(&BleModuleAppData ,0x00 , sizeof(BleModuleAppDateType));
                    
        }break;       
    }
}

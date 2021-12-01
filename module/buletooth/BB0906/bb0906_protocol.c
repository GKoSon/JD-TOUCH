#include "bb0906_protocol.h"
#include "config.h"


BleModuleAppDateType     BleModuleAppData;




void ble_clear_buffer( void )
{
    memset(&BleModuleAppData ,0x00 , sizeof(BleModuleAppDateType));
    
    //ble_cleal_timer();
}

void ble_easyclear_buffer( char i )
{
    memset(&BleModuleAppData ,0x00 , sizeof(BleModuleAppDateType));
    //memset(&pag[i] , 0x00 , sizeof(ProtData_T));
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

void all_printf(void *p,int len)
{
  uint8_t *d = (uint8_t *)p;
  for(int i=0;i<len;i++)
    printf("%02X-",d[i]);
}
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
              

all_printf(&BleModuleAppData,sizeof(BleModuleAppData));

                      memset( &ble_app[0]      ,0 ,sizeof(BleProtData));
                      
                      memcpy( &ble_app[0].hdr  ,&BleModuleAppData.Msg.hdr  ,sizeof(AppDataHeardType));
                      memcpy( &ble_app[0] ,&BleModuleAppData.Msg.Data ,BleModuleAppData.Msg.DatLength);
                      ble_app[0].alllen = 0xFF;
        release_sig();      
              
          memset(&BleModuleAppData ,0x00 , sizeof(BleModuleAppDateType));
                    
        }break;       
    }
}

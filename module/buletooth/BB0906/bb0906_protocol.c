#include "bb0906_protocol.h"
#include "config.h"


BleModuleAppDateType     BleModuleAppData;



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
void ble_receive_data_process( uint8_t ucData)
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
            {NEVERSHOW} 

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
//test
#if 0
if(BleModuleAppData.Msg.Data[0]==0x11){
wrirenfc=1;NEVERSHOW
} else if(BleModuleAppData.Msg.Data[0]==0x22)
{wrirenfc=0;NEVERSHOW}
#endif

#if 0
all_printf(&BleModuleAppData.Msg,sizeof(BleModuleAppData)-23);
printf("\r\n");
#endif

              for( i = 0 ; i < BLEMODE_PHONE_MAX; i++ )
              {
                      if(aiot_strcmp(BleModuleAppData.Msg.hdr.FormAddr,ble_app[i].hdr.FormAddr , BLE_ADDR_SIZE) == TRUE)
                      {
                            /*连续过来的下一包*/
                          struct _BleProtData_
                          {
                            Byte0    id;/*头部第一个字节 当前只完成msgid【设备返回时候维持一样】*/
                            uint8_t  cmd;/*头部第二个字节 表示执行的命令【设备返回时是0X01如果异常是0X0F】*/
                            Byte2    num;/*头部第三个字节 表示传输中的序列【设备返回时是0X10因为我短小的一帧】*/
                            uint8_t  len;/*头部第四个字节 表示传输中的序列 当前这一帧的长度*/

                            uint8_t  body[16];
                          }hb;
                          

                          memcpy( &hb       , &BleModuleAppData.Msg.Data ,BleModuleAppData.Msg.DatLength);/*DatLength 是29-9 就是20*/

                          memcpy( ble_app[i].body+ble_app[i].bodylen        ,hb.body,    hb.len);

                          //log(INFO,"之前bodylen=%d 现在增加%d " ,bodylen,hb.len);

                          ble_app[i].bodylen += hb.len;
                          //log(INFO,"第N次 展示序列号 （%d:%d)\n" ,hb.num.byte.seqall,hb.num.byte.seqid);


                          if(hb.num.byte.seqall==hb.num.byte.seqid)
                          {
                              //NEVERSHOW
                              ble_app[i].hdr.WriteType = 0xFF;
                              ble_app[i].num=hb.num;
                              release_sig();    
                          } 
                      i = 100;/*结束流程 防止进入到下面if( i == BLEMODE_PHONE_MAX)*/
                      }
              } /*第一次过来这个for进去马上出来  因为if进不去的 此时ble_app全是0* 所以第一次就是进下面的*/



              if(i == BLEMODE_PHONE_MAX)
              {
/*4D-00-01-[1D-00]-5F-93-DD-CB-30-71-F1-2A-00-// 03-03-21-10- / 08-C0-C4-07-18-C0-C4-07-20-A3-C8-85-F1-D7-2F-2A-00-00-00-00-00-00-00-00-00-00-00-00-00-00-00-00-00-00-00-00-14-00-0F-*/                
                      memset( &ble_app[ch]         ,0                           ,sizeof(BleProtData));
                      memcpy( &ble_app[ch].hdr     ,&BleModuleAppData.Msg.hdr   ,sizeof(AppDataHeardType));
                      memcpy( &ble_app[ch]         ,&BleModuleAppData.Msg.Data  ,BleModuleAppData.Msg.DatLength);
                      /*正好一包就搞定*/
                      //log(INFO,"第1次 展示序列号 （%d:%d)（chg=%d）\n" ,ble_app[ch].num.byte.seqall,ble_app[ch].num.byte.seqid,ch);
                      if(ble_app[ch].num.byte.seqall==ble_app[ch].num.byte.seqid)
                      {
                          log(INFO,"第一次 只是收到一包数据 就可以打完收工啦\n");
                          ble_app[ch].hdr.WriteType = 0xFF;
                          release_sig();    
                          NEVERSHOW
                      } else {
                           //log(INFO,"第一次 展示body长度 %d/%d\n" , BleModuleAppData.Msg.DatLength  - 4,ble_app[ch].len);            
                           ble_app[ch].bodylen = BleModuleAppData.Msg.DatLength - 4; /*29-9  上面的1D就是29 而DatLength已经提前-9了 BLE_DATA_HEARD_LENG   */
                           ble_app[ch].bodylen = ble_app[ch].len;
                      }
                      
                      
                      
              ch = (ch + 1)% BLEMODE_PHONE_MAX ;
              }
              
           
        memset(&BleModuleAppData ,0x00 , sizeof(BleModuleAppDateType));            
        }break; 
     
    }
}

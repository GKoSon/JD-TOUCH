#include "buletooth.h"
#include "bb0906.h"
#include "timer.h"
#include "sysCfg.h"
#include "config.h"


btDrvType    *btDrv = NULL;



void bt_drive_resert( void )
{
  btDrv->resert();
}

uint8_t bt_drive_read_mac( uint8_t *mac )
{
  return (btDrv->read_mac(mac));
}

uint32_t bt_drive_read_version( void )
{
  uint8_t ver[BLE_VERSION_LENGTH];

  btDrv->read_ver(ver);

  return (ver[0]*100+ver[1]*10+ver[2]);
}

void bt_drive_send( ProtData_T *pag , uint8_t *data , uint16_t length  )
{
    //show_ProtData(pag);
    //log_arry(ERR,"bt_drive_send  "  ,data,length);
    btDrv->send(pag->hdr.FormAddr , data , length , pag->hdr.Handle);

}

void bt_drive_set_default( void )
{
  btDrv->set_default();
}

void bt_drive_reinit( void )
{
  log(DEBUG,"[BLE]bt_drive_reinit\n");
  btModule.resert();
  sys_delay(100);
  btModule.set_default();
}





buletoothDriveType btModule =
{
  .init   = bt_drive_reinit,
  .send   = bt_drive_send,
  .read_mac = bt_drive_read_mac,
  .read_version = bt_drive_read_version,
  .set_default = bt_drive_set_default,
  .resert = bt_drive_resert,

};

void bluetooth_drv_init( void )
{
  log(DEBUG,"[BLE]bluetooth_drv_init\n");
  btDrv = &BB0906Drv;
  bb0906_init();
}
void Clear_ProtBuf(void)
{
  memset(&pag[0] , 0x00 , sizeof(ProtData_T));
}

char is_crc_ok( ProtData_T *opag )
{
  
    printf("offsetof(ProtData_T,POS2122T)===========%d\r\n",offsetof(ProtData_T,POS2122T));
  
    uint16_t size,crc16;
    
    ProtData_T pag;
    
    memcpy(&pag,opag,sizeof(ProtData_T));
    
	size = 5 + pag.POS1415_len;

    
    //pag.POS1415_len = exchangeBytes( pag.POS1415_len);
    //pag.POS2122T =    exchangeBytes( pag.POS2122T);
    //pag.POS2324L =    exchangeBytes( pag.POS2324L);
    
    memset(fb , 0 ,  sizeof(fb));
    memcpy(fb , &pag ,size );

    crc16 =  CRC16_CCITT(fb ,size );
    
    //crc16 =  crc16_ccitt(fb ,size );  
    //log_arry(ERR,"is_crc_ok"  ,fb ,size);
    
    printf("size=%d	crc16 =0X%04X  pag.POS31_CRC=0X%04X",	size,crc16 ,pag.POS31_CRC);
    
	if(crc16 == pag.POS31_CRC)
		return 1;
	else
		return 0;
}


void release_sig(void)
{
  static BaseType_t xHigherPriorityTaskWoken =  pdFALSE;;
  xSemaphoreGiveFromISR( xBtSemaphore, &xHigherPriorityTaskWoken );
  portEND_SWITCHING_ISR(xHigherPriorityTaskWoken );
}



void show_ProtData(ProtData_T *pag)
{
  log(DEBUG,"\n****************************************** \n");
  log(DEBUG,"pag->POS11_head= [%02X]\n" , pag->POS11_head);
  log(DEBUG,"pag->POS12_num= [%02X]\n" , pag->POS12_num);
  log(DEBUG,"pag->POS13_rnum= [%02X]\n" , pag->POS13_rnum);
  log(ERR,"pag->POS1415_len= [%04X]\n" , pag->POS1415_len);
  log(DEBUG,"pag->POS2122T= [%04X]\n" , pag->POS2122T);
  log(ERR,"pag->POS2324L= [%04X]\n" , pag->POS2324L);
  log_arry(DEBUG,"pag->POS25V" ,pag->POS25V , pag->POS2324L + 2);
  log(INFO,"pag->POS31_CRC= [%04X]\n" , pag->POS31_CRC);
  log(INFO,"pag->POS25Vlen= [%04X]\n" , pag->POS25Vlen);
  log_arry(INFO,"pag->hdr.FormAddr" ,pag->hdr.FormAddr , 6);
  log(INFO,"pag->hdr.Handle= [%04X]\n" ,pag->hdr.Handle);
  log(ERR,"pag->hdr.WriteType= [%02X]\n" ,pag->hdr.WriteType);
  log(DEBUG,"\n****************************************** \n");
}
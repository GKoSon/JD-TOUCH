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
/*
一般是后面2个参数即可 把数据发出去
这个模组需要前面6个HEX作为头部地址 需要2个HEX作为发出去的句柄
这个都是是手机APP发消息过来的时候 模组随机分配的
这个模组永远是从机 等收到消息以后 才能发消息
*/
void bt_drive_send( uint8_t *mode_head_addr_six_hex ,uint16_t mode_head_handle, uint8_t *data , uint16_t length)
{

    log_arry(ERR,"bt_drive_send"  ,data ,length);
    btDrv->send(mode_head_addr_six_hex , data , length , mode_head_handle);

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


void release_sig(void)
{
  static BaseType_t xHigherPriorityTaskWoken =  pdFALSE;;
  xSemaphoreGiveFromISR( xBtSemaphore, &xHigherPriorityTaskWoken );
  portEND_SWITCHING_ISR(xHigherPriorityTaskWoken );
}

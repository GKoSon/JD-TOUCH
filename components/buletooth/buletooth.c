#include "buletooth.h"
#include "bb0906.h"
#include "timer.h"
#include "sysCfg.h"


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

void bt_drive_send( BleAppMsgType *prot , uint8_t *data , uint16_t length  )
{
  btDrv->send(prot->FormAddr , data , length , prot->Handle);
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

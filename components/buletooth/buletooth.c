#include "buletooth.h"
#include "bb0906.h"
#include "bm77.h"
#include "timer.h"
#include "sysCfg.h"


btDrvType	*btDrv = NULL;



void bt_drive_timer_isr(void)
{
    //btDrv->timer_isr();
}

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

void bt_drive_init( void )
{

    log(INFO,"准备弥补BLE bug 以前啥也没做\n");
    btModule.resert();
    sys_delay(100);
    btModule.set_default();
}

void bt_drive_force_normal(void)
{
    btDrv->force_normal();
}

buletoothDriveType btModule =
{
    .init   = bt_drive_init,
    .send   = bt_drive_send,
    .read_mac = bt_drive_read_mac,
    .read_version = bt_drive_read_version,
    .set_default = bt_drive_set_default,
    .resert = bt_drive_resert,
    .force_normal = bt_drive_force_normal,

};


void bluetooth_drv_init( void )
{
	uint32_t runModule = config.read(CFG_SYS_BLE_TYPE , NULL);
	
	
	if( runModule == 0 )
	{
		log(DEBUG,"蓝牙模块使用BM77\n");
		btDrv = &BM77Drv;
		bm77_init();
	}
	else if( runModule == 1 )
	{
		log(DEBUG,"蓝牙模块使用BB0906\n");
		btDrv = &BB0906Drv;
		bb0906_init();
	}else
	{
		log(INFO,"蓝牙模块选择未初始化 , 不能运行，恢复出厂设置\n");
		config.write(CFG_SET_RESTORE , NULL , FALSE);
		
	}
}

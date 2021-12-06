#include "BleDataHandle.h"
#include "config.h"
#include "buletooth.h"
#include "beep.h"
#include "open_door.h"
#include "permi_list.h"
#include "open_log.h"
#include "magnet.h"
#include "buletooth.h"
#include "timer.h"

xTaskHandle              BtHandle;

void bt_check_work( void )
{
    uint16_t ver = 0;
    
    if( ( ver = btModule.read_version()) != 140)
    {
        beep.write(BEEP_ALARM);
        log_err("[BLE]蓝牙模块工作不正常\n");
    }
    log(DEBUG,"[BLE]module version:%d\n" , ver );
    
}

void ble_data_process( void const *pvParameters)
{
    configASSERT( ( ( unsigned long ) pvParameters ) == 0 );
    
    btModule.init();
    
    bt_check_work();
    
    while(1)
    {
        if( xSemaphoreTake( xBtSemaphore, 1000 ) == pdTRUE )
        {          
          
            for(uint8_t i = 0 ; i < BLE_CONNECT_MAX_NUM ; i++)
            {
                if(ble_app[i].hdr.WriteType == 0xFF)
                { 
                  BleDataHandle(&ble_app[i]);
                  memset(&ble_app[i] , 0x00 , sizeof(BleProtData)); 
                }
            }
        }

        task_keep_alive(TASK_BT_BIT);  
    }
}


void creat_buletooth_task( void )
{
    osThreadDef( bt, ble_data_process , osPriorityHigh, 0, configMINIMAL_STACK_SIZE*7);
    BtHandle = osThreadCreate(osThread(bt), NULL);
    configASSERT(BtHandle);  
}
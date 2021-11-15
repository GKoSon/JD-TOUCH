
#include "BleDataHandle.h"
#include "config.h"
#include "buletooth.h"
#include "beep.h"
#include "open_door.h"
#include "tempwd.h"
#include "permi_list.h"
#include "open_log.h"
#include "magnet.h"
#include "ladder.h"
#include "buletooth.h"
#include "timer.h"

xTaskHandle              BtHandle;
uint8_t                  btOpenDoorPhoneClearTimerHdl = 0xFF;


void bt_check_work( void )
{
    uint16_t ver = 0;
    
    if( ( ver = btModule.read_version()) != 140)
    {
        beep.write_base(&checkErr);
        log_err("蓝牙模块工作不正常\n");
    }
    log(DEBUG,"BT module version:%d\n" , ver );
    
}

void bt_open_door_phone_timer_isr(void)
{
    memset(btPhone ,0x00 , 20);
    timer.stop(btOpenDoorPhoneClearTimerHdl);
    log(DEBUG,"bt phone number clear\n");
}

void ble_old_process( void const *pvParameters)
{
    configASSERT( ( ( unsigned long ) pvParameters ) == 0 );
    
    btOpenDoorPhoneClearTimerHdl = timer.creat(1000 , FALSE , bt_open_door_phone_timer_isr );
    
    btModule.init();
    
    bt_check_work();
    while(1)
    {
        if( xSemaphoreTake( xBtSemaphore, 1000 ) == pdTRUE )
        {
            for(uint8_t i = 0 ; i < BLE_CONNECT_MAX_NUM ; i++)
            {
                if(BleUserMsg.Msg[i].Flag == TRUE)
                {
                    BleDataProcess(&BleUserMsg.Msg[i]);
                    memset(&BleUserMsg.Msg[i] , 0x00 , sizeof(BleAppMsgType));
                    task_keep_alive(TASK_BT_BIT); 
                }
            }
        }

        task_keep_alive(TASK_BT_BIT);  
    }
}

void creat_buletooth_task( void )
{
    osThreadDef( bt, ble_old_process , osPriorityHigh, 0, configMINIMAL_STACK_SIZE*12);
    BtHandle = osThreadCreate(osThread(bt), NULL);
    configASSERT(BtHandle);  
}
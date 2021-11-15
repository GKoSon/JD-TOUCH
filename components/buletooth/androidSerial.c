#include "androidSerial.h"
#include "serialSend.h"
#include "beep.h"
#include "serial.h"
#include "modules_init.h"
#include "timer.h"
#include "unit.h"
#include "open_log.h"
#include "cJSON.h"
#include "sysCfg.h"
#include "open_door.h"
#include "permi_list.h"
#include "tempwd.h"
#include "sysCntSave.h"
#include "BleDataHandle.h"
#include "config.h"
#include "crc32.h"

static portBASE_TYPE           xHigherPriorityTaskWoken = pdFALSE;
static xTaskHandle 		    androidSerialTask;
void 					        *androidSerialPort = NULL;
__IO uint8_t                    androidSerialreceiveTimeStart = FALSE;
__IO uint32_t                   androidSerialreceiveTimecnt = 0;
androidSerialReceiveDataType  	androidSerialReceiveData;

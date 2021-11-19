#include "main.h"
#include "stm32l4xx_hal.h"
#include "cmsis_os.h"
#include "adc.h"
#include "iwdg.h"
#include "rtc.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"
//#include "syscfg.h"
#include "buletooth.h"


//uint8_t               mqttsendbuf[2048]@(0x10000000);
uint8_t                   mqttreadbuf[2048]@(0x10000800);    
uint8_t                                fb[4096]@(0x10001000);
uint8_t                          gDATABUF[2048]@(0x10002000);
usartReceiveDataType                receiveData@(0x10002800);
//SystemConfigType                            cfg@(0x10003200);
//char                     rxOtaData[2048]@(0x10003800);
#if 0
char                     mqttSocketBuffer[2048]@(0x10004000);
BleUserMsgType                       BleUserMsg@(0x10004800);
#endif
char                     mqttSocketBuffer[2048]@(0x10003200);
BleUserMsgType                       BleUserMsg@(0x10004000);
//uint8_t                         COMMONSTATIC[1024]@(0x10004800);
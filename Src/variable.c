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
#include "syscfg.h"
#include "buletooth.h"


_SHType                                  SHType@(0x10000000);
uint8_t                       mqttreadbuf[2048]@(0x10000800);    
uint8_t                                fb[4096]@(0x10001000);
usartReceiveDataType                receiveData@(0x10002000);//ÐÞ¸ÄRECEIVE_MAX 2048¼õÐ¡
SystemConfigType                            cfg@(0x10002800);
char 			        rxOtaData[4096]@(0x10003000);

char	                 mqttSocketBuffer[2048]@(0x10004000);
ProtData_T                               pag[2]@(0x10004800);

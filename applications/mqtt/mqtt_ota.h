#ifndef _MQTT_OTA_H_
#define _MQTT_OTA_H_

#define     USER_FLASH_END_ADDRESS            0x0807A000

#define        UPG_MARK                0xAABB55AA
#define        OTA_OK                    0

#define ota_log(level, fmt, ...)     log(level , fmt, ## __VA_ARGS__) 
//#define ota_log(level, fmt, ...)   





void creat_mqtt_ota_task( void );

#endif

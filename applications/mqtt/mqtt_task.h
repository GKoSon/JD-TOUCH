#ifndef _MQTT_TASK_H_
#define _MQTT_TASK_H_

#define MQTT_TASK 1

#include "mqtt_client.h"


enum
{
  MQTT_INIT,
  MQTT_CONNECT_TCP,
  MQTT_CONNECT_MQTT,
  MQTT_SUBSCRIBE,
  MQTT_DEVINFO,
  MQTT_ALIEVE,
  MQTT_FILTER,
  MQTT_OK,
};

extern uint8_t            mqttreadbuf[2048];    
extern mqttClientType     client;
extern uint8_t            pubTopName[64];

extern void create_mqtt_task( void );
extern uint8_t mqtt_network_normal( void );
extern void mqtt_set_resert( void );

#endif


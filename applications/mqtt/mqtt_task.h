#ifndef _MQTT_TASK_H_
#define _MQTT_TASK_H_

#define MQTT_TASK 1

#include "mqtt_client.h"


enum
{
  GMQTT_INIT,
  GMQTT_CONNECT_TCP,
  GMQTT_CONNECT_MQTT,
  GMQTT_SUBSCRIBE,
  GMQTT_DEVINFO,
  GMQTT_ALIEVE,
  GMQTT_VER,
  GMQTT_FILTER,
  GMQTT_OK,
};

extern uint8_t            mqttreadbuf[2048];    
extern mqttClientType     client;
extern uint8_t            pubTopName[64];

extern void create_mqtt_task( void );
extern uint8_t mqtt_network_normal( void );
extern void mqtt_set_resert( void );

#endif


#ifndef _MQTT_CLIENT_H_
#define _MQTT_CLIENT_H_

#include "MQTTFreeRTOS.h"
#include "MQTTPacket.h"
#include "stdio.h"
#include "m_list.h"

#define MAX_PACKET_ID 65535 /* according to the MQTT specification - do not change! */


#if !defined(MAX_MESSAGE_HANDLERS)
#define MAX_MESSAGE_HANDLERS  5 /* redefinable - how many subscriptions do you want? */
#define GMAX_MESSAGE_HANDLERS 4 /* redefinable - how many subscriptions do you want? */
#endif

enum QoS { QOS0, QOS1, QOS2, SUBFAIL=0x80 };

/* all failure return codes must be negative */
enum returnCode { BUFFER_OVERFLOW = -2, FAILURE = -1, MQTT_SUCCESS = 0 };

#define offset(type, parma) ((unsigned long)(&(((type *)0)->parma)))


typedef struct MQTTMessage
{
    enum QoS	qos;
    uint8_t	retained;
    uint8_t	dup;
    uint16_t	id;
    void	*payload;
    size_t	payloadlen;
} MQTTMessage;

typedef struct MessageData
{  
    MQTTMessage* message;
    MQTTString* topicName;
} MessageData;


typedef void (*messageHandler)(void * , MessageData*);
typedef struct _mqttClient
{
	uint32_t	nextPacketId;
	uint32_t	commandTimeoutMs;
    uint32_t    dataRepeatTimeSec;
	size_t		readbufSize;
	uint8_t		*readbuf;
	uint32_t	keepAliveInterval;
	char		pingOutStanding;
	int			isconnected;
	int			cleansession;

	struct MessageHandlers
	{
		uint8_t *topicFilter;
		void (*fp) (void * , MessageData*);
	} messageHandlers[MAX_MESSAGE_HANDLERS];      /* Message handlers are indexed by subscription topic */

	void (*defaultMessageHandler) (MessageData*);
	
	
	Network*	ipstack;
	Mutex		mutex;
    xTaskHandle sendTaskHandle;
} mqttClientType;

typedef struct MQTTConnackData
{
    uint8_t rc;
    uint8_t sessionPresent;
} mqttConnackDataType;


typedef struct MQTTSubackData
{
    enum QoS grantedQoS;
} mqttSubackDataType;

typedef struct
{
    uint32_t time;
    uint16_t id;
    uint8_t qos;
    uint16_t len;
    uint8_t  msg[412];
}mqttSendMsgType;


extern void mqtt_client_init(mqttClientType* c, Network* network, uint32_t dataRepeatTime , unsigned int command_timeout_ms, unsigned char* readbuf, size_t readbuf_size);

extern int mqtt_connect_server(mqttClientType* c, MQTTPacket_connectData* options);

extern int mqtt_subscribe(mqttClientType* c, uint8_t* topicFilter, enum QoS qos,messageHandler messageHandler);

extern int mqtt_unsubscribe(mqttClientType* c, uint8_t* topicFilter);

extern int mqtt_publish_no_close(mqttClientType* c, mqttSendMsgType *message);

extern int mqtt_publish(mqttClientType* c, uint8_t* topicName, MQTTMessage* message);

extern int mqtt_disconnect(mqttClientType* c);

extern int mqtt_run( mqttClientType* c );

extern void mqtt_send_keep_alive( void );

extern int mqtt_send_publish( mqttClientType *c , uint8_t *topic , uint8_t *sendBuf , uint16_t sendLen ,enum QoS qos , uint8_t retained);

extern int mqtt_send_publish_form_isr( mqttClientType *c , uint8_t *topicName , uint8_t *sendBuf , uint16_t sendLen , enum QoS qos , uint8_t retained);

extern int mqtt_send_packet(mqttClientType* c, uint8_t *buf , int length);

extern void mqtt_message_arrived(void *client , MessageData* data);

extern void mqtt_close_session(mqttClientType* c);

extern int mqtt_wait_and_handle( mqttClientType* c, int packet_type);

#endif

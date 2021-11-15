#ifndef _TSL_MQTT_H_
#define _TSL_MQTT_H_

#include "stdint.h"
#include "mqtt_client.h"

#define MQTT_RECV_SUCCESS 1
#define QUEUE_MQTT_SEND_LENGTH                  (5)
#define QUEUE_MQTT_RECV_LENGTH                  (10)

enum
{
    MQTT_RECV_TIMEOUT = -99,
	MQTT_RECV_NULL  = -98,
	MQTT_RECV_EMPTY = -97,
	MQTT_RECV_DEC_ERR = -96,
	MQTT_RECV_CODE_ERR = -95,
	MQTT_RECV_PB_ERR  = -94,
	MQTT_RECV_LEN_ERR = -93,
};

#define	SEQ_ID_NULL								(-1)




typedef struct _mqttRecvMsgType
{
    enum QoS	qos;
    uint8_t	retained;
    uint8_t	dup;
    uint16_t	id;
    uint8_t	*payload;
    uint16_t	payloadlen;
    uint8_t	topicNo;
} mqttRecvMsgType;



typedef struct
{
	uint8_t msgType;
	uint16_t sqeId;
	uint16_t serType;
	uint16_t cmd;
	uint8_t  data[1500];
	uint16_t len;
}mqttDataStrType;

int tsl_mqtt_recv_data_handle(mqttClientType* c, uint16_t cmd);

void creat_tsl_mqtt_task( mqttClientType* c);

char * GmakeJson(void);

int GparseJson(const char * pJson);

char *getDeviceId(void);

char *cj_create_uploadAccessLog_card(long openTime,char lockStatus,char openResult,    char *cardNo,int cardType,int cardIssueType) ;
char *cj_create_uploadAccessLog_pwd( long openTime,  int passwordType) ;



extern uint16_t HTTP_packREG(char *out);
extern uint16_t HTTP_packUP(char *out);
extern uint16_t HTTP_packDEL(char *out);
extern uint16_t HTTP_packCANREG(char *out);
extern char    HTTP_checkack(char *in);
#endif


#include "mqtt_client.h"
#include "unit.h"
#include <stdlib.h>
#include <string.h>
#include "config.h"
#include "tsl_mqtt.h"
#include "mqtt_task.h"

__IO uint8_t CURTOPIC = 44;

static int getNextPacketId(mqttClientType *c);

uint8_t mqttKeepAlive = FALSE;

 xTaskHandle mqttSendtask;

int mqtt_send_publish(mqttClientType *c, 
                     uint8_t *topicName, 
                       uint8_t *sendBuf, 
                       uint16_t sendLen, 
                          enum QoS qos , 
                        uint8_t retained) {
    mqttSendMsgType p;
    uint8_t buff[412];
    MQTTString topic = MQTTString_initializer;
    topic.cstring = topicName;
    int len = 0;
    uint16_t id = 0;

  //  printf("【topicName:%s】【sendBuf:%s】\r\n",topicName,sendBuf);
    if (!c->isconnected) /* don't send connect packet again if we are already connected */
    {
        log(DEBUG,"[mqtt_send_publish] MQTT服务器未建立连接\n");
        return FAILURE;
    }

    if (qos == QOS1 || qos == QOS2)
        id  = getNextPacketId(c);
    
    len = MQTTSerialize_publish(buff, sizeof(buff), 0, qos , retained , id, topic , sendBuf ,sendLen);
    if (len <= 0)
	{
		log(WARN,"MQTT publish %d组包失败\n",__LINE__);
		return FAILURE;
	}

    p.time = 0 ; 
    p.id = id;
    p.qos = qos;
    p.len = len;
    
    memset(p.msg,0x00,412);
    memcpy(p.msg , buff , len );
	
    xQueueSend( xMqttSendQueue, ( void* )&p, NULL );    
    
    return MQTT_SUCCESS;
}

int mqtt_send_publish_form_isr(mqttClientType *c, 
                              uint8_t *topicName, 
                                uint8_t *sendBuf, 
                                uint16_t sendLen, 
                                    enum QoS qos, 
                                uint8_t retained) {
    mqttSendMsgType p;
    uint8_t buff[256];
    MQTTString topic = MQTTString_initializer;
    topic.cstring = topicName;
    int len = 0;
    uint16_t id = 0;

    if (!c->isconnected) /* don't send connect packet again if we are already connected */
    {
        log(DEBUG,"MQTT服务器未建立连接\n");
        return FAILURE;
    }

    if (qos == QOS1 || qos == QOS2)
        id  = getNextPacketId(c);
    
    len = MQTTSerialize_publish(buff, sizeof(buff), 0, qos , retained , id, topic , sendBuf ,sendLen);
    if (len <= 0)
	{
		log(WARN,"MQTT publish 组包失败\n");
		return FAILURE;
	}
	
    p.time = 0 ; 
    p.id = id;
    p.qos = qos;
    p.len = len;
	memset(p.msg,0x00,256);
    memcpy(p.msg , buff , len );

    xQueueSendFromISR( xMqttSendQueue, ( void* )&p, NULL );    
    
    return MQTT_SUCCESS;
}

extern uint32_t sysRunTimerCnt;
int mqtt_send_mesg( mqttClientType *c  , uint8_t *data , uint16_t len ,uint8_t qos)
{
    int rc = FAILURE;

    MutexLock(&c->mutex);

    //log(DEBUG,"2   time=%d\n" , HAL_GetTick() - sysRunTimerCnt);
    
    if (!c->isconnected) /* don't send connect packet again if we are already connected */
    {
        log(DEBUG,"MQTT服务器未建立连接\n");
        goto exit;
    }

    if ((rc = mqtt_send_packet(c, data , len)) != MQTT_SUCCESS) // send the publish packet
    {
        log(WARN,"MQTT 发布发送数据失败,err = %d\n" , len);
        goto exit;             // there was a problem
    }

    //log(DEBUG,"3   time=%d\n" , HAL_GetTick() - sysRunTimerCnt);
    
    if (qos == QOS1)
    {
        if (mqtt_wait_and_handle(c, PUBACK) == PUBACK)  
        {
            unsigned short mypacketid;
            unsigned char dup, type;
            if (MQTTDeserialize_ack(&type, &dup, &mypacketid, c->readbuf, c->readbufSize) != 1)
                rc = FAILURE;

        }
        else
            rc = FAILURE;
    }
    else if (qos == QOS2)
    {
        if (mqtt_wait_and_handle(c, PUBCOMP) == PUBCOMP)  
        {
            unsigned short mypacketid;
            unsigned char dup, type;
            if (MQTTDeserialize_ack(&type, &dup, &mypacketid, c->readbuf, c->readbufSize) != 1)
                rc = FAILURE;

        }
        else
            rc = FAILURE;
    }

exit:
    if (rc == FAILURE)
        mqtt_close_session(c);

    MutexUnlock(&c->mutex);
    return rc;

}

static void mqtt_send_task( void const *pvParameters)
{
    mqttClientType	*client = NULL;
	mqttSendMsgType msg;
	configASSERT(pvParameters);
	
	client = (mqttClientType *)pvParameters;

    memset(&msg , 0x00 , sizeof(mqttSendMsgType));
	while(1)
	{
		if(xQueueReceive( xMqttSendQueue, &msg, 1000 ) == pdTRUE)
		{
            //if( network_read_status() ==  TRUE )
            {
                //log(DEBUG,"1   time=%d\n" , HAL_GetTick() - sysRunTimerCnt);
                if( mqtt_send_mesg(client , msg.msg , msg.len , msg.qos) == MQTT_SUCCESS) 
                {
                    //log(DEBUG,"4 time=%d\n" , HAL_GetTick() - sysRunTimerCnt);
                    sys_delay(20);
                }      
            }
            //else
            {
                //log(INFO,"网络未连接\n");
            }
            memset(&msg , 0x00 , sizeof(mqttSendMsgType));
		}
		//read_task_stack(__func__,mqttSendtask);
	}
}

xTaskHandle creat_mqtt_send_task( mqttClientType* c )
{
    osThreadDef(mqtt_send, mqtt_send_task, osPriorityNormal, 0, configMINIMAL_STACK_SIZE*10);
    mqttSendtask = osThreadCreate(osThread(mqtt_send), (void *)c);
    configASSERT(mqttSendtask);

    return mqttSendtask;
}

void mqtt_client_init(mqttClientType* c, Network* network, uint32_t dataRepeatTime , unsigned int command_timeout_ms, unsigned char* readbuf, size_t readbuf_size)
{
    int i = 0;

    c->ipstack = network;
    c-> dataRepeatTimeSec = dataRepeatTime;
    c->commandTimeoutMs = command_timeout_ms;
    c->readbuf = readbuf;
    c->readbufSize = readbuf_size;
    c->isconnected = 0;
    c->cleansession = 0;
    c->pingOutStanding = 0;
    c->nextPacketId = 1;

    for (i = 0; i < MAX_MESSAGE_HANDLERS; ++i)
    c->messageHandlers[i].topicFilter = 0;

    c->defaultMessageHandler = NULL;

    MutexInit(&c->mutex);

    c->sendTaskHandle = creat_mqtt_send_task(c);
    creat_tsl_mqtt_task(c);
}

void mqtt_clean_session(mqttClientType* c)
{
    int i = 0;

    for (i = 0; i < MAX_MESSAGE_HANDLERS; ++i)
        c->messageHandlers[i].topicFilter = NULL;
}

void mqtt_close_session(mqttClientType* c)
{
    c->pingOutStanding = 0;
    c->isconnected = 0;
    if (c->cleansession)
        mqtt_clean_session(c);
}

static int getNextPacketId(mqttClientType *c) 
{
    return c->nextPacketId = (c->nextPacketId == MAX_PACKET_ID) ? 1 : c->nextPacketId + 1;
}

int mqtt_send_packet(mqttClientType* c, uint8_t *buf , int length)
{
	int len = 0;
	
	len = c->ipstack->mqttwrite(c->ipstack , buf , length , c->commandTimeoutMs);
	
	if( len != length )
	{
		return FAILURE;
	}
	
	return MQTT_SUCCESS;
}

int mqtt_decode_packet(mqttClientType* c, int* value, int timeout)
{
    unsigned char i;
    int multiplier = 1;
    int len = 0;
    const int MAX_NO_OF_REMAINING_LENGTH_BYTES = 4;

    *value = 0;
    do
    {
        int rc = MQTTPACKET_READ_ERROR;

        if (++len > MAX_NO_OF_REMAINING_LENGTH_BYTES)
        {
            rc = MQTTPACKET_READ_ERROR; /* bad data */
            goto exit;
        }
        rc = c->ipstack->mqttread(c->ipstack, &i, 1, timeout);
        if (rc != 1)
            goto exit;
        *value += (i & 127) * multiplier;
        multiplier *= 128;
    } while ((i & 128) != 0);
exit:
    return len;
}

int mqtt_read_packet( mqttClientType* c)
{
    MQTTHeader header = {0};
    int len = 0;
    int rem_len = 0;
	
	/* 1. read the header byte.  This has the packet type in it */
    int rc = c->ipstack->mqttread(c->ipstack, c->readbuf, 1, c->commandTimeoutMs );

	if (rc != 1)
	{
		if( rc != 0 )
			log(WARN,"MQTT 获取数据失败 , err = %d \n" , rc);
        goto exit;
	}
	
	len = 1;
    /* 2. read the remaining length.  This is variable in itself */
    mqtt_decode_packet(c, &rem_len,  c->commandTimeoutMs );
	len += MQTTPacket_encode(c->readbuf + 1, rem_len); /* put the original remaining length back into the buffer */

	if (rem_len > (c->readbufSize - len))
    {
        rc = BUFFER_OVERFLOW;
        goto exit;
    }

    /* 3. read the rest of the buffer using a callback to supply the rest of the data */
    if (rem_len > 0 && (rc = c->ipstack->mqttread(c->ipstack, c->readbuf + len, rem_len, c->commandTimeoutMs) != rem_len)) {
        rc = 0;
        goto exit;
    }

    header.byte = c->readbuf[0];
    rc = header.bits.type;
	
exit:
    return rc;
}

int mqtt_wait_ack( mqttClientType *c , int packet_type )
{
    int rc = FAILURE;
	int time =  c->commandTimeoutMs /50;
	int timeTemp = time;
	c->commandTimeoutMs = 50;
	
    do
    {
        rc = mqtt_read_packet( c );
		if((rc == packet_type) || ( rc < 0) )
		{
			c->commandTimeoutMs = timeTemp*50;
			return rc;
		}
    }
    while ( time-- );

	c->commandTimeoutMs = timeTemp*50;
    return rc;	
}

static void NewMessageData(MessageData* md, MQTTString* aTopicName, MQTTMessage* aMessage) {
    md->topicName = aTopicName;
    md->message = aMessage;
}

// assume topic filter and name is in correct format
// # can only be at end
// + and # can only be next to separator
static char isTopicMatched(char* topicFilter, MQTTString* topicName)
{
    char* curf = topicFilter;
    char* curn = topicName->lenstring.data;
    char* curn_end = curn + topicName->lenstring.len;

    while (*curf && curn < curn_end)
    {
        if (*curn == '/' && *curf != '/')
            break;
        if (*curf != '+' && *curf != '#' && *curf != *curn)
            break;
        if (*curf == '+')
        {   // skip until we meet the next separator, or end of string
            char* nextpos = curn + 1;
            while (nextpos < curn_end && *nextpos != '/')
                nextpos = ++curn + 1;
        }
        else if (*curf == '#')
            curn = curn_end - 1;    // skip until end of string
        curf++;
        curn++;
    };

    return (curn == curn_end) && (*curf == '\0');
}

int mqtt_deliver_message( mqttClientType* c, MQTTString* topicName, MQTTMessage* message)
{
    int i;
    int rc = FAILURE;

    CURTOPIC = 44;
    // we have to find the right message handler - indexed by topic
    for (i = 0; i < MAX_MESSAGE_HANDLERS; ++i)
    {
        if (c->messageHandlers[i].topicFilter != 0 && (MQTTPacket_equals(topicName, (char*)c->messageHandlers[i].topicFilter) ||
                isTopicMatched((char*)c->messageHandlers[i].topicFilter, topicName)))
        {
            if (c->messageHandlers[i].fp != NULL)
            {
                MessageData md;
                NewMessageData(&md, topicName, message);
                c->messageHandlers[i].fp(c , &md);
                
                CURTOPIC = i;
               // printf("-------HERE--CURTOPIC=%d----\r\n",CURTOPIC);

                rc = MQTT_SUCCESS;
            }
        }
    }

    if (rc == FAILURE && c->defaultMessageHandler != NULL)
    {
        MessageData md;
        NewMessageData(&md, topicName, message);
        c->defaultMessageHandler(&md);
         printf("-------HERE--HERE----\r\n");
        rc = MQTT_SUCCESS;
    }

     //   printf("-------HERE--HERE--HERE--\r\n");
    return rc;
}

int mqtt_set_message_handler(mqttClientType* c, uint8_t* topicFilter, messageHandler messageHandler)
{
    int rc = FAILURE;
    int i = -1;

    /* first check for an existing matching slot */
    for (i = 0; i < MAX_MESSAGE_HANDLERS; ++i)
    {
        if (c->messageHandlers[i].topicFilter != NULL && strcmp((char *)c->messageHandlers[i].topicFilter, (char *)topicFilter) == 0)
        {
            if (messageHandler == NULL) /* remove existing */
            {
                c->messageHandlers[i].topicFilter = NULL;
                c->messageHandlers[i].fp = NULL;
            }
            rc = MQTT_SUCCESS; /* return i when adding new subscription */
            break;
        }
    }
    /* if no existing, look for empty slot (unless we are removing) */
    if (messageHandler != NULL) {
        if (rc == FAILURE)
        {
            for (i = 0; i < MAX_MESSAGE_HANDLERS; ++i)
            {
                if (c->messageHandlers[i].topicFilter == NULL)
                {
                    rc = MQTT_SUCCESS;
                    break;
                }
            }
        }
        if (i < MAX_MESSAGE_HANDLERS)
        {
            c->messageHandlers[i].topicFilter = topicFilter;
            c->messageHandlers[i].fp = messageHandler;
        }
    }
    return rc;
}

int mqtt_keepalive( mqttClientType* c)
{
    int rc = MQTT_SUCCESS;
	int len = 0;
	uint8_t buff[256];
	
	if (c->pingOutStanding)
	{
		rc = FAILURE; /* PINGRESP not received in keepalive interval */
	}
	else
	{
		len = MQTTSerialize_pingreq(buff, sizeof(buff));
		if (len > 0 && (rc = mqtt_send_packet(c, buff , len)) == MQTT_SUCCESS) // send the ping packet
		{
			c->pingOutStanding = 1;   
		}
	}
    return rc;
}

int mqtt_cycle( mqttClientType* c)
{
	uint8_t buff[1500];
    int len = 0,rc = MQTT_SUCCESS;
	int packet_type = 0;
		
	if (!c->isconnected) /* don't send connect packet again if we are already connected */
    {
        log(DEBUG,"MQTT服务器未建立连接\n");
		rc = FAILURE;
        goto exit;
    }
	
    packet_type = mqtt_read_packet(c);     /* read the socket, see what work is due */

    switch (packet_type)
    {
        default:
            /* no more data to read, unrecoverable. Or read packet fails due to unexpected network error */
            rc = packet_type;
            goto exit;
        case 0: /* timed out reading packet */
            break;
        case CONNACK:
        {

        }break;
        case PUBACK:
        {
            unsigned short mypacketid;
            unsigned char dup, type;
            if (MQTTDeserialize_ack(&type, &dup, &mypacketid, c->readbuf, c->readbufSize) == 1)
            {
                //log(DEBUG,"get a publish ack , id = %d \n" , mypacketid);
            }
            else
            {
                rc = FAILURE;
            }
        }
        case SUBACK:
            break;
        case PUBLISH:
        {
            MQTTString topicName;
            MQTTMessage msg;
            int intQoS;
            msg.payloadlen = 0; /* this is a size_t, but deserialize publish sets this as int */
            if (MQTTDeserialize_publish(&msg.dup, &intQoS, &msg.retained, &msg.id, &topicName,
               (unsigned char**)&msg.payload, (int*)&msg.payloadlen, c->readbuf, c->readbufSize) != 1)
                goto exit;
            msg.qos = (enum QoS)intQoS;
			
            if (msg.qos != QOS0)
            {
                if (msg.qos == QOS1)
                    len = MQTTSerialize_ack(buff, sizeof(buff), PUBACK, 0, msg.id);
                else if (msg.qos == QOS2)
                    len = MQTTSerialize_ack(buff, sizeof(buff), PUBREC, 0, msg.id);
                if (len <= 0)
                    rc = FAILURE;
                else
                    rc = mqtt_send_packet(c, buff , len);
                if (rc == FAILURE)
                    goto exit; // there was a problem
            }
            
            //log(INFO,"recv mqtt data [ 123456 -1] \n");
            mqtt_deliver_message(c, &topicName, &msg);
            
            break;
        }
        case PUBREC:
        case PUBREL:
        {
            unsigned short mypacketid;
            unsigned char dup, type;
            if (MQTTDeserialize_ack(&type, &dup, &mypacketid, c->readbuf, c->readbufSize) != 1)
                rc = FAILURE;
            else if ((len = MQTTSerialize_ack(buff, sizeof(buff),
                (packet_type == PUBREC) ? PUBREL : PUBCOMP, 0, mypacketid)) <= 0)
                rc = FAILURE;
            else if ((rc = mqtt_send_packet(c, buff , len)) != MQTT_SUCCESS) // send the PUBREL packet
                rc = FAILURE; // there was a problem
            if (rc == FAILURE)
                goto exit; // there was a problem
            break;
        }

        case PUBCOMP:
        {
            unsigned short mypacketid;
            unsigned char dup, type;
            if (MQTTDeserialize_ack(&type, &dup, &mypacketid, c->readbuf, c->readbufSize) == 1)
            {
                //log(DEBUG,"get a publish comp ack , id = %d \n" , mypacketid);
            }
            else
            {
                rc = FAILURE;
            }
            
            
        }break;
        case PINGRESP:
			//log(INFO,"Get a ping response\n");
            c->pingOutStanding = 0;
            break;
    }

	if( mqttKeepAlive )
	{
		mqttKeepAlive = FALSE;
		rc = mqtt_keepalive( c );
	}
    /*if (keepalive(c) != MQTT_SUCCESS) {
		log(WARN,"send ping request is error\n");
        //check only keepalive FAILURE status so that previous FAILURE status can be considered as FAULT
        rc = FAILURE;
    }*/

exit:
    if (rc == MQTT_SUCCESS)
        rc = packet_type;
    else if (c->isconnected)
	{
		log(WARN,"close connect , rc = %d \n" , rc);
        mqtt_close_session(c);
	}
    return rc;
}


int mqtt_wait_and_handle( mqttClientType* c, int packet_type)
{
    int rc = FAILURE;
	int time =  c->commandTimeoutMs/50;
	int timeTemp = time;
	c->commandTimeoutMs = 50;
    do
    {
        rc = mqtt_cycle( c );
		if((rc == packet_type) || ( rc < 0) )
		{
			c->commandTimeoutMs = timeTemp*50;
			return rc;
		}
    }while ( time-- );

	c->commandTimeoutMs = timeTemp*50;
    return rc;
}


int mqtt_connect_with_results(mqttClientType* c, MQTTPacket_connectData* options, mqttConnackDataType* data)
{
	int len = 0;
	int rc = FAILURE;
	uint8_t buff[256];
	
	MQTTPacket_connectData default_options = MQTTPacket_connectData_initializer;
	
	MutexLock(&c->mutex);
	
	if (c->isconnected) /* don't send connect packet again if we are already connected */
	{
		log(DEBUG,"MQTT 服务器已经建立连接\n");
		goto exit;
	}
	
	if (options == 0)
	{
		log(INFO,"连接参数设置为空，选用默认参数连接\n");
        options = &default_options; /* set default options if none were supplied */
	}
	
	c->keepAliveInterval = options->keepAliveInterval;
        c->cleansession = options->cleansession;
	log(DEBUG,"keet alive inter val=%d , clean session = %d\n" , c->keepAliveInterval , c->cleansession);
	if ((len = MQTTSerialize_connect(buff, sizeof(buff), options)) <= 0)
	{
		log(WARN,"MQTT connect 组包失败\n");
		goto exit;
	}
	//log_arry(DEBUG,"send connect message" ,buff ,  len);
	if ((rc = mqtt_send_packet(c, buff , len)) != MQTT_SUCCESS) // send the connect packet
	{
		log(WARN,"MQTT 连接服务器 发送数据失败,err = %d\n" , len);
        goto exit;             // there was a problem
	}
	
	    // this will be a blocking call, wait for the connack
    if (mqtt_wait_ack(c, CONNACK) == CONNACK)
    {
        data->rc = 0;
        data->sessionPresent = 0;
        if (MQTTDeserialize_connack(&data->sessionPresent, &data->rc, c->readbuf, c->readbufSize) == 1)
            rc = data->rc;
        else
            rc = FAILURE;
    }
    else
        rc = FAILURE;
	
exit:
    if (rc == MQTT_SUCCESS)
    {
        c->isconnected = 1;
        c->pingOutStanding = 0;
    }

	MutexUnlock(&c->mutex);

    return rc;
}

int mqtt_connect_server(mqttClientType* c, MQTTPacket_connectData* options)
{
	mqttConnackDataType data;
	
	return mqtt_connect_with_results(c, options, &data);
	
}




					  
int mqtt_subscribe_with_results(mqttClientType* c, uint8_t * topicFilter, enum QoS qos,
       messageHandler messageHandler, mqttSubackDataType* data)
{
	int rc = FAILURE;
    int len = 0;
	uint8_t buff[256];
	
    MQTTString topic = MQTTString_initializer;
    topic.cstring = topicFilter;
	
	MutexLock(&c->mutex);
	
	if (!c->isconnected) /* don't send connect packet again if we are already connected */
	{
		log(DEBUG,"MQTT服务器未建立连接\n");
		goto exit;
	}
	
	len = MQTTSerialize_subscribe(buff, sizeof(buff), 0, getNextPacketId(c), 1, &topic, (int*)&qos);
	if (len <= 0)
	{
		log(WARN,"MQTT publish %d组包失败\n",__LINE__);
		goto exit;
	}
	
	if ((rc = mqtt_send_packet(c, buff , len)) != MQTT_SUCCESS) // send the subscribe packet
	{
		log(WARN,"MQTT 订阅 发送数据失败,err = %d\n" , len);
        goto exit;             // there was a problem
	}
	
    if (mqtt_wait_and_handle(c, SUBACK) == SUBACK)      // wait for suback
    {
        int count = 0;
        unsigned short mypacketid;
        data->grantedQoS = QOS0;
        if (MQTTDeserialize_suback(&mypacketid, 1, &count, (int*)&data->grantedQoS, c->readbuf, c->readbufSize) == 1)
        {
            if (data->grantedQoS != 0x80)
                rc = mqtt_set_message_handler(c, topicFilter, messageHandler);
        }
    }
    else
        rc = FAILURE;
	
	
	
exit:
    if (rc == FAILURE)
	{
		mqtt_close_session(c);
	}

	MutexUnlock(&c->mutex);

    printf("Gsubscribe %s rc=%d\r\n",topicFilter,rc);

    return rc;
}



int mqtt_subscribe(mqttClientType* c, uint8_t* topicFilter, enum QoS qos,messageHandler messageHandler)
{
    mqttSubackDataType data;
	
    return mqtt_subscribe_with_results(c, topicFilter, qos, messageHandler, &data);
}


int mqtt_unsubscribe(mqttClientType* c, uint8_t* topicFilter)
{
    int rc = FAILURE;
    uint8_t buff[256];
    MQTTString topic = MQTTString_initializer;
    topic.cstring = topicFilter;
    int len = 0;


    MutexLock(&c->mutex);

    if (!c->isconnected) /* don't send connect packet again if we are already connected */
    {
        log(DEBUG,"MQTT服务器未建立连接\n");
        goto exit;
    }


    if ((len = MQTTSerialize_unsubscribe(buff, sizeof(buff), 0, getNextPacketId(c), 1, &topic)) <= 0)
    {
		log(WARN,"MQTT publish %d组包失败\n",__LINE__);
		goto exit;
	}
    
    if ((rc = mqtt_send_packet(c, buff , len)) != MQTT_SUCCESS) // send the subscribe packet
	{
		log(WARN,"MQTT 订阅 发送数据失败,err = %d\n" , len);
        goto exit;             // there was a problem
	}


    if (mqtt_wait_and_handle(c, UNSUBACK) == UNSUBACK)  
    {
        unsigned short mypacketid;  // should be the same as the packetid above
        if (MQTTDeserialize_unsuback(&mypacketid, c->readbuf, c->readbufSize) == 1)
        {
            /* remove the subscription message handler associated with this topic, if there is one */
            mqtt_set_message_handler(c, topicFilter, NULL);
        }
    }
    else
        rc = FAILURE;

exit:
    if (rc == FAILURE)
        mqtt_close_session(c);

    MutexUnlock(&c->mutex);

    return rc;
}


int mqtt_publish(mqttClientType* c, uint8_t* topicName, MQTTMessage* message)
{
    int rc = FAILURE;
    uint8_t buff[256];
    MQTTString topic = MQTTString_initializer;
    topic.cstring = topicName;
    int len = 0;


    MutexLock(&c->mutex);

    if (!c->isconnected) /* don't send connect packet again if we are already connected */
    {
        log(DEBUG,"MQTT服务器未建立连接\n");
        goto exit;
    }

    if (message->qos == QOS1 || message->qos == QOS2)
        message->id = getNextPacketId(c);

    len = MQTTSerialize_publish(buff, sizeof(buff), 0, message->qos, message->retained, message->id,
              topic, (unsigned char*)message->payload, message->payloadlen);
    if (len <= 0)
	{
		log(WARN,"MQTT publish %d组包失败\n",__LINE__);
		goto exit;
	}
    
    
    if ((rc = mqtt_send_packet(c, buff , len)) != MQTT_SUCCESS) // send the publish packet
    {
        log(WARN,"MQTT 发布发送数据失败,err = %d\n" , len);
        goto exit;             // there was a problem
    }

    
    if (message->qos == QOS1)
    {
        if (mqtt_wait_and_handle(c, PUBACK) == PUBACK)  
        {
            unsigned short mypacketid;
            unsigned char dup, type;
            if (MQTTDeserialize_ack(&type, &dup, &mypacketid, c->readbuf, c->readbufSize) != 1)
                rc = FAILURE;
        }
        else
            rc = FAILURE;
    }
    else if (message->qos == QOS2)
    {
		if (mqtt_wait_and_handle(c, PUBCOMP) == PUBCOMP)  
        {
            unsigned short mypacketid;
            unsigned char dup, type;
            if (MQTTDeserialize_ack(&type, &dup, &mypacketid, c->readbuf, c->readbufSize) != 1)
                rc = FAILURE;
        }
        else
            rc = FAILURE;
    }

exit:
    if (rc == FAILURE)
        mqtt_close_session(c);

    MutexUnlock(&c->mutex);
    return rc;
}

int mqtt_disconnect(mqttClientType* c)
{
    int rc = FAILURE;
    int len = 0;
	uint8_t buff[256];

	MutexLock(&c->mutex);

	len = MQTTSerialize_disconnect(buff, sizeof(buff));
	
	if( len > 0 )
	{
		if ((rc = mqtt_send_packet(c, buff , len)) != MQTT_SUCCESS) // send the publish packet
		{
			log(WARN,"MQTT 发布发送数据失败,err = %d\n" , len);
		}
	}
	
    mqtt_close_session(c);

	MutexUnlock(&c->mutex);

    return rc;
}

void mqtt_send_keep_alive( void )
{
	mqttKeepAlive = TRUE;
}

int mqtt_run( mqttClientType* c )
{
    int rt = MQTT_SUCCESS;
    int timeout;
    
    MutexLock(&c->mutex);

    timeout = c->commandTimeoutMs;
    c->commandTimeoutMs = 50;
	
    rt = mqtt_cycle(c) ;
	
    c->commandTimeoutMs = timeout;

    MutexUnlock(&c->mutex);
	
	sys_delay(10);

    return rt;
}


#include "mqtt_client.h"

extern char topicPath[GMAX_MESSAGE_HANDLERS][52];


uint8_t GettopicNo(char *topicName)
{
  char i;
  for( i=0;i<GMAX_MESSAGE_HANDLERS;i++)
      if(aiot_strcmp(( unsigned char *)topicPath[i],( unsigned char *)topicName,23))//24怎么来的？从后往前比较一样12个MAC不是区别 还有Request/8个 也就是20 最大是时间
         return i;
         
  return 44;       
}
extern char      RXOKLOCK;
//extern uint8_t    COMMONSTATIC[1024];
void mqtt_message_arrived(void *client , MessageData* data)
{
 //   SHOWME  
mqttRecvMsgType p;

log(INFO,"Message arrived on topic %.*s\n", data->topicName->lenstring.len, data->topicName->lenstring.data);
   // log_arry(ERR,"recv data:" , data->message->payload , data->message->payloadlen);
log(INFO,"data->message->payload: \r\n %.*s \r\n",data->message->payloadlen, (char *)data->message->payload);
log(ERR, "\r\n\r\ndata->message->payloadlen = %d\r\n",data->message->payloadlen);//一般200以内
	if( data->message->payloadlen != 0 && (0 == RXOKLOCK))
	{
		memset(&p , 0x00 , sizeof(mqttRecvMsgType));
        p.topicNo =  GettopicNo(data->topicName->lenstring.data);//CURTOPIC
		p.id = data->message->id;
		p.payloadlen = data->message->payloadlen;
		p.qos = data->message->qos;
		p.dup = data->message->dup;
		p.retained = data->message->retained;
		p.payload = malloc( sizeof(uint8_t)*data->message->payloadlen);

		if( p.payload == NULL )
		{
			log_err("内存不够\n");
			soft_system_resert(__func__);
		}

      //  memset(COMMONSTATIC, 0x00, 1024);
      //  p.payload =  COMMONSTATIC;     
    
        memset(p.payload , 0x00, sizeof(uint8_t)*data->message->payloadlen);
        log(ERR,"malloc p address = %x\n" , p.payload);
        memcpy(p.payload , data->message->payload , data->message->payloadlen);
        //log(INFO,"data->message->payload: \r\n %s \r\n", p.payload);
//     log(INFO,"p.topicNo = %d\n",p.topicNo);
//       RXOKLOCK =1;
        xQueueSend(xMqttRecvQueue, (void*)&p, NULL);  

//       log(INFO,"recv mqtt data [ 123456 -2] \n");
//        SHOWME
	}
}



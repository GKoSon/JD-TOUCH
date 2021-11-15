/*******************************************************************************
 * Copyright (c) 2014, 2015 IBM Corp.
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * and Eclipse Distribution License v1.0 which accompany this distribution.
 *
 * The Eclipse Public License is available at
 *    http://www.eclipse.org/legal/epl-v10.html
 * and the Eclipse Distribution License is available at
 *   http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * Contributors:
 *    Allan Stockdill-Mander - initial API and implementation and/or initial documentation
 *******************************************************************************/

#if !defined(MQTTFreeRTOS_H)
#define MQTTFreeRTOS_H
#include <string.h>
#include "FreeRTOS.h"
//#include "FreeRTOS_Sockets.h"
//#include "FreeRTOS_IP.h"
#include "semphr.h"
#include "task.h"

typedef struct SocketId
{
	int8_t id;
}MqttSocketId;

typedef struct Timer 
{
	TickType_t xTicksToWait;
	xTimeOutType xTimeOut;
} Timer;

typedef struct Network Network;
/* The socket type itself. */
typedef void *xSocket_t;

/* The xSocketSet_t type is the equivalent to the fd_set type used by the
Berkeley API. */
typedef void *xSocketSet_t;
struct Network
{
	xSocket_t my_socket;
	int (*mqttread) (Network*, unsigned char*, int, int);
	int (*mqttwrite) (Network*, unsigned char*, int, int);
	void (*disconnect) (Network*);
};

void TimerInit(Timer*);
char TimerIsExpired(Timer*);
void TimerCountdownMS(Timer*, unsigned int);
void TimerCountdown(Timer*, unsigned int);
int TimerLeftMS(Timer*);

typedef struct Mutex
{
	xSemaphoreHandle sem;
} Mutex;

void MutexInit(Mutex*);
int MutexLock(Mutex*);
int MutexUnlock(Mutex*);

typedef struct Thread
{
	xTaskHandle task;
} Thread;

int ThreadStart(Thread*, void (*fn)(void*), void* arg);

int FreeRTOS_read(Network*, unsigned char*, int, int);
int FreeRTOS_write(Network*, unsigned char*, int, int);
void FreeRTOS_disconnect(Network*);

void mqtt_network_close( void );
void mqtt_network_init(Network* n);
int mqtt_network_connect(Network* n, uint8_t* addr, int port);
/*int NetworkConnectTLS(Network*, char*, int, SlSockSecureFiles_t*, unsigned char, unsigned int, char);*/

#endif

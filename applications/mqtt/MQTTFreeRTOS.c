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
 *    Ian Craggs - convert to FreeRTOS
 *******************************************************************************/

#include "MQTTFreeRTOS.h"
#include "unit.h"
#include "socket.h"

extern char    mqttSocketBuffer[2048];
MqttSocketId    mqttSocketId;

int        mqttSocketRecvLen = 0;

int ThreadStart(Thread* thread, void (*fn)(void*), void* arg)
{
    int rc = 0;
    uint16_t usTaskStackSize = (configMINIMAL_STACK_SIZE * 10);
    UBaseType_t uxTaskPriority = uxTaskPriorityGet(NULL); /* set the priority as the same as the calling task*/

    rc = xTaskCreate(fn,    /* The function that implements the task. */
        "MQTTTask",            /* Just a text name for the task to aid debugging. */
        usTaskStackSize,    /* The stack size is defined in FreeRTOSIPConfig.h. */
        arg,                /* The task parameter, not used in this case. */
        uxTaskPriority,        /* The priority assigned to the task is defined in FreeRTOSConfig.h. */
        NULL);        /* The task handle is not used. */
        
    return rc;
}

void MutexInit(Mutex* mutex)
{
    mutex->sem = xSemaphoreCreateMutex();
}

int MutexLock(Mutex* mutex)
{
    return xSemaphoreTake(mutex->sem, portMAX_DELAY);
}

int MutexUnlock(Mutex* mutex)
{
    return xSemaphoreGive(mutex->sem);
}


void TimerCountdownMS(Timer* timer, unsigned int timeout_ms)
{
    timer->xTicksToWait = timeout_ms ; /* convert milliseconds to ticks */
    vTaskSetTimeOutState(&timer->xTimeOut); /* Record the time at which this function was entered. */
}

void TimerCountdown(Timer* timer, unsigned int timeout) 
{
    TimerCountdownMS(timer, timeout * 1000);
}

int TimerLeftMS(Timer* timer) 
{
    xTaskCheckForTimeOut(&timer->xTimeOut, &timer->xTicksToWait); /* updates xTicksToWait to the number left */
    return (timer->xTicksToWait );
}

char TimerIsExpired(Timer* timer)
{
    return xTaskCheckForTimeOut(&timer->xTimeOut, &timer->xTicksToWait) == pdTRUE;
}

void TimerInit(Timer* timer)
{
    timer->xTicksToWait = 0;
    memset(&timer->xTimeOut, '\0', sizeof(timer->xTimeOut));
}

int FreeRTOS_recv(void *socketId, uint8_t *buffer, int len, int timeout_ms)
{
    //int ret = 0;
    //int timeout =     timeout_ms / 5;
    uint8_t bufferTemp[2000];
    MqttSocketId *mqttId = (MqttSocketId *)(socketId);

    if( mqttSocketRecvLen == len )
    {
        memcpy(buffer , mqttSocketBuffer , len);
        memset(mqttSocketBuffer , 0x00 , len);
        mqttSocketRecvLen -= len;
        return len;
    }    
    else if( mqttSocketRecvLen > len )
    {
        memcpy(bufferTemp , mqttSocketBuffer , mqttSocketRecvLen);
        memcpy(buffer , mqttSocketBuffer , len);
        memset(mqttSocketBuffer , 0x00 , mqttSocketRecvLen);
        memcpy(mqttSocketBuffer , bufferTemp+len , mqttSocketRecvLen-len);
        mqttSocketRecvLen -= len;
        return len;
    }
    else
    {
        int8_t rt = 0;
        
        rt = socket.read(mqttId->id , timeout_ms);
        
        if( rt < 0)
        {
            if( rt == SOCKET_READ_TIMEOUT )
            {
                return 0;
            }
            
            return rt;
        }
        
        //log(DEBUG,"Recv len = %d \n" , rt);
        
        mqttSocketRecvLen += rt;
        if( mqttSocketRecvLen == len )
        {
            memcpy(buffer , mqttSocketBuffer , len);
            memset(mqttSocketBuffer , 0x00 , len);
            mqttSocketRecvLen -= len;
            return len;
        }    
        else if( mqttSocketRecvLen > len )
        {
            memcpy(bufferTemp , mqttSocketBuffer , mqttSocketRecvLen);
            memcpy(buffer , mqttSocketBuffer , len);
            memset(mqttSocketBuffer , 0x00 , mqttSocketRecvLen);
            memcpy(mqttSocketBuffer , bufferTemp+len , mqttSocketRecvLen-len);
            mqttSocketRecvLen -= len;
            return len;
        }
        
        return 0 ;
        
    }
}

int FreeRTOS_recv_buffer(void *socketId ,  uint8_t *buffer , int len ,int timeout_ms)
{
    int ret = 0;
    MqttSocketId *mqttId = (MqttSocketId *)(socketId);

    ret = socket.read_buffer(mqttId->id , buffer , len , timeout_ms);

    if( ret == SOCKET_READ_TIMEOUT)
    {
        return 0;
    }
    else
    {
        return ret ; 
    }
}

int FreeRTOS_read(Network* n, unsigned char* buffer, int len, int timeout_ms)
{
    int time = timeout_ms/10 ; /* convert milliseconds to ticks */

    int recvLen = 0;

    do
    {
        int rc = 0;
        
        rc = FreeRTOS_recv_buffer(n->my_socket, buffer + recvLen, len - recvLen, 10);
        if (rc > 0)
            recvLen += rc;
        else if (rc < 0)
        {
            recvLen = rc;
            break;
        }

    } while ((recvLen < len) && ( time-- ));

    return recvLen;
}

int FreeRTOS_send(void *socketId ,  uint8_t *buffer , int len ,int timeout_ms)
{
    MqttSocketId *mqttId = (MqttSocketId *)(socketId);
    int ret = 0;
    
    ret = socket.send(mqttId->id , buffer , len , timeout_ms);
    
    if( ret == SOCKET_OK)
    {
        return len;
    }
    
    return  ret;
}

int FreeRTOS_write(Network* n, unsigned char* buffer, int len, int timeout_ms)
{
    int time = timeout_ms/10000;
    int sentLen = 0;

    do
    {
        int rc = 0;

        rc = FreeRTOS_send(n->my_socket, buffer + sentLen, len - sentLen, 10000);
        if (rc > 0)
        {
            sentLen += rc;
        }
        else if (rc < 0)
        {
            sentLen = rc;
            break;
        }
        
    } while (sentLen < len && time--);

    if( sentLen < 0)
    {
        log(WARN,"Send error =%d\n" , sentLen);
    }

    return sentLen;
}

void FreeRTOS_disconnect(Network* n)
{
    MqttSocketId *mqttId = (MqttSocketId *)(n->my_socket);
    
    //FreeRTOS_closesocket(n->my_socket);
    socket.disconnect(mqttId->id);
}

void mqtt_network_init(Network* n)
{
    n->my_socket = (void *)&mqttSocketId;
    n->mqttread = FreeRTOS_read;
    n->mqttwrite = FreeRTOS_write;
    n->disconnect = FreeRTOS_disconnect;
}

void mqtt_network_close( void )
{
    socket.close();
}

int mqtt_network_connect(Network* n, uint8_t* addr, int port)
{        
    int8_t ret = 0;
    MqttSocketId *mqttId = (MqttSocketId *)(n->my_socket);
    
    while( socket.isOK() != TRUE )
    {
        sys_delay(500);
        //log(DEBUG,"[MQTT]底层socket等待socket.isOK()\n");
    }
    log(DEBUG,"[MQTT]底层socket驱动已经加载成功，开始连接MQTT服务器\n");
    
    if( (ret = socket.connect(addr , port, mqttSocketBuffer , sizeof(mqttSocketBuffer))) >= 0)
    {
        mqttId->id = ret;
        log(DEBUG,"[MQTT]MQTT服务器连接成功\n");
    }
    else
    {
        log(DEBUG,"[MQTT]MQTT服务器连接失败\n");	
    }
    return ret;
}


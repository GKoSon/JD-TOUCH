#include "socket.h"
#include "sim800c.h"
#include "unit.h"
#include "net.h"
#include "open_log.h"
#include "component.h"
#include "sysCfg.h"
#include "esp12s.h"
#include "w5500_config.h"
#include "config.h"
#include "sys_led.h"


devComType *devCom = NULL;

__IO socketWorkStatusEnum socketStatus = SOCKET_CLOSE_STATUS;

sockeArryType   sockeArry[SOCKET_CONNECT_MAX];

void socket_bind_buffer( int8_t id , char *pData , uint16_t size)
{
	if( id < SOCKET_CONNECT_MAX)
	{
		sockeArry[id].len = 0;
		sockeArry[id].status =  SOCKET_INIT;
		sockeArry[id].useFlag = TRUE;
		sockeArry[id].msg = pData;
                sockeArry[id].maxSize = size;

	}
	else
	{
		log_err("scoket绑定失败，id = %d\n" , id);
	}
}

void socket_clear_bind( int8_t id)
{
	if( id < SOCKET_CONNECT_MAX)
	{
		sockeArry[id].len = 0;
		sockeArry[id].status =  SOCKET_INIT;
		sockeArry[id].useFlag = FALSE;
		sockeArry[id].msg = NULL;
	}
	else
	{
		log_err("scoket解绑定失败，id = %d\n" , id);
	}
}


void socket_clear_all( void )
{
	for(uint8_t i = 0 ; i < SOCKET_CONNECT_MAX; i++)
	{
		sockeArry[i].len = 0;
		sockeArry[i].status =  SOCKET_INIT;
		sockeArry[i].useFlag = FALSE;
		sockeArry[i].msg = NULL;
	}

	socketStatus = SOCKET_CLOSE_STATUS;

}

void socket_clear_buffer( uint8_t id)
{
	if( id < SOCKET_CONNECT_MAX )
	{
		sockeArry[id].len =0;
	}
}

sockeArryType * socket_read_obj( uint8_t id)
{
    if( id < SOCKET_CONNECT_MAX )
    {
        return (&sockeArry[id]);
    }

    return NULL;
}



int8_t socket_find_port( void )
{
    for(uint8_t i = 0 ; i < SOCKET_CONNECT_MAX; i++)
    {
        if( sockeArry[i].useFlag == FALSE)
        {
            return i;
        }
    }

    return -1;
}

void socket_set_status( socketWorkStatusEnum status)
{
	socketStatus = status;
	if( socketStatus == SOCKET_WORKING_STATUS)
	{
		sysLed.write(SYS_LED_CONNECT_NET);
	}
	else
	{
		sysLed.write(SYS_LED_NORMAL);
	}
}

void socket_err(int8_t err , int8_t id)
{

    if( err == SOCKET_READ_TIMEOUT)
    {
        return;
    }
    
    log(DEBUG,"socket[%d] , 返回码:%d\n" , id, err);

}


int8_t socket_write(uint8_t id ,  uint8_t *sendData , uint16_t length , uint32_t timeout )
{
    int ret = SOCKET_OK;
    
    sockeArryType *socket = socket_read_obj(id);

    if(socketStatus != SOCKET_WORKING_STATUS)
    {
    	return SOCKERT_STATUS_ERR;
    }

    if(socket == NULL)
    {
    	log_err("发送不存在这个id , id = %d\n" , id);
        return SOCKET_SEND_NOID ;
    }

    if( socket->useFlag == FALSE)
    {
        log(WARN,"发送socket 未建立连接 id = %d\n" , id);
        return SOCKET_SEND_NOCON;
    }

	if( xSemaphoreTake( xSocketSemaphore, timeout ) == pdTRUE )
	{
		if( ( ret = devCom->send(id , sendData , length )) == TRUE)
		{
			//log(DEBUG , "socket %d 发送数据完成\n" , id);
			xSemaphoreGive(xSocketSemaphore);
			return SOCKET_OK;
		}
		xSemaphoreGive(xSocketSemaphore);
		return ret;
	}
	else
	{
		log(WARN,"socket %d 发送数据，有其他任务占用资源\n" , id);
		return SOCKET_SEND_BUSY;
	}

}

int socket_read(int8_t id , uint32_t timeout)
{
    int ret = SOCKET_OK;
    uint32_t timecnt = timeout/5;

    sockeArryType *socket = socket_read_obj(id);

    if(socketStatus != SOCKET_WORKING_STATUS)		    return SOCKERT_STATUS_ERR;


    if(socket == NULL)                                  return SOCKER_READ_NOID ;
    
    if( socket->useFlag == FALSE)    {        log(WARN,"socket 未建立连接 id = %d\n" , id);        return SOCKET_READ_NOCON;    }

    while(timecnt--)
    {
        socket = socket_read_obj(id);

        if(socket == NULL){log_err("不存在这个id , id = %d\n" , id);	return SOCKER_READ_NOID ;}

        if( socket->useFlag == FALSE)	{log(WARN,"socket 未建立连接 id = %d\n" , id);	return SOCKET_READ_NOCON;}

    	if( socket->status == SOCKET_READ )
    	{
    		if( socket->len > 0 )
    		{
    			ret = socket->len;
    			socket->len = 0;
    			socket->status = SOCKET_INIT;
		        if(otasee)     log_arry(INFO,"socket read data" ,(uint8_t *)socket->msg ,ret);//是怎么？socket->msg  socket_bind_buffer
                 //printf("ret =%d\r\n",ret);       
    			 return ret;
    		}
    		else
    		{
    			log_err("socket %d 获取数据,数据长度为: %d\n" , id , socket->len);
    			return SOCKET_READ_LENERR;
    		}
    	}
    	else if( socket->status == SOCKET_CLOSE )
    	{
    		socket_clear_bind(id);
    		return  SOCKET_CONNECT_CLOSE;
    	}
    	sys_delay(5);
    }

  return SOCKET_READ_TIMEOUT;
}



int socket_read_data(int8_t id , uint8_t *recvData , int32_t recvLen , uint32_t timeout)
{
    int ret = SOCKET_OK;
    uint32_t timecnt = timeout/5;

    sockeArryType *socket = socket_read_obj(id);

    if(socketStatus != SOCKET_WORKING_STATUS)
	{
		return SOCKERT_STATUS_ERR;
	}

    if(socket == NULL)
    {
    	log_err("不存在这个id , id = %d\n" , id);
        return SOCKER_READ_NOID ;
    }

    if( socket->useFlag == FALSE)
    {
        log(WARN,"socket 未建立连接 id = %d\n" , id);
        return SOCKET_READ_NOCON;
    }

    while(timecnt--)
    {
    	socket = socket_read_obj(id);

        if(socket == NULL)
        {
                log_err("不存在这个id , id = %d\n" , id);
                return SOCKER_READ_NOID ;
        }

        if( socket->useFlag == FALSE)
        {
                log(WARN,"socket 未建立连接 id = %d\n" , id);
                return SOCKET_READ_NOCON;
        }

    	if( socket->status == SOCKET_READ )
    	{
//log(ERR,"socket read len = %d , buffer len = %d\n"  , recvLen , socket->len );
//if(otasee)log_arry(ERR, "socket recv", (uint8_t *)socket->msg  , socket->len );
            if( socket->len <= recvLen )
            {
                memcpy(recvData , socket->msg , socket->len);
                ret = socket->len;
                
                memset(socket->msg , 0x00 , socket->len);
                socket->len = 0;
    		socket->status = SOCKET_INIT;

                return ret;
            }
            else
            {
                uint8_t tempData[2048];
				
                memcpy(recvData , socket->msg , recvLen);
                memcpy(tempData , socket->msg , socket->len);
                memset(socket->msg , 0x00 , socket->len);
                memcpy(socket->msg , tempData+recvLen , socket->len-recvLen);
                socket->len -= recvLen;

                return recvLen;
                
            }

    	}
    	else if( socket->status == SOCKET_CLOSE )
    	{
    		socket_clear_bind(id);
    		return  SOCKET_CONNECT_CLOSE;
    	}
    	sys_delay(5);
    }

	return SOCKET_READ_TIMEOUT;
}


int8_t socket_connect(uint8_t *ip , uint16_t port , char *pData , uint16_t size)
{
	 
    int8_t id =0 ,ret = SOCKET_CONNECT_ERR;

    if(socketStatus != SOCKET_WORKING_STATUS)
    {
            return SOCKERT_STATUS_ERR;
    }

    if( xSemaphoreTake( xSocketSemaphore, 60000 ) == pdTRUE )
    {
    	if( (id = socket_find_port()) >=0 )
    	{
              if( ( ret = devCom->connect(id , ip , port)) == SOCKET_OK )
              {
                      socket_bind_buffer(id , pData , size);
                      xSemaphoreGive( xSocketSemaphore );
                      return id;
              }
              else
              {
                      log_err("与服务器建立连接失败,err = %d\n" , ret);
              }
    	}
    	else
    	{
    		ret = SOCKET_CONNECT_FULL;
    		log_err("scoket 已达到最大连接数量\n");
    	}
    	xSemaphoreGive( xSocketSemaphore );
    }
    else
    {
    	ret = SOCKET_CONNECT_BUSY;
        log(WARN,"链接服务器失败，有其他任务占用资源\n");
    }

    return (ret);
}


int8_t socket_disconnect( int8_t id )
{
  
    sockeArryType *socket = socket_read_obj(id);

    if(socketStatus != SOCKET_WORKING_STATUS)
	{
		return SOCKERT_STATUS_ERR;
	}

    if(socket == NULL)
    {
    	log_err("不存在这个id , id = %d\n" , id);
        return SOCKER_READ_NOID ;
    }

    if( socket->useFlag == FALSE)
    {
        log(WARN,"socket 未建立连接 id = %d\n" , id);
        return SOCKET_READ_NOCON;
    }

    if( xSemaphoreTake( xSocketSemaphore, 30000 ) == pdTRUE )
    {
    	socket_clear_bind(id);
    	if( devCom->disconnect(id) == TRUE)
    	{
    		log(DEBUG,"Socket关闭成功\n");
    	}
    	else
    	{
    		log(DEBUG,"Socket关闭失败\n");
    	}
    	xSemaphoreGive( xSocketSemaphore );
    	return SOCKET_OK;
    }
    else
    {

        log(WARN,"链接服务器失败，有其他任务占用资源\n");
        return  SOCKET_CLOSE_BUSY;
    }

}




uint8_t socket_is_ok( void )
{
    return (devCom->isOK());
}
void socket_close_f( void )
{

    socket_set_status(SOCKET_CLOSE_STATUS);

    devCom->close();

    log(ERR,"SOCKET暴力关闭\n");

    xSemaphoreGive( xSocketSemaphore );

}


void socket_close( void )
{

    //if( xSemaphoreTake( xSocketSemaphore, 45000 ) == pdTRUE )
    {
        if( socketStatus != SOCKET_CLOSE_STATUS)
        {
            for(int8_t id = 0 ;  id < SOCKET_CONNECT_MAX ; id++)
            {
                sockeArry[id].len = 0;
                sockeArry[id].status =  SOCKET_INIT;
                sockeArry[id].useFlag = FALSE;
                sockeArry[id].msg = NULL;
            }

            socket_set_status(SOCKET_CLOSE_STATUS);

            devCom->close();
        }
        else
        {
            log(INFO,"SOCKET已经关闭，不用重复关闭\n");
        }
        
        //xSemaphoreGive( xSocketSemaphore );
    }
   // else
    {
        //log(WARN,"close关闭失败\n");
    }
}



socketOpsType   socket = 
{
  .isOK =         socket_is_ok,
  .disconnect =   socket_disconnect,
  .connect =      socket_connect,
  .read =         socket_read,
  .read_buffer =  socket_read_data,
  .send =         socket_write,
  .close =        socket_close,
};




void socket_init( void )
{   
    for( uint8_t i = 0; i < SOCKET_CONNECT_MAX ;i++)
    {
    	sockeArry[i].id = i;
        sockeArry[i].len = 0;
        sockeArry[i].msg = NULL;
        sockeArry[i].status = SOCKET_INIT;
        sockeArry[i].useFlag = FALSE;
    }

    socketStatus = SOCKET_CLOSE_STATUS;

}


void socket_compon_init(void )/*must--标识必须要建立任务 lev-是正常时候建立任务 */
{
	uint32_t runModule = config.read(CFG_SYS_NET_TYPE , NULL);
	
	if( runModule == TSLNetType_TSLGPRS )
	{
		log(DEBUG,"使用GPRS联网\n");
		devCom = &gsm;
	}
	else if( runModule == TSLNetType_TSLWIFI )
	{
		log(DEBUG,"使用WIFI联网\n");
		devCom = &wifi;
	}
	else if( runModule == TSLNetType_TSLEthernet )
	{
		log(DEBUG,"使用以太网联网\n");
		devCom = &eth;
	}
	else
	{
		log(INFO,"联网方式选择未初始化 , 不能运行，恢复出厂设置\n");
		config.write(CFG_SET_RESTORE , NULL , FALSE);
	}
	
	socket_init();
    
    devCom->init();

}




#include "stdlib.h"
#include "sim800c.h"
#include "beep.h"
#include "bsp.h"
#include "usart.h"
#include "modules_init.h"
#include "config.h"
#include "timer.h"
#include "socket.h"


portBASE_TYPE           xHigherPriorityTaskWoken = pdFALSE;
void                    *gsmPort = NULL;
static xTaskHandle         gsmTask;
gsmRunStatusEnum        gsmRun = GSM_INIT;
gsmRunEnum              gsmRunStatus = GSM_POWER_ON;
__IO uint8_t            gsmReceiveMode = DATA_MODE;
__IO uint8_t            receiveTimeStart = FALSE;
__IO uint32_t           receiveTimecnt = 0;
__IO int8_t                receiveSocketConnectStatus = FALSE;
__IO int8_t                sendStartStatus = FALSE;
static uint8_t            receiveReturnBuff[50], receiveReturnBuffLeng;


void gsm_receive_process( void );

void gsm_receive_byte( uint8_t ch)
{
    receiveTimeStart = TRUE;
    receiveTimecnt =0;
    //putchar(ch);
    receiveData.rxBuff[receiveData.len++] = ch;

    if ( receiveData.len > RECEIVE_MAX)
    {
        log(WARN,"recv too length\n");
        memset(&receiveData , 0x00 ,sizeof(usartReceiveDataType));
    }    
}

void gsm_timer_isr(void)
{
    if( receiveTimeStart)
    {
        if( receiveTimecnt++ >= 2 )
        {
            receiveTimeStart = FALSE;
            receiveTimecnt =0;

            if((gsmReceiveMode == COMMAND_MODE)||(gsmRunStatus != GSM_INIT_SUCCESS))
            {
                receiveData.receiveFinsh = TRUE;
            }
            else
            {
                xSemaphoreGiveFromISR( xUsartNetSemaphore, &xHigherPriorityTaskWoken );
                portEND_SWITCHING_ISR(xHigherPriorityTaskWoken );
            }
        }
    }
}

void gsm_command(uint8_t *cmd , uint16_t length)
{
    //log(DEBUG,"GSM SEND = %s\r\n" , cmd);
    serial.puts(gsmPort , cmd , length);
    
}

uint8_t gsm_send_receive(uint8_t *cmd , uint16_t length , usartReceiveDataType *respone , uint16_t timeout)
{
    uint16_t timeCnt = timeout/10;

    gsmReceiveMode = COMMAND_MODE;
    
    gsm_command(cmd , length);

    while(timeCnt--)
    {
        if(receiveData.receiveFinsh == TRUE)
        {
            //log(DEBUG,"����� = %s\n" , receiveData.rxBuff+2);
            memcpy(respone , &receiveData , sizeof(usartReceiveDataType));         
            memset( &receiveData ,0x00 ,  sizeof(usartReceiveDataType));
            gsmReceiveMode = DATA_MODE;
            return TRUE;
        }
        sys_delay(10);
    }
    gsmReceiveMode = DATA_MODE;
    return FALSE;

}

uint8_t gsm_send_command(uint8_t *cmd , uint16_t length , uint16_t timeout , uint8_t repert , uint8_t *checkData)
{
    usartReceiveDataType respone;

    while (1)
    {
        if( gsm_send_receive(cmd , length , &respone , timeout) ==TRUE )
        {
            //log(DEBUG,"��������, DATA = %s \r\n" , respone.rxBuff);
            if( strstr (respone.rxBuff , (char const *)checkData) != NULL)
            {
                return TRUE;
            }
            sys_delay(timeout);
        }
        
        if(repert != 0xFF)
        {
            if( --repert == 0 )    return FALSE;
        }
          
    }

}


uint8_t gsm_send_receive_repeat(uint8_t *cmd , uint16_t length ,  uint16_t timeout , uint8_t repert ,usartReceiveDataType *respone )
{

    while (1)
    {
        if( gsm_send_receive(cmd , length , respone , timeout) ==TRUE )
        {
            return TRUE;
        }

        if(repert != 0xFF)
        {
            if( --repert == 0 )    return FALSE;
        }
    }
}

uint8_t gsm_sen_apn(uint8_t repeat)
{
    usartReceiveDataType respone;

    while(repeat--)
    {
        if( gsm_send_receive_repeat("AT+COPS?\r\n" , strlen("AT+COPS?\r\n") ,500 , 30 , &respone) == TRUE)
        {
            log(DEBUG,"COPS=%s\n" , respone.rxBuff);
            if( strstr(respone.rxBuff , "MOBILE") != NULL)
            {
                log(DEBUG,"�й��ƶ�SIM���������й��ƶ�APN = cmiotqipeng.js\n");
                if( gsm_send_command("AT+CSTT=\"cmiotqipeng.js\"\r\n" , strlen("AT+CSTT=\"cmiotqipeng.js\"\r\n") ,500 , 30 , "\r\nOK\r\n") == TRUE)
                {
                    return TRUE;
                }
            }
            else if( strstr(respone.rxBuff , "UNICOM") != NULL)
            {
                log(DEBUG,"�й���ͨSIM���������й���ͨAPN = unim2m.njm2mapn\n");
                if( gsm_send_command("AT+CSTT=\"unim2m.njm2mapn\"\r\n" , strlen("AT+CSTT=\"unim2m.njm2mapn\"\r\n") ,500 , 30 , "\r\nOK\r\n") == TRUE)
                {
                    return TRUE;
                }
            }

        }

    }
    return FALSE;
}

uint8_t gsm_check_csq( uint8_t repeat , uint8_t *getCsq )
{
    uint8_t csq = 0;
    char csqtemp[2]={0x00};
    usartReceiveDataType respone;

    while(repeat--)
    {
        if( gsm_send_receive_repeat("AT+CSQ\r\n" , strlen("AT+CSQ\r\n") ,500 , 30 , &respone) == TRUE)
        {
            
            if( strstr(respone.rxBuff , "\r\nOK\r\n") != NULL )
            {
                memcpy(csqtemp , &respone.rxBuff[8], 2);
                csq = atoi(csqtemp);
            }

            log(DEBUG,"GSM CSQ = %d \n" , csq);

            *getCsq = csq;

            if( csq > 10 )
            {
                return TRUE;
            }
            log(INFO,"�����źŲ� , csq = %d\n" , csq);\
            sys_delay(1500);
        }
    }
    return FALSE;
}


/*
* HIGH - POWER OFF
* LOW - POWER ON
*/
void gsm_power_resert( void)
{
    pin_ops.pin_write(GSM_POWER_PIN , PIN_HIGH ); 
    sys_delay(3000);
    pin_ops.pin_write(GSM_POWER_PIN , PIN_LOW);
    sys_delay(1000);
}



uint8_t  gsm_power_on( void )
{
    if ( gsm_send_command("AT\r\n" , strlen("AT\r\n") , 500 ,20 , "\r\nOK\r\n") == TRUE )
    {          
        log(INFO,"GSM�����ɹ�\n");
        return TRUE;
    }

    log(INFO,"GSM����ʧ��\n");
    return FALSE;
}


uint8_t gsm_module_init( void )
{
    while( gsmRunStatus!= GSM_INIT_SUCCESS)
    {
        switch(gsmRunStatus)
        {
        case GSM_POWER_OFF:
        {
            gsm_power_resert();
            log(DEBUG,"GSMģ��ػ��ɹ�\n");
            gsmRunStatus = GSM_POWER_ON;
        }break;
        case GSM_POWER_ON:
        {
            log(DEBUG,"GSMģ�鿪��\n");
            //gsmRunStatus = ( gsm_power_on() == FALSE)?GSM_POWER_OFF:GSM_CLOSE_ECHO;
            gsm_assert(gsm_power_on() , GSM_CLOSE_ECHO);
        }break;
        case GSM_CLOSE_ECHO:
        {
            log(DEBUG,"�رջ���\n");
            //gsmRunStatus = ( gsm_send_command("ATE0\r\n" , strlen("ATE0\r\n") , 1000 , 30 , "\r\nOK\r\n") == FALSE)?GSM_POWER_OFF:GSM_SET_IPR;
            gsm_assert(gsm_send_command("ATE0\r\n" , strlen("ATE0\r\n") , 1000 , 30 , "\r\nOK\r\n") , GSM_SET_IPR);
        }break;
        case GSM_SET_IPR:
        {
            log(DEBUG,"���ò�����\n");
            //gsmRunStatus = ( gsm_send_command("AT+IPR=115200\r\n" , strlen("AT+IPR=115200\r\n") , 1000 , 30 , "\r\nOK\r\n") == FALSE)?GSM_POWER_OFF:GSM_READ_SIM;
            gsm_assert(gsm_send_command("AT+IPR=115200\r\n" , strlen("AT+IPR=115200\r\n") , 1000 , 30 , "\r\nOK\r\n"),GSM_READ_SIM);
            
        }break;

        case GSM_READ_SIM:
        {
            log(DEBUG,"��ȡSIM��\n");
            //gsmRunStatus = ( gsm_send_command("AT+CPIN?\r\n" , strlen("AT+CPIN?\r\n") , 1000 , 30 , "\r\n+CPIN: READY\r\n") == FALSE)?GSM_POWER_OFF:GSM_READ_CSQ;
            gsm_assert(gsm_send_command("AT+CPIN?\r\n" , strlen("AT+CPIN?\r\n") , 1000 , 15 , "\r\n+CPIN: READY\r\n"),GSM_READ_CSQ);
        }break;
        case GSM_READ_CSQ:
        {
          
            uint8_t csq;
            log(DEBUG,"��ȡ�ź�����\n");
            //gsmRunStatus = ( gsm_check_csq(30 , &csq) == FALSE)?GSM_POWER_OFF:GSM_READ_NET;
            gsm_assert(gsm_check_csq(30 , &csq),GSM_READ_NET);

        }break;
        case GSM_READ_NET:
        {
            log(DEBUG,"��ȡ����ע��״̬\n");
            //gsmRunStatus = ( gsm_send_command("AT+CREG?\r\n" , strlen("AT+CREG?\r\n") , 1000 , 30 , "\r\n+CREG: 0,1\r\n") == FALSE)?GSM_POWER_OFF:GSM_READ_GPRS;
            gsm_assert(gsm_send_command("AT+CREG?\r\n" , strlen("AT+CREG?\r\n") , 1000 , 30 , "\r\n+CREG: 0,1\r\n"),GSM_READ_GPRS);
        }break;

        case GSM_READ_GPRS:
        {
            log(DEBUG,"��ȡGPRS����״̬\n");
            //gsmRunStatus = ( gsm_send_command("AT+CGATT?\r\n" , strlen("AT+CGATT?\r\n") , 1000 , 30 , "\r\n+CGATT: 1\r\n") == FALSE)?GSM_POWER_OFF:GSM_SET_SEND_MODE;
            gsm_assert(gsm_send_command("AT+CGATT?\r\n" , strlen("AT+CGATT?\r\n") , 1000 , 30 , "\r\n+CGATT: 1\r\n"),GSM_SET_SEND_MODE);
        }break;
        case GSM_SET_SEND_MODE:
        {
            log(DEBUG,"���ÿ��ٷ���ģʽ\n");
            //gsmRunStatus = ( gsm_send_command("AT+CIPQSEND=1\r\n" , strlen("AT+CIPQSEND=1\r\n") , 1000 , 30 , "\r\nOK\r\n") == FALSE)?GSM_POWER_OFF:GSM_SET_MUX;
            gsm_assert(gsm_send_command("AT+CIPQSEND=0\r\n" , strlen("AT+CIPQSEND=0\r\n") , 1000 , 30 , "\r\nOK\r\n"),GSM_SET_MUX);
        }break;


        case GSM_SET_MUX:
        {
            log(DEBUG,"���ö�����ģʽ\n");
            usartReceiveDataType respone;
            if((gsm_send_receive_repeat("AT+CIPMUX?\r\n" , strlen("AT+CIPMUX?\r\n") , 1000 , 90 , &respone) != FALSE))
            {
                if(strstr(respone.rxBuff , "+CIPMUX: 1") == NULL)
                {
                    gsmRunStatus = ( gsm_send_command("AT+CIPMUX=1\r\n" , strlen("AT+CIPMUX=1\r\n") , 1000 , 2, "\r\nOK\r\n") == FALSE)?GSM_POWER_OFF:GSM_CLOSE_CONNECT;
                }
                else
                {
                    gsmRunStatus = GSM_CLOSE_CONNECT;
                }
            }
            /*
            if((gsm_send_receive_repeat("AT+CIPMODE?\r\n" , strlen("AT+CIPMODE?\r\n") , 1000 , 90 , &respone) != FALSE))
            {
                if(strstr(respone.rxBuff , "+CIPMODE: 0") == NULL)
                {
                    gsmRunStatus = ( gsm_send_command("AT+CIPMODE=0\r\n" , strlen("AT+CIPMODE=0\r\n") , 1000 , 2, "\r\nOK\r\n") == FALSE)?GSM_POWER_OFF:GSM_CLOSE_CONNECT;
                }
                else
                {
                    gsmRunStatus = GSM_CLOSE_CONNECT;
                }
            }*/
        }break;
        case GSM_CLOSE_CONNECT:
        {
            log(DEBUG,"�ر����缤��\n");
            if( gsm_send_command("AT+CIPSHUT\r\n" , strlen("AT+CIPSHUT\r\n") , 1000 , 30 , "\r\nSHUT OK\r\n") == FALSE)
            {
                gsmRunStatus = GSM_POWER_OFF;
            }
            else
            {
                gsmRunStatus = GSM_SET_APN;
                sys_delay(3000);
            }
        }break;
        case GSM_SET_APN:
        {
            log(DEBUG,"����APN\n");
            //gsmRunStatus = ( gsm_sen_apn(30) == FALSE)?GSM_POWER_OFF:GSM_ON_GPRS;
            gsm_assert( gsm_sen_apn(30),GSM_ON_GPRS);

        }break;
        case GSM_ON_GPRS:
        {
            log(DEBUG,"����GPRS\n");
            //gsmRunStatus = ( gsm_send_command("AT+CIICR\r\n" , strlen("AT+CIICR\r\n") , 1000 , 90 , "\r\nOK\r\n") == FALSE)?GSM_POWER_OFF:GSM_READ_IP;
            gsm_assert(gsm_send_command("AT+CIICR\r\n" , strlen("AT+CIICR\r\n") , 1000 , 90 , "\r\nOK\r\n"),GSM_READ_IP);
        }break;
        case GSM_READ_IP:
        {
            usartReceiveDataType respone;
            log(DEBUG,"��ȡIP\n");
            if((gsm_send_receive_repeat("AT+CIFSR\r\n" , strlen("AT+CIFSR\r\n") , 1000 , 90 , &respone) == FALSE))
            {
                gsmRunStatus = GSM_POWER_OFF;
            }
            else
            {
                if( strstr(respone.rxBuff , "ERROR") == NULL)
                {
                    log(DEBUG,"�豸IP = %s\n" , respone.rxBuff+2);
                    gsmRunStatus = GSM_INIT_FINSG;
                }
            }
        }break;

        case GSM_INIT_FINSG:
        {
            log(DEBUG,"GSM��ʼ����ɣ����Խ���tcp����\n");
            gsmRunStatus = GSM_INIT_SUCCESS;            
        }break;
        default:
        {
            gsmRunStatus = GSM_POWER_OFF;
            log(WARN,"GSM��ʼ��û�����״̬\n ���������������⣬��λ�豸\n");
        }break;
        }          
    }
    
    if( gsmRunStatus == GSM_INIT_SUCCESS)
    {
        gsmRun = GSM_RECEIVE;
        socket_set_status(SOCKET_WORKING_STATUS);
        return TRUE;
    }
    
    return FALSE;

}

/*
void rx_data_analisys(char *str , int len)
{
    uint8_t *otp = (uint8_t *)str;
    
    while(len--)
    {
        if(aiot_strcmp(otp , "\r\n+" , strlen("\r\n+")) == true )
        {
            uint8_t *pst = otp+strlen( "\r\n+");
            if(aiot_strcmp(pst , "RECEIVE," , strlen("RECEIVE,")) == true )
            {
                int len =0 , id =0 ,i =0;
                char lenBuff[4] = {0};
                sockeArryType   *socket;
                
                //printf("socket��ȡ������\n");
                
                pst = pst+strlen( "RECEIVE,");
                
                id = *pst++ -'0';

                memset(lenBuff , 0x00 , 4);
                pst++;
                while(*pst != ':')
                {
                    lenBuff[i++] = *pst++;
                }
                len = atoi(lenBuff);
                
                if( len > 0 )
                {
                    socket =   socket_read_obj(id);
                    if(( socket->useFlag == TRUE ) &&(socket->id == id))
                    {                        
                        socket->status = SOCKET_READ;
                        //log(INFO," socket = %d ,len = %d , data len = %d\n" , id , len , socket->len);
                        if( socket->msg != NULL)
                        {
                            memcpy(socket->msg+socket->len , pst+3 ,len);
                            socket->len += len;
                            //memcpy(socket->msg , pst+3 ,len);
                            //socket->len = len;
                            //log(DEBUG,"recv data =%s\n" , socket->msg);
                        }
                        else
                        {
                            log(WARN,"��ǰsocket��Buffδ����\n");
                        }
                    }
                    else
                    {
                        log(WARN,"socketδ���� , ���߲��ǵ�ǰId , id=%d , socket id =%d\n" , id , socket->id);
                    }
                }
                otp = pst+3+len;
                continue;

            }
            else if( aiot_strcmp(pst , "PDP: DEACT" , strlen("PDP: DEACT")) == true )
            {
                printf("PDP�������ʧ�ܣ����SIM����������\n");
                
                gsm_close();
                socket_clear_all();
                
                otp = pst+strlen("PDP: DEACT");
                continue;
            }
            else if( aiot_strcmp(pst , "CPIN: NOT READY" , strlen("CPIN: NOT READY")) == true)
            {
                printf("SIM����ȡʧ��\n");
                
                gsm_close();
                socket_clear_all();
                otp = pst+ strlen("CPIN: NOT READY");
                continue;
            }            
        }
        else if(aiot_strcmp(otp , "\r\n>" , strlen("\r\n>")) == true )
        {
            //printf("׼����������\n");
            otp = otp + strlen("\r\n>");
            xSemaphoreGive( xSocketCmdSemaphore );
            continue;
        }
        
        else if( aiot_strcmp(otp+3 , ", CONNECT FAIL" , strlen(", CONNECT FAIL")) != NULL)
        {

            receiveSocketConnectStatus = FALSE;

            xSemaphoreGive( xSocketConnectSemaphore);  
            otp = otp+strlen("\r\n0, CONNECT FAIL\r\n");

            //log(INFO ,"���ط���������ʧ��\n" );

            continue;
        }  
        else if( aiot_strcmp(otp+3 , ", CONNECT OK" , strlen(", CONNECT OK")) != NULL)
        {

            receiveSocketConnectStatus = TRUE;
            
            xSemaphoreGive( xSocketConnectSemaphore);
            otp = otp+strlen("\r\n0, CONNECT OK\r\n");

            //log(INFO ,"���ط��������ӳɹ�\n" );

            continue;
        }
        else if(aiot_strcmp(otp+3 , ", CLOSED\r\n" , strlen(", CLOSED\r\n")) == true )
        {
            uint8_t *pst = otp+2;
            int id =0;
            sockeArryType   *socket;
            
            id = *pst - '0';

            log(WARN,"�ر�socket = %d\n" , id);

            socket =   socket_read_obj(id);
            if(socket->useFlag == TRUE)
            {
                socket->len = 0;
                socket->status =  SOCKET_CLOSE;
                socket->useFlag = FALSE;
                socket->msg = NULL;
            }
            else
            {
                log(WARN,"socketδʹ��\n");
            }
            otp = pst+strlen("\r\n0, CLOSED\r\n");
            
            continue;
        }
        else if(strstr(otp+3 , "SEND OK") != NULL )
        {
            uint8_t *pst = otp+2;
            //int socket =0;

            //socket = *pst - '0';
            
            //log(DEBUG,"socket ������� = %d\n" , socket);
            otp = pst+strlen(", SEND OK\r\n");
            
            xSemaphoreGive( xSocketSendSemaphore );
            
            continue;
        }
        else if(aiot_strcmp(otp+3 , ", SEND FAIL\r\n" , strlen(", SEND FAIL\r\n")) == true )
        {
            uint8_t *pst = otp+2;
            int socket =0;

            socket = *pst - '0';
            
            log(WARN,"socket ����ʧ�� = %d\n" , socket);
            
            otp = pst+strlen(", SEND FAIL\r\n");
            
            xSemaphoreGive( xSocketSendSemaphore );
            
            continue;
        }
        //
        else if(aiot_strcmp(otp , "\r\nDATA ACCEPT:" , strlen("\r\nDATA ACCEPT:")) == true )
        {
            otp = otp+strlen("\r\nDATA ACCEPT:0,");

            xSemaphoreGive( xSocketSendSemaphore );

            continue;
        }
        else if(aiot_strcmp(otp , "\r\nERROR" , strlen("\r\nERROR")) == true )
        {

            otp = otp+strlen("\r\nERROR");
            if( sendStartStatus )
            {
                sendStartStatus = FALSE;
                xSemaphoreGive( xSocketCmdSemaphore );
            }
            continue;
        }
        otp++;
    }
    //log(DEBUG,"\n���ݽ������\n");
    
}*/






uint8_t read_dat_type(char *msg, uint16_t *outlen)
{
    if (aiot_strcmp((uint8_t *)msg, "\r\n+RECEIVE,", strlen("\r\n+RECEIVE,")) == true)
    {
        *outlen = strlen("\r\n+RECEIVE,");

        return GSM_RECEIVE_TYPE;
    }

    else if (aiot_strcmp((uint8_t *)msg, "\r\nPDP: DEACT", strlen("\r\nPDP: DEACT")) == true)
    {
        *outlen = strlen("\r\nPDP: DEACT");

        return GSM_DEACT_TYPE;
    }

    else if (aiot_strcmp((uint8_t *)msg, "\r\nCPIN: NOT READY", strlen("\r\nCPIN: NOT READY")) == true)
    {
        *outlen = strlen("\r\nCPIN: NOT READY");

        return GSM_NOT_READY_TYPE;
    }

    else if (aiot_strcmp((uint8_t *)msg, "\r\n>", strlen("\r\n>")) == true)
    {
        *outlen = strlen("\r\n>");

        return GSM_SEND_READY;
    }

    
    else if (aiot_strcmp((uint8_t *)(msg + 3), ", CONNECT FAIL\r\n", strlen(", CONNECT FAIL\r\n")) == true)
    {
        *outlen = strlen("\r\n0, CONNECT FAIL\r\n");

        return GSM_CONNECT_FAIL_TYPE;
    }

    else if (aiot_strcmp((uint8_t *)(msg+3), ", CONNECT OK\r\n", strlen(", CONNECT OK\r\n")) == true)
    {
        *outlen = strlen("\r\n0, CONNECT OK\r\n");

        return GSM_CONNECT_OK_TYPE;
    }

    else if (aiot_strcmp((uint8_t *)(msg + 3), ", CLOSED\r\n", strlen(", CLOSED\r\n")) == true)
    {
        *outlen = strlen("\r\n0, CLOSED\r\n");

        return GSM_CLOSE_TYPE;
    }
    

    else if (aiot_strcmp((uint8_t *)(msg + 3), ", SEND OK\r\n", strlen(", SEND OK\r\n")) == true)
    {
        *outlen = strlen("\r\n0, SEND OK\r\n");

        return GSM_SEND_OK_TYPE;
    }

    else if (aiot_strcmp((uint8_t *)(msg + 3), ", SEND FAIL\r\n", strlen(", SEND FAIL\r\n")) == true)
    {
        *outlen = strlen("\r\n0, SEND FAIL\r\n");

        return GSM_SEND_FAIL_TYPE;
    }

    else if (aiot_strcmp((uint8_t *)(msg), receiveReturnBuff, receiveReturnBuffLeng) == true)
    {
        *outlen = receiveReturnBuffLeng;

        return GSM_DATA_ACCEPT_TYPE;
    }

    else if (aiot_strcmp((uint8_t *)(msg), "\r\nERROR\r\n", strlen("\r\nERROR\r\n")) == true)
    {
        *outlen = strlen("\r\nERROR\r\n");

        return GSM_ERROR_TYPE;
    }

    return GSM_RECEIVE_NULL;
}

uint16_t gsm_receive_close_handle(char *msg)
{
    int id = 0;
    char *port = msg + 2;
    sockeArryType   *socket;

    id = *port - '0';

    if (id > 5)
    {
        return false;
    }
    log(DEBUG,"�ر�socket = %d\n", id);
    socket = socket_read_obj(id);
    if (socket->useFlag == TRUE)
    {
        socket->len = 0;
        socket->status = SOCKET_CLOSE;
        socket->useFlag = FALSE;
        socket->msg = NULL;
    }
    else
    {
        log(WARN, "socketδʹ��\n");
    }

    return true;
}

uint16_t gsm_receive_data_handle(char *msg)
{
    char *data = msg;
    uint8_t id = 0xFF , i = 0;
    uint16_t len = 0 , size = 0;
    char lenBuff[10] = { 0 };
        sockeArryType   *socket;

    //log(DEBUG,"recv :%s\n" , data);

    id = data[0] - '0';
    if (id > 5)
    {
        log(DEBUG," error id =%d\n" ,id);
        return 0;
    }

    data += 2;
    memset(lenBuff, 0x00, sizeof(lenBuff));
    while (*data != ':')
    {
        lenBuff[i++] = *data++;
        if (i > 5)
        {
            log(DEBUG," error len =%s , len len = %d \n", lenBuff , i);
            return 0;
        }
    }
    len = atoi(lenBuff);
    if ((len > 2000)||( len == 0))
    {
        return 0;
    }
    
    data+=3;
    socket =   socket_read_obj(id);
    if(( socket->useFlag == TRUE ) &&(socket->id == id))
    {                        
        socket->status = SOCKET_READ;
        //log_arry(DEBUG,"recv " , data , len );
        if( socket->msg != NULL)
        {
            if( socket->len+len < socket->maxSize )
            {
                memcpy(socket->msg+socket->len , data ,len);
                socket->len += len;
            }
            else
            {
                log(WARN,"����BUFF���� , len = %d , socket len = %d , max size = %d\n" , len , socket->len , socket->maxSize);
            }
        }
        else
        {
            log(WARN,"��ǰsocket��Buffδ����\n");
        }
    }
    else
    {
        log(WARN,"socketδ���� , ���߲��ǵ�ǰId , id=%d , socket id =%d\n" , id , socket->id);
    }
    data += len; 

    //log(DEBUG,"buffer =%s\n", databuffer);
    //log(DEBUG,"data = %d , msg =%d\n" , data , msg);

    size = data - msg;

    //log(DEBUG,"data size=%d\n", size);

    return size;
}

void rx_data_analisys(char *str, int length)
{
    char *otp = str, *pst = NULL;
    uint8_t type = GSM_RECEIVE_NULL;
    uint16_t typeLen = 0;

    
    do{
        //log(DEBUG," length = %d , data=%s\n", length , otp);
        typeLen = 0;
        if ((pst = strstr(otp, "\r\n")) != NULL)
        {
            length -= (pst - otp);
            if ((type = read_dat_type(pst, &typeLen)) != GSM_RECEIVE_NULL)
            {
                //printf("Rece type=%d , len =%d\n", type, typeLen);
                switch (type)
                {
                    case GSM_RECEIVE_TYPE:
                    {
                        uint16_t size = 0;

                        if ((size = gsm_receive_data_handle(pst + typeLen)) != 0)
                        {
                            typeLen  += size;
                        }
                    }break;
                    case GSM_NOT_READY_TYPE:
                    case GSM_DEACT_TYPE:
                    {
                        if(type== GSM_DEACT_TYPE)
                            log(INFO,"PDP�������ʧ�ܣ����SIM����������\n");
                        else if(type == GSM_NOT_READY_TYPE)
                            log(INFO,"SIM����ȡʧ��\n");

                        gsm_close();
                        socket_clear_all();

                    }break;

                    case GSM_SEND_READY:
                    {
                        xSemaphoreGive(xSocketCmdSemaphore);
                    }break;
                    case GSM_CONNECT_FAIL_TYPE:
                    case GSM_CONNECT_OK_TYPE:
                    {
                        receiveSocketConnectStatus = ((type==GSM_CONNECT_OK_TYPE)?TRUE:FALSE);
                        xSemaphoreGive(xSocketConnectSemaphore);

                    }break;

                    case GSM_CLOSE_TYPE:
                    {
                        gsm_receive_close_handle(pst);
                    }break;
                    case GSM_DATA_ACCEPT_TYPE:
                    {
                        log(DEBUG,"gsm moduce accpet data\n");
                    }//break;
                    case GSM_SEND_FAIL_TYPE:
                    case GSM_SEND_OK_TYPE:
                    {
                        xSemaphoreGive(xSocketSendSemaphore);
                    }break;

                    case GSM_ERROR_TYPE:
                    {
                        if (sendStartStatus)
                        {
                            sendStartStatus = FALSE;
                            xSemaphoreGive(xSocketCmdSemaphore);
                        }
                    }break;
                    default: log(WARN,"no this type -gsm \n"); break;
                }
                otp = pst + typeLen;
                length -= typeLen;
            }
            else
            {
                length -= 2;
                otp += 2;
            }
            
        }
        else
        {
            length--;
            otp++;
        }
        if(( length < 0) || ( length > RECEIVE_MAX))
        {
            log(INFO,"SIM800C����ʧ�� , len = %d\n" , length);
            log(DEBUG,"data=%s\n" , str);
            return;
        }
    } while ((length)&&( otp != NULL ));

}

void gsm_module_receive( void )
{
    usartReceiveDataType    tempBuff;
    
    if( xSemaphoreTake( xUsartNetSemaphore, 3000 ) == pdTRUE )
    {

        //vTaskSuspendAll();
        
        memset(&tempBuff  , 0x00 , sizeof(usartReceiveDataType));
        memcpy(&tempBuff  , &receiveData , sizeof(usartReceiveDataType));
        memset(&receiveData ,0x00 ,  sizeof(usartReceiveDataType));
        
        rx_data_analisys(tempBuff.rxBuff , tempBuff.len);

        //xTaskResumeAll();
    }

}


void sim800c_init(void)
{
    gsmPort = serial.open("serial2");
    
    if( gsmPort == NULL)
    {
        beep.write(BEEP_ALARM);
        return ;
    }
    
    pin_ops.pin_mode(GSM_POWER_PIN , PIN_MODE_OUTPUT);
    pin_ops.pin_write(GSM_POWER_PIN ,PIN_LOW);
    
    serial.init(gsmPort  , 115200 ,gsm_receive_byte);
    timer.creat(1 , TRUE , gsm_timer_isr);
}

static void gsm_task( void const *pvParameters)
{
    sim800c_init();
    
    gsmRunStatus = GSM_POWER_ON; 
    gsmRun = GSM_INIT;
    
    for( ; ; )
    {
        switch(gsmRun)
        {
            case GSM_INIT:
            {
                gsm_module_init();                
            }break;
            case GSM_RECEIVE:
            {
                gsm_module_receive();
            }break;
            default:break;
        }
        //read_task_stack(__func__,gsmTask);
    }
}


void creat_gsm_task( void )
{
    osThreadDef( gsm, gsm_task , osPriorityRealtime, 0, configMINIMAL_STACK_SIZE*20);
    gsmTask = osThreadCreate(osThread(gsm), NULL);
    configASSERT(gsmTask);
}



/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

int8_t gsm_connect_server ( int8_t id , uint8_t *ip , uint16_t port)
{
    usartReceiveDataType respone;
    uint8_t sendBuff[128] ,sendSize =0, connectip[100];

    memset(sendBuff , 0x00 , 128);
    memset(connectip , 0x00 , 100);

    
    if((strstr((char *)ip,"com")!=NULL)||(strstr((char *)ip,"cn")!=NULL))
    {
        memcpy(connectip , ip , strlen((char *)ip));
    }
     else if((strstr((char *)ip,".")!=NULL))
    {
       log(INFO,"��ַ��string 192.168.1.2���� ��%s��\n",ip);memcpy(connectip , ip , strlen((char *)ip));
    }
    else
    {
        sprintf( (char *)connectip , "%d.%d.%d.%d" , ip[0],ip[1],ip[2],ip[3]);
    }
    
    
    sendSize = sprintf((char *)sendBuff , "AT+CIPSTART=%d,\"TCP\",\"%s\",\"%d\"\r\n" , id , connectip , port);

    if( gsm_send_receive_repeat(sendBuff,sendSize , 30000 , 1 ,&respone) == TRUE)
    {
        if( strstr(respone.rxBuff , "\r\nERROR\r\n") != NULL)
        {
            if( strstr(respone.rxBuff , "ALREADY CONNECT") != NULL)
            {
                log(INFO,"�Ѿ���������������� , socket id = %d\n" , id);
                return SOCKET_OK;
            }
        }
        else if( xSemaphoreTake( xSocketConnectSemaphore, 30000 ) == pdTRUE )
        {
            if ( receiveSocketConnectStatus == TRUE)
            {
                log(INFO,"���ӷ������ɹ� , socket id = %d\n" , id);
                return SOCKET_OK;
            }
            else
            {
                log(WARN,"ģ�鷵�����ӷ�����ʧ�� , socket id = %d\n" , id);
                return SOCKET_CONNECT_FAIL;
            }
        }
        log(WARN,"���������Ӻ������յ�����, DATA = %s\n" , respone.rxBuff);
        rx_data_analisys(respone.rxBuff , respone.len);
        memset( &respone ,0x00 ,  sizeof(usartReceiveDataType));
        return SOCKET_CONNECT_TIMEOUT;
    }
    else
    {
        log(WARN,"GSM�������ӷ�����û����Ӧ�����ڽ��������⣬��λģ��");
        return SOCKET_CONNECT_NORES;
    }

}


int8_t gsm_write_data(uint8_t socketId , uint8_t *msg , uint16_t length)
{
    uint8_t comm[64] , cmdLength =0;
    //, endChar = 0x1A;

    sendStartStatus = TRUE;
    memset(comm , 0x00 , sizeof(comm));
    cmdLength = sprintf((char *)comm , "AT+CIPSEND=%d,%d\r\n" , socketId , length);
    memset(receiveReturnBuff , 0x00 , 50);
    receiveReturnBuffLeng = 0;
    receiveReturnBuffLeng = sprintf((char *)receiveReturnBuff , "\r\nDATA ACCEPT:%d,%d\r\n" , socketId , length);
        
    //log_arry(DEBUG," send" ,msg , length );
    gsm_command(comm , cmdLength);
    if( xSemaphoreTake( xSocketCmdSemaphore, 30000 ) == pdTRUE )
    {        
        if( sendStartStatus == FALSE )
        {            
            log(WARN,"scoket %d �������ݷ��ش���\n" , socketId);
            return SOCKET_SEND_RETURNERR;
        }
        gsm_command(msg ,length );
        //gsm_command(&endChar ,1 );
        if( xSemaphoreTake( xSocketSendSemaphore, 30000 ) == pdTRUE )
        {
            if( sendStartStatus == TRUE )
            {
                sendStartStatus = FALSE;
                return SOCKET_OK;
            }
            sendStartStatus = FALSE;
             log(WARN,"scoket %d �������ݷ���ʧ��\n" , socketId);
            return SOCKET_SEND_FAIL;
        }
        sendStartStatus = FALSE;
        log(WARN,"scoket %d ���������޽��ճɹ�����\n" , socketId);
        return SOCKET_SEND_NORESULT;
    }
    sendStartStatus = FALSE;
    log(WARN,"scoket %d ����������׼������\n" , socketId);
    return SOCKET_SEND_NOREADY;
}


uint8_t gsm_init_is_ok( void )
{
    return ((gsmRun ==GSM_RECEIVE)?TRUE:FALSE);
}

void gsm_close( void )
{
    gsmRunStatus = GSM_POWER_ON;
    gsmRun = GSM_INIT;
}

int8_t gsm_disconnect_tcp( int8_t id )
{
    usartReceiveDataType respone;
    
    uint8_t sendBuff[128] ,sendSize =0;

    memset(sendBuff , 0x00 , 128);

    sendSize = sprintf((char *)sendBuff , "AT+CIPCLOSE=%d\r\n" , id);

    if( gsm_send_receive_repeat(sendBuff ,sendSize , 10000 , 1 , &respone) == TRUE)
    {
        if( strstr( respone.rxBuff , ", CLOSE OK\r\n") != NULL)
        {
            log(DEBUG,"socket [%d] ���ӹرճɹ�\n" , id);
            return TRUE;
        }
    }

    log(DEBUG,"socket [%d] ���ӹر�ʧ��\n" , id);

    return FALSE;
}

devComType     gsm=
{
    .init = creat_gsm_task,
    .isOK = gsm_init_is_ok,
    .connect = gsm_connect_server,
    .disconnect = gsm_disconnect_tcp,
    .send = gsm_write_data,
    .close = gsm_close,
};


int8_t gsm_http_download_file ( uint8_t *fineName )
{

    return 0;
}

void gsm_http_close( void )
{

}

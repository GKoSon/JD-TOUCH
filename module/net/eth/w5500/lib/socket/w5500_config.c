#include "w5500_config.h"
#include "w5500_drv.h"
#include "wizchip_conf.h"
#include "unit.h"
#include "sysCfg.h"
#include "dhcp.h"
#include "timer.h"
#include "w5500\w5500.h"
#include "w_socket.h"
#include "socket.h"
#include "dns.h"

extern uint8_t gDATABUF[2048];
uint8_t hdcpTimerHandle = 0xFF;
uint8_t dnsTimerHandle = 0xFF;
__IO uint8_t w5500InitStatus = FALSE;
static xTaskHandle         w5500Task;

ETHRunStatusEnum ethstatus=ETH_INIT;

typedef struct
{
    uint8_t dnsIp[4];
}dnsIpTableType;


dnsIpTableType    dnsTable[]=
{
    {114,114,114,114},        //国内
    {114,114,115,115},        //国内
    {8,8,8,8},
    {8,8,4,4},
    {223,5,5,5},            //阿里
    {223,6,6,6},            //阿里
    {208,67,222,222},        //OpenDNS
    {208,67,220,220},        //OpenDNS

};

uint8_t w5500Mac[6]={0x00,0x08,0xdc,0x00,0x00,0x00};

const uint8_t dnsTableCnt = sizeof (dnsTable) / sizeof (dnsTable[0]);

void w5500_print_net_info( void )
{
    wiz_NetInfo    netinfo;

    wizchip_getnetinfo(&netinfo);

    log(ERR,"硬件MAC: %02X:%02X:%02X:%02X:%02X:%02X\r\n",netinfo.mac[0],netinfo.mac[1],netinfo.mac[2],netinfo.mac[3],netinfo.mac[4],netinfo.mac[5]);
    log(DEBUG,"SIP: %d.%d.%d.%d\r\n", netinfo.ip[0],netinfo.ip[1],netinfo.ip[2],netinfo.ip[3]);
    log(DEBUG,"GAR: %d.%d.%d.%d\r\n", netinfo.gw[0],netinfo.gw[1],netinfo.gw[2],netinfo.gw[3]);
    log(DEBUG,"SUB: %d.%d.%d.%d\r\n", netinfo.sn[0],netinfo.sn[1],netinfo.sn[2],netinfo.sn[3]);
    log(DEBUG,"DNS: %d.%d.%d.%d\r\n", netinfo.dns[0],netinfo.dns[1],netinfo.dns[2],netinfo.dns[3]);

}


void w5500_set_mac( void )
{
    
    uint8_t *deviceMac = NULL;
    
    config.read(CFG_BLE_MAC , (void **)&deviceMac);
    memcpy(w5500Mac+3 , deviceMac+3 , 3);
    
    setSHAR(w5500Mac);
    
}


uint8_t w5500_line_connect( void )
{
    return (getPHYCFGR() & PHYCFGR_LNK_ON);
}

void w5500_ip_assign( void )
{
    wiz_NetInfo    netinfo;
    
    memcpy(netinfo.mac , w5500Mac , 6);    
    getIPfromDHCP(netinfo.ip);
    getGWfromDHCP(netinfo.gw);
    getSNfromDHCP(netinfo.sn);
    getDNSfromDHCP(netinfo.dns);
    netinfo.dhcp = NETINFO_DHCP;
    
    wizchip_setnetinfo(&netinfo);
    
    log(DEBUG,"DHCP LEASED TIME : %d Sec.\r\n", getDHCPLeasetime());
    
    
}

void w5500_ip_update( void )
{
    wiz_NetInfo    netinfo;
    
    memcpy(netinfo.mac , w5500Mac , 6);    
    getIPfromDHCP(netinfo.ip);
    getGWfromDHCP(netinfo.gw);
    getSNfromDHCP(netinfo.sn);
    getDNSfromDHCP(netinfo.dns);
    netinfo.dhcp = NETINFO_DHCP;
    
    wizchip_setnetinfo(&netinfo);
    
    
    log(DEBUG,"DHCP LEASED TIME : %d Sec.\r\n", getDHCPLeasetime());
    
}

void w5500_ip_conflict( void )
{
    log(ERR,"no juggess\n");
}

uint8_t w5500_dhcp( void )
{
    uint16_t reCnt = 500;

    timer.start(hdcpTimerHandle);
    
    DHCP_init(DHCP_SN, gDATABUF);
    
    reg_dhcp_cbfunc(w5500_ip_assign, w5500_ip_update, w5500_ip_conflict);
    
    while(( DHCP_run() != DHCP_IP_LEASED )&&( --reCnt))
    {
        sys_delay(100);
    }
    
    DHCP_stop();
    
    timer.stop(hdcpTimerHandle);
    
    w5500_print_net_info();
    
    if( reCnt == 0 )
    {
        return FALSE;
    }
    
    return TRUE;
}


void spiMutexEnter(void)
{    
  __set_PRIMASK(1);
        
}

void spiMutexExit(void)
{
  __set_PRIMASK(0);
}


uint8_t w5500_config_init( void )
{
    uint8_t W5500SockBufSize[2][8] = {{2,2,2,2,2,2,2,2,},{2,2,2,2,2,2,2,2}}; 

    
    w5500_drv_init();
    
    w5500_drv_resert();
    
    timer.stop(dnsTimerHandle);

    reg_wizchip_spi_cbfunc(w5500_read_byte, w5500_write_byte);

    reg_wizchip_cs_cbfunc(w5500_chip_enable, w5500_chip_disable);

    reg_wizchip_cris_cbfunc(spiMutexEnter,spiMutexExit);
    
    if(ctlwizchip(CW_INIT_WIZCHIP,(void*)W5500SockBufSize) == -1)
    {
#if WIZSYSTEM_DEBUG == 1
        log(DEBUG,"W5500 initialized fail.\r\n");
#endif
        return FALSE;
    } 
    
    w5500_set_mac();
    
    w5500_print_net_info();
    
    return TRUE;
}


uint8_t w5500_init( void )
{
    DeviceIpType    *devIp;
    
    
    w5500_drv_resert();
    
    config.read(CFG_IP_INFO , (void **)&devIp);
    
    while( w5500_line_connect() == 0)
    {
        log(ERR,"网线未连接\n");
        sys_delay(1000);
    }
    
    log(ERR,"网线已经连接 devIp->dhcpFlag=%d\n",devIp->dhcpFlag);
    
    if( devIp->dhcpFlag == TRUE )
    {
        log(ERR,"DHCP获取设备IP信息\n");
        if( w5500_dhcp() != TRUE )
        {
            log(ERR,"DHCP失败\n");
            return FALSE;
        }
    }
    else
    {
        wiz_NetInfo    netinfo;
        log(ERR,"禁止DHCP获取设备IP信息 ,使用静态IP地址\n");
        log(DEBUG,"SIP: %d.%d.%d.%d\r\n", devIp->ip[0],devIp->ip[1],devIp->ip[2],devIp->ip[3]);
        log(DEBUG,"GAR: %d.%d.%d.%d\r\n", devIp->gateway[0],devIp->gateway[1],devIp->gateway[2],devIp->gateway[3]);
        log(DEBUG,"SUB: %d.%d.%d.%d\r\n", devIp->mark[0],devIp->mark[1],devIp->mark[2],devIp->mark[3]);
        log(DEBUG,"DNS: %d.%d.%d.%d\r\n", devIp->dns[0],devIp->dns[1],devIp->dns[2],devIp->dns[3]);
        
        netinfo.dhcp = NETINFO_DHCP;
        
        memcpy(netinfo.mac , w5500Mac , 6);    
        memcpy(netinfo.ip , devIp->ip , 4);
        memcpy(netinfo.gw , devIp->gateway , 4);
        memcpy(netinfo.sn , devIp->mark , 4);
        memcpy(netinfo.dns , devIp->dns , 4);
        
        wizchip_setnetinfo(&netinfo);
        
        sys_delay(100);
        
        w5500InitStatus = TRUE;
            
    }
    socket_set_status(SOCKET_WORKING_STATUS);

    w5500InitStatus = TRUE;
    
    
    
    return TRUE;
}

uint8_t    w5500_isOK( void )
{
    return w5500InitStatus;
}


int8_t w5500_disconnect( int8_t id )
{
    log(DEBUG,"关闭socket = %d \n" , id);
    
    eth_disconnect(id);
    
    return TRUE;
}

int8_t w5500_dns_to_host( uint8_t *name ,uint8_t *ip)
{
      int8_t repeat = 3;

    timer.start(dnsTimerHandle);
    
    memset(gDATABUF , 0x00 , sizeof(gDATABUF));
    
    DNS_init(DNS_SN, gDATABUF);
    
    log(DEBUG,"dns host:%s\n" ,name);
    
    while(repeat--)
    {
        for( uint8_t i = 0 ; i < dnsTableCnt; i++)
        {
              log(DEBUG,"DNS Srver ip=%d,%d,%d,%d\n",dnsTable[i].dnsIp[0],dnsTable[i].dnsIp[1],dnsTable[i].dnsIp[2],dnsTable[i].dnsIp[3]);
            if( DNS_run(dnsTable[i].dnsIp , name , ip) == 1)
            {
                  if( (ip[0]|ip[1]|ip[2]|ip[3]) == 0)
                {
                      continue;
                }
                log(DEBUG,"DNS success , ip=%d,%d,%d,%d\n",ip[0],ip[1],ip[2],ip[3]);
                
                timer.stop(dnsTimerHandle);
                
                return TRUE;
            }
            sys_delay(500);
        }
    }
    
    log(ERR,"DNS解析失败\n");
    
    timer.stop(dnsTimerHandle);
    
    return    FALSE; 
}



int8_t w5500_connect( int8_t id , uint8_t *ip , uint16_t port)
{
      int8_t rt = 0,temp=0;
    uint8_t des_ip[4]={0,0,0,0};
    
    if( ( rt = eth_socket(id,Sn_MR_TCP,0,0x00)) < 0 )
    {
          log(ERR,"W5500 socket err = %d\n" , rt);
        return rt;
    }
    
    if((strstr((char *)ip,"com")!=NULL)||(strstr((char *)ip,"cn")!=NULL))
    {
    log(DEBUG,"w5500 连接服务器, id =%d , ip = %s, port =%d\n" , id , ip , port);
        if( w5500_dns_to_host(ip , des_ip) == FALSE)
        {
            return SOCKET_CONNECT_ERR;
        }
    }
    
    
    if(strstr((char *)ip,".")!=NULL)
    {
    log(INFO,"地址是string 192.168.1.2这样 【%s】\n",ip);
        IPStrTO4ARR(des_ip , ip);
      log(DEBUG,"w5500 des_ip,  ip = %d.%d.%d.%d\n" , des_ip[0], des_ip[1], des_ip[2], des_ip[3] );
    }
    
    
else
    {
    log(DEBUG,"w5500 连接服务器 地址数数据, id =%d , ip = %d.%d.%d.%d, port =%d\n" , id , ip[0], ip[1], ip[2], ip[3] , port);
        for(temp=0;temp<4;temp++)
        {
            des_ip[temp]=ip[temp];
        }
    }
    
    log(ERR,"W5500 connect port = %d\n" , port);
    
    if( ( rt = eth_connect(id , des_ip , port) ) != W_SOCK_OK)
    {
        log(ERR,"W5500 connect err = %d\n" , rt);
        return rt;
    }
          
    return SOCKET_OK;
}

int8_t w5500_send( uint8_t socketId , uint8_t *sendData , uint16_t length )
{
    //log(DEBUG,"w5500 发送数据,id = %d , data = %s ,length = %d \n" , socketId , sendData , length);
    // while(getSn_TX_FSR(socketId)<length);
      //log_arry(DEBUG,"socket send" , sendData , length);
    eth_send( socketId , sendData , length);
    // sys_delay(30);
    
    return TRUE;
}

void w5500_close( void )
{
    log(DEBUG,"W5500 close \n");
    w5500InitStatus = FALSE;
    ethstatus=ETH_INIT;  
}

void w5500_run(void)
{
    int len=0;
    uint8_t sock_status=0 , sn_ir_type;

    if( w5500_line_connect() == 0 )
    {
        log_err("网线未连接 , 重启W5500\n");
                socket_clear_all();
                w5500_close();
    }
    
    for(uint8_t sock_sn=0;sock_sn<SOCKET_CONNECT_MAX;sock_sn++) 
    {
        if( sockeArry[sock_sn].useFlag == TRUE)
        {
            sock_status=getSn_SR(sock_sn);
            //log(DEBUG,"sn:%d sock_status:%2X\n",sock_sn,sock_status);
            switch(sock_status)
            {
                case SOCK_ESTABLISHED:
                {
                    sn_ir_type=getSn_IR(sock_sn);
                    //log(DEBUG,"sn_ir_type:%2X\n",sn_ir_type);
                    if( sn_ir_type& Sn_IR_CON)
                    {
                        setSn_IR(sock_sn, Sn_IR_CON);
                    }           
                    len=getSn_RX_RSR(sock_sn);
                    if(len>0)
                    {
                        int ret = 0;
                        //log(INFO,"w5500 port =%d , w5500 recv data len =%d\n" ,sock_sn , len); 
                        
                        if( (ret = eth_recv(sock_sn,(uint8_t *)sockeArry[sock_sn].msg,len))>0)
                        {
                             if(otasee)log_arry(DEBUG,"[W5500 recv]" , (uint8_t *)sockeArry[sock_sn].msg , len);
                                                  
                             //if(len<5) log_arry(ERR,"[W5500 recv]" , sockeArry[sock_sn].msg , len);
                            sockeArry[sock_sn].len =len; 
                            sockeArry[sock_sn].status=SOCKET_READ;
                            //if(sock_sn==1)
                            //{
                              //printf(" [%.*s]",len,sockeArry[sock_sn].msg);
                            //}
                        }
                        else
                        {
                            log(ERR,"w5500 recv err code=%d.\n" , ret);
                        }
                        //log(DEBUG,"W5500 recv = %s\r\n" , sockeArry[sock_sn].msg); 
                    } 
                }break;
                case SOCK_CLOSE_WAIT:
                {
                  
                  /*
                                  if(sock_sn == 1)
                                  {
                                  
                                    log(ERR,"w5500 OTA遇到麻烦\n");
                                    W5500ERR = 1;
                                  }
                                  //这是以前的  只有else
  
                                    log(DEBUG,"disconnect and closeing sock_sn:%d \n" , sock_sn);
                                    eth_disconnect(sock_sn);
                                    socket_clear_bind(sock_sn);
                                    //w_close(sock_sn);

              */
                  
                                  log(DEBUG,"disconnect and closeing sock_sn:%d \n" , sock_sn);
                                  if(sock_sn == 1)
                                  {
                                  
                                    log(ERR,"w5500 OTA遇到麻烦\n");
                                    W5500ERR = 1;
                                  }
                                  else
                                  {
                                      
                                    log(DEBUG,"disconnect and closeing sock_sn:%d \n" , sock_sn);
                                    eth_disconnect(sock_sn);
                                    socket_clear_bind(sock_sn);
                                    //w_close(sock_sn);
                                  }
                                 soft_system_resert(__FUNCTION__);
                }break;
                default:break; 
            }
        }
        sys_delay(10);
    }
}

void w5500_config( void )
{
    while( w5500_config_init() != TRUE )
    {
        log(WARN , "W5500 初始化配置失败\n");
        sys_delay(1000);
    }
}

static void w5500_task( void const *pvParameters)
{
    hdcpTimerHandle = timer.creat(1000 , TRUE , DHCP_time_handler);
    dnsTimerHandle = timer.creat(1000 , FALSE , DNS_time_handler);

    w5500_config();
    
    for(;;)
    {

      
        switch(ethstatus)
        {
            case ETH_INIT:
            {
                if( w5500_init() == TRUE )
                {
                    sys_delay(10);
                    ethstatus=ETH_RUN;
                }
                else
                {
                    sys_delay(100);
                }
            }break;
            case ETH_RUN:
            {
                w5500_run();
                sys_delay(10);
            }break;
            default:break; 
        }
        //read_task_stack(__func__,w5500Task);
    }
}

void creat_w5500_task( void )
{
    osThreadDef(ETnet, w5500_task, osPriorityHigh, 0, configMINIMAL_STACK_SIZE*15);
    w5500Task = osThreadCreate(osThread(ETnet), NULL);
    configASSERT(w5500Task);
}

devComType     eth=
{
    .init = creat_w5500_task,
    .isOK = w5500_isOK,
    .connect = w5500_connect,
    .disconnect = w5500_disconnect,
    .send = w5500_send,
    .close = w5500_close,
};
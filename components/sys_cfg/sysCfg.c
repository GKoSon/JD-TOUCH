#include "sysCfg.h"
#include "crc16.h"
#include "buletooth.h"
#include "spi_flash.h"
#include "unit.h"
#include "component.h"
#include "magnet.h"
#include "beep.h"
#include "open_log.h"
#include "permi_list.h"
#include "err_log.h"
#include "tempwd.h"
#include "iwdg.h"
#include "chipFlash.h"
#include "adc.h"
#include "socket.h"
#include "tagComponent.h"
#include "buletooth.h"
#include "md5.h"
#include "sha1.h"
#include "mqtt_client.h"
extern char topicPath[GMAX_MESSAGE_HANDLERS][52];

SystemConfigType   cfg;

_ble2http *bh = &cfg.ble2http;

void hal_read_chipId(unsigned char *p)
{
    #define CHIP_ID_START 0x1FFF7590
    unsigned char i;
    unsigned char *pIDStart=(unsigned char *)(CHIP_ID_START);   
    
    for(i=0;i!=12;i++)
    {
        *p++=*pIDStart++;
    }

}




uint8_t sys_cfg_read(SystemConfigType *data)
{
    return (chip_flash_read( DSYS_CFG_ADDR , (uint8_t *)data , sizeof(SystemConfigType) ));
}


void sys_cfg_clear( void )
{
    uint8_t cnt = 100,ack=0;
  
    if( chip_flash_get_lock() == TRUE )
    {
      
      while(1)
      {
         MX_NVIC_SetIRQ(DISABLE);
         ack = chip_flash_earse(DSYS_CFG_ADDR);
         MX_NVIC_SetIRQ(ENABLE); 
        if(ack == 0)
          break;
        sys_delay(500);
        if(--cnt==0)
          break;
      
      }
      if(cnt==0)
          log(ERR,"CFG�ڴ��ʽ��ʧ��r\n");
      else
          log(INFO,"CFG�ڴ��ʽ���ɹ�r\n");
      
    }
        
     chip_flash_release_lock();



}


void sys_cfg_write(SystemConfigType *cfg)
{
    uint8_t cnt = 100,ack=0;
    if(chip_flash_get_lock()== TRUE )
    {       
      while(1)
      {
         MX_NVIC_SetIRQ(DISABLE);
         ack = chip_flash_write( DSYS_CFG_ADDR , (uint8_t *)cfg , sizeof(SystemConfigType) ) ;
         MX_NVIC_SetIRQ(ENABLE); 
          if(ack == TRUE)
          break;
          sys_delay(500);
          if(--cnt==0)
          break;
      
      }
      if(cnt==0)
          log(ERR,"sys_cfg_writeʧ�� r\n");
      else
          log(ERR,"sys_cfg_write�ɹ�\r\n");

      chip_flash_release_lock(); 
    } 
    else
    log(ERR,"sys_cfg_write ERR\n"); 
}




void sysCfg_save( void )
{
    cfg.crc16 = 0;
   
    memset(fb , 0xFF , sizeof(fb));
    
    memcpy(fb , &cfg , sizeof(SystemConfigType));
    
    cfg.crc16 =  crc16_ccitt(fb , sizeof(fb));
        
    sys_cfg_write(&cfg);
}


void sysCfg_load( void )
{
    uint16_t crc , calcCrc;
    uint8_t cnt = 20;
    
    do
    {
        sys_cfg_read(&cfg);
        if((cfg.crc16 == 0xFFFF) && (cfg.mark == 0xFFFFFFFF))
        {
            log(WARN,"��ȡ����CRC��MARK����FF return\n");   
            return ;
        }
        crc = cfg.crc16;
        cfg.crc16 = 0;
        memset(fb , 0xFF , sizeof(fb));
        memcpy(fb , &cfg , sizeof(SystemConfigType));
        
        calcCrc =  crc16_ccitt(fb , sizeof(fb));
        if(calcCrc != crc )
        {
            log(ERR,"��ȡϵͳ�����ļ�,�����CRC = %x, ��ȡ����CRC = %x \n" , calcCrc , crc);
        }
    }while((calcCrc != crc)&& (--cnt) );

    if( cnt ==0 )
    {
        log_err("�����ļ����޸� ,���¼�������δ��\n");

        sysCfg_save();
        
        return ;
    }

    return ;
}



void sys_info_read_device_module( void )
{
    uint8_t config=0 , config1 = 0 , config2 = 0;
    uint32_t moduleValue = 0;
    
    pin_ops.pin_mode(NET_CONFIG_PIN1 , PIN_MODE_INPUT);
    pin_ops.pin_mode(NET_CONFIG_PIN2 , PIN_MODE_INPUT);
    
    config1 = pin_ops.pin_read(NET_CONFIG_PIN1);
    config2 = pin_ops.pin_read(NET_CONFIG_PIN2);
    
    config = (config1<<1|config2)&0x03;
    
    
    if( config ==0)
    {
        log(DEBUG,"GPRS����ģʽ\n");	
        cfg.parm.support_net_types = TSLNetType_TSLGPRS;    
    }
 
    if( config ==1)
    {
        log(DEBUG,"WIFI����ģʽ\n");
        cfg.parm.support_net_types = TSLNetType_TSLWIFI;
    }
    if( config ==2)
    {
        log(DEBUG,"��̫����ģʽ\n");
        cfg.parm.support_net_types = TSLNetType_TSLEthernet;
    }

      moduleValue = read_choose_module_value();

      if(  ( moduleValue > 50) && ( moduleValue < 80))
      {
        cfg.parm.support_ble_types = 0;//BM77            
      }
      else if(( moduleValue >= 80) && ( moduleValue < 115))
      {
        cfg.parm.support_ble_types = 1;//0906
      }
      else if(( moduleValue >= 140) && ( moduleValue < 180))
      {
        cfg.parm.support_ble_types = 0;    
      }
      else if(( moduleValue >= 200) && ( moduleValue < 240))
      {
        cfg.parm.support_ble_types = 1;
      }
      else
      {
        log(ERR,"�豸BLEģʽADC�ɼ�����Լ����Χ Ĭ����BM77\n");
        cfg.parm.support_ble_types = 0;
      }
//cfg.parm.support_ble_types = 0;
      log(ERR,"�豸BLEģʽ=%d .[0--BM77  1---0906]\n" ,  cfg.parm.support_ble_types);     
}


      
void sysCfg_set_ble_default( void )
{
    cfg.ble.iBeacon_rssi = 0x28;

    memset(cfg.ble.ble_admin_num , 0x00 , BLE_ADMIN_LENGTH);
    
    memset(cfg.ble.ble_mac , 0xFE , BLE_MAC_LENGTH);

    cfg.ble.ble_version = 0;
  
    btModule.resert();

    sys_delay(300);

    log(ERR,"[SYS]׼��BLE��ȡMAC��ַ\n");
    if( btModule.read_mac(cfg.ble.ble_mac) == BLE_OK)
    {

        cfg.ble.ble_version = btModule.read_version();
        
        btModule.set_default();
    }
    else
    {
        log(DEBUG,"[SYS]BLE��ȡMAC��ַʧ��\n");
    }

}



void devip_set_default(void)
{
      cfg.devIp.dhcpFlag = TRUE;
}



void mqtt_set_default( void )
{

      memset(&cfg.mqtt, 0x00 ,sizeof(MqttLoginInfoType));

      memcpy(cfg.mqtt.mqttUserName , "shhx" ,strlen("shhx"));
      memcpy(cfg.mqtt.mqttUserPwd  , "7ed98d7018caf009c60008521274ceb4" ,strlen("7ed98d7018caf009c60008521274ceb4"));
      sprintf(cfg.mqtt.mqttClientId,"%032s",(char *)cfg.parm.deviceName);

}


void sysCfg_set_default( void )
{
      SHOWME  SHOWME  SHOWME
      memset(&cfg,0,sizeof(SystemConfigType));
      uint8_t DeafultPwd[BLE_PASSWORD_LENGTH] ={0X12,0x34,0x56};

      cfg.mark = DEFAULT_MARK;
      cfg.parm.deviceNum = DEVICE_NUM;
      cfg.parm.lock_mode = DEVICD_MODE;   
      cfg.parm.device_type = DEVICE_TYPE;
      cfg.parm.delay_time = OPEN_DELAY;

      ///////////INFO
      //����                
      config.write(CFG_USER_PWD ,DeafultPwd ,FALSE);
      config.write(CFG_PAIR_PWD ,DeafultPwd ,FALSE);

      memset(&cfg.commun , 0x00 , sizeof(SystemCommunitiesType));
      sys_info_read_device_module();  //��ò��� һ�� ����

      bluetooth_drv_init();        //��ʼ������ģ��  һ������

      tag_component_init();        //��ʼ��NFCоƬ   д��

      socket_compon_init();       //��ʼ��SOCKET    ��������

      sysCfg_set_ble_default();  //���MAC

      ///////////INFO

      cfg.parm.alarm_time = 3;/*Ĭ��3min�Ŵų�ʱ*/

      cfg.parm.magnet_status = DEFAULT_MAGNET_STATUS;//PIN_HIGH ����ʱ���ָ�
      cfg.parm.updataTime = FALSE;
      hal_read_chipId(cfg.parm.chipId);

      memset(cfg.wifi.ssid , 0x00 , 50);
      memcpy(cfg.wifi.ssid , "TERMINUS" , strlen("TERMINUS"));

      memset(cfg.wifi.pwd , 0x00 , 32);
      memcpy(cfg.wifi.pwd , "WXtsl66117066" , strlen("WXtsl66117066"));


      //OTA    ȫ0
      cfg.parm.soft_version = DEVICE_SW_VERSION;

      //�ƶ���ŵ  
      memset( &cfg.server , 0x00 , sizeof(netAttriType));
      memcpy(cfg.server.net.ip , NET_IP , strlen(NET_IP));
      cfg.server.net.port = MQTT_PORT;
      cfg.server.httpport = HTTP_PORT;
      cfg.server.otaport =  OTA_PORT;
      
      //������ŵ
//devip_set_default();

      //����MAC�ı�ʶ
      sprintf((char *)cfg.parm.deviceName , "%.4s%02X%02X%02X%02X%02X%02X", DEVICE_NAME ,
      cfg.ble.ble_mac[0],cfg.ble.ble_mac[1],cfg.ble.ble_mac[2],
      cfg.ble.ble_mac[3],cfg.ble.ble_mac[4],cfg.ble.ble_mac[5]);
      
      mqtt_set_default();

      HAL_IWDG_Refresh(&hiwdg);
      
//      magnet_input_status_init(0);/*0--��ʶ���ڲ�д���ڴ� 1--����д���ڴ�*/
/*���������� ������װ�Ǻõ� ��������Ŵűպ���ȥ��װ�� �Ǿͷ��� ���԰�װ��ʱ�� �Ŵ�һ����Ҫ����*/
/*û���� �Ѿ�����cfg.parm.magnet_status = DEFAULT_MAGNET_STATUS;*/
      permi.clear();

      journal.clear();

      //tempwd.clear();
    
      SHOWME  SHOWME  SHOWME
}
void showsyslocation(SystemCommunitiesType *p)
{
    log(ERR,"super_code = %d\n" ,  p->super_code);
    for(char i=0;i<5;i++)
    {
         printf("[%d]village_id  = %08X\t" ,  i,p->communities[i].village_id);
         printf("[%d]building_id = %08X\t" ,  i,p->communities[i].building_id);
         printf("[%d]floor_id    = %08X\t" ,  i,p->communities[i].floor_id);
         printf("[%d]house_id    = %08X\t" ,  i,p->communities[i].house_id);
         printf("\r\n");
    }
}

void sysCfg_print( void )
{
      log(DEBUG,"\n");
      log(INFO,"********************* 20190810 Copyright by terminus hw team ********************* \n");
      log(DEBUG,"ϵͳ����ʱ��: %s %s .\r\n" ,__DATE__,__TIME__);
      log(DEBUG,"�����Ž�CRC16 = %X cfgsize=%d\n",cfg.crc16,sizeof(SystemConfigType));
      log(DEBUG,"�豸��� = [%d]\n",cfg.parm.deviceNum);
      log(DEBUG,"�豸���� = [%d]\n",cfg.parm.device_type);
      log(DEBUG,"�豸ˮƽ = [%d]\n",cfg.level);
      log(ERR,"MQTTDONE = [%d]\n",cfg.parm.mqttdone);
      log(ERR,"�豸ģʽ =[%d][��װλ�� 0:�ſڻ���Ԫ�Ž� ,1:¥���Ž� ,2: Χǽ��С���Ž�]\n" ,cfg.parm.lock_mode);
      log(DEBUG,"����汾: [%d]\n" , cfg.parm.soft_version);

      log(DEBUG,"delay_time: [%d]\n" , cfg.parm.delay_time);
      log(DEBUG,"alarm_time: [%d]\n" , cfg.parm.alarm_time);
      log(ERR,"�Ŵ�������ƽ: [%d]\n" , cfg.parm.magnet_status);
      log_arry(DEBUG,"�豸ΨһID" ,cfg.parm.chipId , DEVICE_CHIP_LENGTH);
      log(DEBUG,"�豸�������� = [%d] [0:BM77 ,1:0906]\n" ,cfg.parm.support_ble_types);
      log(DEBUG,"�豸�������� = %s \n" ,cfg.parm.deviceName);
      log(DEBUG,"����ģ��汾�� [%d] \n" , cfg.ble.ble_version );
      log_arry(DEBUG,"����ģ���MAC��ַ "  ,cfg.ble.ble_mac ,BLE_MAC_LENGTH);
      log(DEBUG,"�豸�������� = [%d] [1:GPRS,2:WFI,4ETH]\n" ,cfg.parm.support_net_types);
      log(ERR,"�Ƿ�DHCP = [%d] \n" ,cfg.devIp.dhcpFlag);
      printf("SIP: %d.%d.%d.%d\n", cfg.devIp.ip[0],cfg.devIp.ip[1],cfg.devIp.ip[2],cfg.devIp.ip[3]);
      printf("GAR: %d.%d.%d.%d\n", cfg.devIp.gateway[0],cfg.devIp.gateway[1],cfg.devIp.gateway[2],cfg.devIp.gateway[3]);
      printf("SUB: %d.%d.%d.%d\n", cfg.devIp.mark[0],cfg.devIp.mark[1],cfg.devIp.mark[2],cfg.devIp.mark[3]);
      printf("DNS: %d.%d.%d.%d\n", cfg.devIp.dns[0],cfg.devIp.dns[1],cfg.devIp.dns[2],cfg.devIp.dns[3]);
      log(DEBUG,"MQTTҵ�������IP��ַ: %s:%d\n" , cfg.server.net.ip , cfg.server.net.port);
      log(DEBUG,"HTTPҵ�������IP��ַ: %s:%d\n" , cfg.server.net.ip , cfg.server.httpport);
      log(DEBUG,"OTA ҵ�������IP��ַ: %s:%d\n" , cfg.server.net.ip,  cfg.server.otaport);
      log(DEBUG,"mqtt client    = %s\n" ,    cfg.mqtt.mqttClientId);
      log(DEBUG,"mqtt user name = %s\n" ,    cfg.mqtt.mqttUserName);
      log(DEBUG,"mqtt user pwd  = %s\n" ,    cfg.mqtt.mqttUserPwd);
      showsyslocation(&cfg.commun);

      //tempwd.show();
      permi.show();
      log(INFO,"************************************ end  ************************************ \n");
      log(INFO,"************************************ end  ************************************ \n");
}

uint8_t cfg_write ( uint8_t mode , void *parma , uint8_t earseFlag)
{
    uint8_t status = true;
    
    switch(mode)
    {
      
         case CFG_DEV_USED:
        {
            cfg.parm.mqttdone = *(uint8_t *)(parma);
        }break;

        case CFG_SYS_SW_VERSION:
        {
            cfg.parm.soft_version = *(uint16_t *)(parma);
        }break;

         case CFG_SYS_OTA_VERSION:
        {
            cfg.otaVar.ver = *(uint32_t *)(parma);
        }break;       
        case CFG_PAIR_PWD:
        {
            memcpy(cfg.pair_pwd , parma , BLE_PASSWORD_LENGTH);
            
        }break;
        case CFG_USER_PWD:
        {
            memcpy(cfg.user_pwd , parma , BLE_PASSWORD_LENGTH);
            
        }break;

        case CFG_BLE_ADMIN:
        {
            memcpy(cfg.ble.ble_admin_num , parma , BLE_ADMIN_LENGTH);
        }break;

        case CFG_BLE_MAC:
        {
            memcpy(cfg.ble.ble_mac , parma , BLE_MAC_LENGTH);
        }break;
        case CFG_BLE_VERSION:
        {
            cfg.ble.ble_version = *(uint32_t *)(parma);
        }break;
        case CFG_BLE_RSSI:
        {
            cfg.ble.iBeacon_rssi = *(uint8_t *)(parma);
        }break;

        case CFG_SYS_OPEN_TIME:
        {
            cfg.parm.delay_time = *(uint8_t *)(parma);
            log(DEBUG,"���ÿ�����ʱ = %d \n" , cfg.parm.delay_time*100);
        }break;
      
        case CFG_SYS_ALARM_TIME:
        {
            cfg.parm.alarm_time = *(uint8_t *)(parma);
            log(DEBUG,"�Ŵű���ʱ�� = %d \n" , cfg.parm.alarm_time);
        }break;
        case CFG_SYS_LOCK_MODE:
        {
            cfg.parm.lock_mode = *(uint8_t *)(parma);
            log(DEBUG,"�����豸���� = %d \n" , cfg.parm.lock_mode);
        }break;
        case CFG_SYS_COMMUN_CODE:
        {
            memcpy(&cfg.commun , parma , sizeof(SystemCommunitiesType));
            
        }break;   
        case CFG_SYS_COMMUN_ONE_CODE:
        {
            memcpy(&cfg.commun.communities[0] , parma , sizeof(TSLCommunityModel));
           
        }break;
        
        case CFG_SYS_SUPER_CODE:
        {
            cfg.commun.super_code = *(uint32_t *)(parma);
            
        }break;
        case CFG_OTA_PORT:
        {
             cfg.server.otaport = *(uint16_t *)(parma);
             log(INFO,"OTA PORT = %d \n" ,cfg.server.otaport);
        }break;

        
        case CFG_SYS_MAGNET_STATUS:
        {
             cfg.parm.magnet_status = *(uint8_t *)(parma);
             log(INFO,"�����Ŵ������ʼ��״̬STATUS = %d \n" , cfg.parm.magnet_status);
        }break;

        case CFG_NET_ADDR:
        {
             memcpy(&cfg.server.net , parma , sizeof(serverAddrType));
        }break;


        case CFG_OTA_URL:
        {
             memset(&cfg.server.otaurl,0,64);
             memcpy(&cfg.server.otaurl , parma , GMIN(64,strlen((char*)parma)));
        }break;
        case CFG_OTA_CONFIG:
        {
             memcpy(&cfg.otaVar , parma , sizeof(otaType));
        }break;
          
        case CFG_SET_RESTORE:
        {/*����ڲ��ļ� ������ ����*/
            sys_cfg_clear();
            soft_system_resert(__func__);
        }break;
        case CFG_WIFI_INFO:
        {
            memcpy(&cfg.wifi , parma , sizeof(wifiApInfoType));
        }break;
        case CFG_IP_INFO:
        {
            memcpy(&cfg.devIp , parma , sizeof(DeviceIpType));
        }break;
        case CFG_SYS_UPDATA_TIME:
        {
            cfg.parm.updataTime = *(uint8_t *)(parma);
            log(DEBUG,"�豸ͬ��ʱ��״̬=%d\n" , cfg.parm.updataTime);
        }break;
         case CFG_CLEAR_RESTORE_FLAG:
        {
            uint32_t bit = *(uint32_t *)(parma);
            cfg.sysRestoreFlag &= ~bit;
        }break;
        case CFG_SET_RESTORE_FLAG:
        {
            uint32_t bit = *(uint32_t *)(parma);
            cfg.sysRestoreFlag |= bit;
        }break;  
        case CFG_DEV_LEVEL:
        {
            cfg.level = *(uint8_t *)(parma);
        }break;
        case CFG_NOTHING:
        default:
        {
            log(INFO,"Set message is no choose mode:%d\r\n",mode);
            status = false;
        }break;
    }
  
    if(earseFlag)
    {
        log(INFO,"earseFlag 1\r\n");
        sysCfg_save();
    }
    
    return status;
}

uint32_t cfg_read ( uint8_t mode , void **parma )
{
    uint32_t data = 0;

    switch(mode)
    {
        case CFG_DEV_USED:
        {
            data = cfg.parm.mqttdone;
        }break;

         case CFG_OTA_PORT:
        {
            data = cfg.server.otaport;
        }break;
        case CFG_PAIR_PWD:
        {
            *parma = cfg.pair_pwd;
            data = BLE_PASSWORD_LENGTH;
        }break;
        case CFG_USER_PWD:
        {
           *parma = cfg.user_pwd;
            data = BLE_PASSWORD_LENGTH;
        }break;
        case CFG_BLE_ADMIN:
        {
            *parma = cfg.ble.ble_admin_num;
        }break;
        case CFG_BLE_MAC:
        {
            *parma = cfg.ble.ble_mac;
        }break;
              
        case CFG_BLE_RSSI:
        {
            data = cfg.ble.iBeacon_rssi;
        }break;

        case CFG_BLE_VERSION:
        {
            data = cfg.ble.ble_version;
        }break;

        case CFG_SYS_OPEN_TIME:
        {
            data = cfg.parm.delay_time;
        }break;
        
        case CFG_SYS_CHIP_ID:
        {
            *parma = &cfg.parm.chipId;
        }break;
        case CFG_SYS_DEVICE_TYPE:
        {
            data = cfg.parm.device_type;
        }break;
        case CFG_SYS_DEVICE_NUM:
        {
            data = cfg.parm.deviceNum;
        }break;
        
        case CFG_SYS_DEVICE_NAME:
    {
            *parma = cfg.parm.deviceName;
    }break;

        case CFG_SYS_SW_VERSION:
        {
            data = cfg.parm.soft_version;
        }break;
        case CFG_SYS_NET_TYPE:
        {
            data = cfg.parm.support_net_types;
        }break;

        case CFG_SYS_BLE_TYPE:
        {
            data = cfg.parm.support_ble_types;
        }break;
        case CFG_SYS_COMMUN_CODE:
        {
            *parma = &cfg.commun;
        }break;

        case CFG_SYS_COMMUN_ONE_CODE:
        {
            *parma = &cfg.commun.communities[0];
        }break;
        case CFG_SYS_ALARM_TIME:
        {
            data = cfg.parm.alarm_time;
        }break;
        case CFG_SYS_LOCK_MODE:
        {
            data = cfg.parm.lock_mode;
        }break;
        case CFG_SYS_SUPER_CODE:
        {
            *parma = &cfg.commun.super_code;
        }break;
 
        case CFG_SYS_MAGNET_STATUS:
        {
             data = cfg.parm.magnet_status;
        }break;
        case CFG_NET_ADDR:
        {
             *parma = &cfg.server.net;

        }break;


         case CFG_OTA_URL:
        {
              *parma = &cfg.server.otaurl;

        }break;
         case CFG_HTTP_ADDR:
        {
              *parma =  &cfg.server.net;
              data = cfg.server.httpport;
        }break;
        case CFG_OTA_CONFIG:
        {
                *parma = &cfg.otaVar;
        }break;

        case CFG_SYS_UPDATA_TIME:
        {
            data = cfg.parm.updataTime;
        }break;
        
        case CFG_WIFI_INFO:
        {
            *parma = &cfg.wifi;
        }break;

        case CFG_MQTT_CLIENTID:
        {
            *parma = &cfg.mqtt.mqttClientId;
        }break;
        case CFG_MQTT_USERNAME:
        {
            *parma = &cfg.mqtt.mqttUserName;
        }break;
        case CFG_MQTT_USERPWD:
        {
            *parma = &cfg.mqtt.mqttUserPwd;
        }break;
     
        case CFG_MQTT_MAC:
        {      
            *parma = &cfg.parm.deviceName[4];
        }break;

        case CFG_PRO_PWD:
        {
            static uint8_t proj_pwd[7];
            memset(proj_pwd , 0     ,     7);
            G_1byteTo2str(proj_pwd,cfg.pair_pwd,3);
            *parma = proj_pwd;
        }break;
        
        case CFG_IP_INFO:
        {
            *parma = &cfg.devIp;
        }break;
        
             
        case CFG_SET_RESTORE_FLAG:
        {
            data = cfg.sysRestoreFlag;
        }break;
        case CFG_CLEAR_RESTORE_FLAG:
        {
            data = cfg.sysRestoreFlag;
        }break;
        case CFG_DEV_LEVEL:
        {
            data = cfg.level;
        }break;
        default:
        {
            log_err("Get message is no choose mode:%d\r\n",mode);
            parma = NULL;
        }break;
    } 

    return data;
}


cfgTaskType    config=
{
    .write = cfg_write,
    .read = cfg_read,
};



void sysCfg_init( void )
{

    sysCfg_load();
    
    if( cfg.mark != DEFAULT_MARK)
    {
        log(INFO,"Device is't init , init system cfg.\r\n");
        log_arry(DEBUG,"Mark is error" , (uint8_t *)&cfg , sizeof(SystemConfigType));
        sysCfg_set_default();
        sysCfg_save();
        soft_system_resert(__func__);
    }
    
    //HERE
      log(INFO,"topicPath[0] %s\r\n",topicPath[0]);
      for(char i=1;i<GMAX_MESSAGE_HANDLERS;i++)
      {
        strcat(topicPath[i],(char *)&(cfg.parm.deviceName[4]));
        log(INFO,"topicPath[%d] %s\r\n",i,topicPath[i]);
      }
      

    sysCfg_print();

}




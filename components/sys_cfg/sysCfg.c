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
#include "iwdg.h"
#include "chipFlash.h"
#include "adc.h"
#include "socket.h"
#include "tagComponent.h"
#include "buletooth.h"
#include "md5.h"
#include "sha1.h"
#include "mqtt_client.h"
extern SystemConfigType   cfg;
extern _SHType SHType;

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
          log(ERR,"CFG内存格式化失败r\n");
      else
          log(INFO,"CFG内存格式化成功r\n");
      
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
          sys_delay(50);
          if(--cnt==0)
          break;
      
      }
      if(cnt==0)
          log(ERR,"[SYS]sys_cfg_write失败 r\n");
      else
          log(ERR,"[SYS]sys_cfg_write成功\r\n");

      chip_flash_release_lock(); 
    } 
    else
    log(ERR,"[SYS]sys_cfg_write ERR\n"); 
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
            log(WARN,"读取到的CRC和MARK都是FF return\n");   
            return ;
        }
        crc = cfg.crc16;
        cfg.crc16 = 0;
        memset(fb , 0xFF , sizeof(fb));
        memcpy(fb , &cfg , sizeof(SystemConfigType));
        
        calcCrc =  crc16_ccitt(fb , sizeof(fb));
        if(calcCrc != crc )
        {
            log(ERR,"读取系统配置文件,计算的CRC = %x, 读取到的CRC = %x \n" , calcCrc , crc);
        }
    }while((calcCrc != crc)&& (--cnt) );

    if( cnt ==0 )
    {
        log_err("配置文件被修改 ,重新加载配置未见\n");

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
        log(DEBUG,"GPRS联网模式\n");	
        cfg.parm.support_net_types = TSLNetType_TSLGPRS;    
    }
 
    if( config ==1)
    {
        log(DEBUG,"WIFI联网模式\n");
        cfg.parm.support_net_types = TSLNetType_TSLWIFI;
    }
    if( config ==2)
    {
        log(DEBUG,"以太联网模式\n");
        cfg.parm.support_net_types = TSLNetType_TSLEthernet;
    }

      moduleValue = read_choose_module_value();

      if(  ( moduleValue > 50) && ( moduleValue < 80))
      {
        cfg.parm.support_ble_types = 0;        
      }
      else if(( moduleValue >= 80) && ( moduleValue < 115))
      {
        cfg.parm.support_ble_types = 1;
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
        log(ERR,"[SYS]设备BLE模式ADC采集不在约定范围 默认是0906\n");
        cfg.parm.support_ble_types = 1;
      }

      log(ERR,"[SYS]设备BLE模式=%d .(0--BM77  1--0906)\n" ,  cfg.parm.support_ble_types);     
}

void sysCfg_set_ble_default( void )
{

    memset(cfg.ble.ble_mac , 0xFE , BLE_MAC_LENGTH);

    cfg.ble.ble_version = 0;
  
    btModule.resert();

    sys_delay(300);

    log(ERR,"[SYS]准备BLE读取MAC地址\n");
    if( btModule.read_mac(cfg.ble.ble_mac) == BLE_OK)
    {

        cfg.ble.ble_version = btModule.read_version();
        
        btModule.set_default();
    }
    else
    {
        log(DEBUG,"[SYS]BLE读取MAC地址失败\n");
    }

}

void mqtt_set_default( void )
{
      memset(&cfg.mqtt, 0x00 ,sizeof(MqttLoginInfoType));

      //memcpy(cfg.mqtt.mqttUserName , "dark" ,strlen("dark"));
      //memcpy(cfg.mqtt.mqttUserPwd  , "48e8a059e523b9550ac37665ea088cdb" ,strlen("48e8a059e523b9550ac37665ea088cdb"));
      memcpy(cfg.mqtt.mqttUserName , "device" ,strlen("device"));/*一开始使用这个账户  登入MQTT马上被踢出来 MQTT返回5 是因为名字不对mqttUserName装不下 后面是乱码 */
      memcpy(cfg.mqtt.mqttUserPwd  , "48e8a059e523b9550ac37665ea088cdb" ,strlen("48e8a059e523b9550ac37665ea088cdb"));
      sprintf(cfg.mqtt.mqttClientId,"%032s",(char *)cfg.parm.deviceName);
}


void sysCfg_set_default( void )
{
      SHOWME  SHOWME  SHOWME
      memset(&cfg,0,sizeof(SystemConfigType));
      uint8_t DeafultPwd[BLE_PASSWORD_LENGTH] ={0X12,0x34,0x56};

      cfg.mark = DEFAULT_MARK;

      cfg.parm.lock_mode = DEVICD_MODE;   

      cfg.parm.delay_time = OPEN_DELAY;
        
      config.write(CFG_USER_PWD ,DeafultPwd ,FALSE);
      config.write(CFG_PAIR_PWD ,DeafultPwd ,FALSE);


      sys_info_read_device_module();  //获得参数 一个 三个

      bluetooth_drv_init();        //初始化蓝牙模块  一个参数

      tag_component_init();        //初始化NFC芯片   写死

      socket_compon_init();       //初始化SOCKET    三个参数

      sysCfg_set_ble_default();  //获得MAC

      cfg.parm.alarm_time = 3;/*默认3min门磁超时*/

      cfg.parm.magnet_status = DEFAULT_MAGNET_STATUS;//PIN_HIGH 开门时表现高
      cfg.parm.updataTime = FALSE;
      hal_read_chipId(cfg.parm.chipId);

//ESP32
memset(cfg.wifi.ssid , 0x00 , 50);
memcpy(cfg.wifi.ssid , "mCube-Xsens-Employee" , strlen("mCube-Xsens-Employee"));

memset(cfg.wifi.pwd , 0x00 , 32);
memcpy(cfg.wifi.pwd , "Kiss2017" , strlen("Kiss2017"));
uint32_t restoreBit = SYS_NET_RESTORE_BIT;
config.write(CFG_SET_RESTORE_FLAG , &restoreBit ,TRUE);


      cfg.parm.soft_version = DEVICE_SW_VERSION;

 
      memset( &cfg.server , 0x00 , sizeof(netAttriType));
      memcpy(cfg.server.net.ip , NET_IP , strlen(NET_IP));
      cfg.server.net.port = MQTT_PORT;


      sprintf((char *)cfg.parm.deviceName , "%.4s%02X%02X%02X%02X%02X%02X", DEVICE_NAME ,
      cfg.ble.ble_mac[0],cfg.ble.ble_mac[1],cfg.ble.ble_mac[2],
      cfg.ble.ble_mac[3],cfg.ble.ble_mac[4],cfg.ble.ble_mac[5]);
      
      mqtt_set_default();

      HAL_IWDG_Refresh(&hiwdg);
      
//      magnet_input_status_init(0);/*0--标识现在不写到内存 1--立刻写入内存*/
/*这里有问题 正常安装是好的 如果正好门磁闭合你去安装呢 那就反了 所以安装的时候 门磁一定不要吸合*/
/*没意义 已经有了cfg.parm.magnet_status = DEFAULT_MAGNET_STATUS;*/
      permi.clear();

      journal.clear();

    
      SHOWME  SHOWME  SHOWME
}

void ShowIp(serverAddrType *p)
{
  printf("\r\n********************* ip   %s *******************\r\n",p->ip);
  printf("********************* port %d  *******************\r\n",p->port);
}

void show_OTA(void)
{
    log(INFO,"\n********************* ******show_OTA******  ********************* \n");

    printf("**************otaUpgMark:%d\n",cfg.otaVar.otaUpgMark);
    printf("*********************ver:%d\n",cfg.otaVar.ver);
    printf("****************fileSize:%d\n",cfg.otaVar.fileSize);
    printf("*******************crc32:0X%04X\n",cfg.otaVar.crc32);
    
    printf("******************otaurl:%s\n",cfg.server.otaurl);
    
    
    printf("******************otanet:%d\n",cfg.server.otanet.port);
    printf("******************otanet:%s\n",cfg.server.otanet.ip);

    log(INFO,"\n********************* ******show_OTA******  ********************* \n");
}

void show_SH(_SHType *p)
{
    
    char num=0;

    log(INFO,"\n********************* ******show_SH******  ********************* \n");

    printf("******************deviceCode:%s\n",p->deviceCode);
    printf("*********************gup.md5:%d\n",p->gup.md5);
    printf("*********************gup.cnt:%d\n",p->gup.cnt);
    printf("*********************gup.code\n");
    num = MIN(GUPMAX,p->gup.cnt);
    for(char i=0;i<num;i++)
    for(char j=0;j<11;j++)
    {
        printf("%02X ",p->gup.code[i][j]);
        if(j==10) printf("\n");
    }
   log(INFO,"\n********************* ******show_SH******  ********************* \n");

}
void sysCfg_print( void )
{
      log(DEBUG,"\n");
      log(INFO,"********************* 2021 Copyright ********************* \n");
      log(DEBUG,"系统编译时间: %s %s .\r\n" ,__DATE__,__TIME__);
      log(DEBUG,"门禁配置文件CRC16 = %X SIZE=%d\n",cfg.crc16,sizeof(SystemConfigType));    
      log(ERR,"设备模式 =[%d][0:单元机  1：围墙机 2：多围墙]\n" ,cfg.parm.lock_mode);
      log(DEBUG,"软件版本: [%d]\n" , cfg.parm.soft_version);
      log(DEBUG,"delay_time: [%d]\n" , cfg.parm.delay_time);
      log(DEBUG,"alarm_time: [%d]\n" , cfg.parm.alarm_time);
      log(ERR,"门磁正常电平: [%d]\n" , cfg.parm.magnet_status);
      log_arry(DEBUG,"设备唯一ID" ,cfg.parm.chipId , DEVICE_CHIP_LENGTH);
      log(DEBUG,"设备蓝牙类型 = [%d] [0:BM77 ,1:0906]\n" ,cfg.parm.support_ble_types);
      log(DEBUG,"设备蓝牙名称 = %s \n" ,cfg.parm.deviceName);
      log(DEBUG,"蓝牙模块版本号 [%d] \n" , cfg.ble.ble_version );
      log_arry(DEBUG,"蓝牙模块的MAC地址 "  ,cfg.ble.ble_mac ,BLE_MAC_LENGTH);   
      log_arry(DEBUG,"配对密码 "  ,cfg.pair_pwd,BLE_PASSWORD_LENGTH);
      log(DEBUG,"设备网络类型 = [%d] [1:GPRS,2:WFI,4ETH]\n" ,cfg.parm.support_net_types);
      log(DEBUG,"MQTT业务服务器IP地址: %s:%d\n" , cfg.server.net.ip ,    cfg.server.net.port);
      log(DEBUG,"OTA 业务服务器IP地址: %s:%d\n" , cfg.server.otanet.ip,  cfg.server.otanet.port);
      log(DEBUG,"mqtt client    = %s\n" ,    cfg.mqtt.mqttClientId);
      log(DEBUG,"mqtt user name = %s\n" ,    cfg.mqtt.mqttUserName);
      log(DEBUG,"mqtt user pwd  = %s\n" ,    cfg.mqtt.mqttUserPwd);
      log(ERR,"设备是否拉过黑白名单 = [%d]\n",cfg.parm.filterSynced);
     
      permi.show();//展示一下黑白名单
 //permi_list_init();
//permi.show();//展示一下黑白名单
      log(INFO,"************************************ end  ************************************ \n");
      log(INFO,"************************************ end  ************************************ \n");
}

uint8_t cfg_write ( uint8_t mode , void *parma , uint8_t earseFlag)
{
    uint8_t status = true;
    
    switch(mode)
    {
      
         case CFG_SYS_DEVICE_NAME:
        {
            memcpy(cfg.parm.deviceName,parma,4);/*仅仅修改前面4个*/  
        }break;
        
         case CFG_MQTT_DC:
        {      
            memcpy(SHType.deviceCode,parma,21);

            if(chip_flash_write( DSYS_DIANMA_ADDR , (uint8_t *)&SHType, sizeof(_SHType)))
            {
              log(DEBUG,"\r\n*******CFG_SYS_SHANGHAI WRITE OK*********\r\n");
              return true;
            }log(DEBUG,"\r\n*******CFG_SYS_SHANGHAI WRITE FAIL*********\r\n");

        }break;
                    
        case MQTT_FILTER_SYNCED:
        {
            cfg.parm.filterSynced = *(uint8_t *)(parma);
        }break;

        case CFG_SYS_SW_VERSION:
        {
            cfg.parm.soft_version = *(uint32_t *)(parma);
        }break;

      
        case CFG_PAIR_PWD:
        {
            memcpy(cfg.pair_pwd , parma , BLE_PASSWORD_LENGTH);
            
        }break;
        case CFG_USER_PWD:
        {
            memcpy(cfg.user_pwd , parma , BLE_PASSWORD_LENGTH);
            
        }break;

        case CFG_BLE_MAC:
        {
            memcpy(cfg.ble.ble_mac , parma , BLE_MAC_LENGTH);
        }break;
        
        case CFG_BLE_VERSION:
        {
            cfg.ble.ble_version = *(uint32_t *)(parma);
        }break;

        case CFG_SYS_OPEN_TIME:
        {
            cfg.parm.delay_time = *(uint8_t *)(parma);
            log(DEBUG,"设置开门延时 = %d \n" , cfg.parm.delay_time*100);
        }break;
      
        case CFG_SYS_ALARM_TIME:
        {
            cfg.parm.alarm_time = *(uint8_t *)(parma);
            log(DEBUG,"门磁报警时间 = %d \n" , cfg.parm.alarm_time);
        }break;
        case CFG_SYS_LOCK_MODE:
        {
            cfg.parm.lock_mode = *(uint8_t *)(parma);
            log(DEBUG,"设置设备类型 = %d \n" , cfg.parm.lock_mode);
        }break;
        
        case CFG_SYS_MAGNET_STATUS:
        {
             cfg.parm.magnet_status = *(uint8_t *)(parma);
             log(INFO,"设置门磁输入初始化状态STATUS = %d \n" , cfg.parm.magnet_status);
        }break;

        case CFG_NET_ADDR:
        {
             memset(&cfg.server.net,0,sizeof(serverAddrType));
             memcpy(&cfg.server.net , parma , sizeof(serverAddrType));
        }break;

        case CFG_OTA_ADDR:
        {
             memset(&cfg.server.otanet,0,sizeof(serverAddrType));
             memcpy(&cfg.server.otanet , parma , sizeof(serverAddrType));
        }break;
        case CFG_OTA_URL:
        {
             memset(&cfg.server.otaurl,0,64);
             memcpy(&cfg.server.otaurl , parma , MIN(64,strlen((char*)parma)));
             log(DEBUG,"CFG_OTA_URL[%s]\r\n",cfg.server.otaurl);
        }break;
        case CFG_OTA_CONFIG:
        {
             memcpy(&cfg.otaVar , parma , sizeof(otaType));
        }break;
          
        case CFG_SET_RESTORE:
        {/*清空内部文件 在重启 凶猛*/
            sys_cfg_clear();
            chip_flash_earse( DSYS_DIANMA_ADDR );
            soft_system_resert(__func__);
        }break;
        case CFG_WIFI_INFO:
        {
            memcpy(&cfg.wifi , parma , sizeof(wifiApInfoType));
        }break;

        case CFG_SYS_UPDATA_TIME:
        {
            cfg.parm.updataTime = *(uint8_t *)(parma);
            log(DEBUG,"设备同步时间状态=%d\n" , cfg.parm.updataTime);
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
        
        case CFG_SYS_SHANGHAI:
        {
          if(chip_flash_write( DSYS_DIANMA_ADDR , (uint8_t *)parma, sizeof(_SHType)))
          {
            log(DEBUG,"\r\n*******CFG_SYS_SHANGHAI WRITE OK*********\r\n");
            return true;
          }log(DEBUG,"\r\n*******CFG_SYS_SHANGHAI WRITE FAIL*********\r\n");

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
        log(INFO,"[SYS]写到本地\r\n");
        sysCfg_save();
    }
    
    return status;
}

uint32_t cfg_read ( uint8_t mode , void **parma )
{
    uint32_t data = 0;

    switch(mode)
    {
        case MQTT_FILTER_SYNCED:
        {
            data = cfg.parm.filterSynced;
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
        
        case CFG_BLE_MAC:
        {
            *parma = cfg.ble.ble_mac;
        }break;
              
        case CFG_BLE_VERSION:
        {
            data = cfg.ble.ble_version;
        }break;

        case CFG_SYS_OPEN_TIME:
        {
            data = cfg.parm.delay_time;
            log_err("[SYS]READ CFG_SYS_OPEN_TIME:%d\r\n",data);
        }break;
        
        case CFG_SYS_CHIP_ID:
        {
            *parma = &cfg.parm.chipId;
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

        case CFG_SYS_ALARM_TIME:
        {
            data = cfg.parm.alarm_time;
        }break;
        case CFG_SYS_LOCK_MODE:
        {
            data = cfg.parm.lock_mode;
        }break;
 
        case CFG_SYS_MAGNET_STATUS:
        {
             data = cfg.parm.magnet_status;
        }break;
        
        case CFG_NET_ADDR:
        {
             *parma = &cfg.server.net;

        }break;
        
        case CFG_OTA_ADDR:
        {
             *parma = &cfg.server.otanet;

        }break;

         case CFG_OTA_URL:
        {
              *parma = &cfg.server.otaurl;

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
        
        case CFG_MQTT_DC:
        {      
            *parma = SHType.deviceCode;
        }break;
                    
        case CFG_SET_RESTORE_FLAG:
        {
            data = cfg.sysRestoreFlag;
        }break;
        
        case CFG_CLEAR_RESTORE_FLAG:
        {
            data = cfg.sysRestoreFlag;
        }break;
        
        case CFG_SYS_SHANGHAI:
        {
          if(chip_flash_read( DSYS_DIANMA_ADDR ,(uint8_t *)&SHType ,sizeof(_SHType)))
          {       
                printf("\r\nCFG_SYS_SHANGHAI READ OK size = %d\n",sizeof(_SHType));
                return true;
          } printf("\r\nCFG_SYS_SHANGHAI READ FAIL\n");
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
    
    
 

/*上海平台信息*/  
    memset(&SHType,0x00,sizeof(_SHType));
    
    config.read(CFG_SYS_SHANGHAI , (void **)&SHType );

    if(SHType.deviceCode[0]==0XFF)
    {

      memset(&SHType,0x00,sizeof(_SHType));//因为config.read(CFG_SYS_SHANGHAI , (void **)&SHType );已经变成全F
      #if 0
      log_err("\n************新设备****在此设置缺损值******\n");
      static char * deviceCode   =   "310104004005001101003";//21长!!!!!
      memcpy(SHType.deviceCode,deviceCode,strlen(deviceCode));

      //static char * groupcode    =   "223101040040050010000009";//默认通行组  
      //memcpy_down(SHType.gup.code[0],groupcode,22);
      //SHType.gup.cnt=1;
      #else
            log_err("\n************新设备****无操作******\n");
      #endif
    }
    else
    {
      log_err("\n************旧设备*****无操作*****\n");
    }
      
    show_SH(&SHType);

    
/*主题*/  

log(INFO,"topicPath0 %s\r\n",topicPath0);
log(INFO,"topicPath1 %s\r\n",topicPath1);
log(INFO,"topicPath2 %s\r\n",topicPath2);
log(INFO,"topicPath3 %s\r\n",topicPath3);
log(INFO,"topicPath4 %s\r\n",topicPath4);
strncat(topicPath1,SHType.deviceCode,21);
strncat(topicPath2,SHType.deviceCode,21);
strncat(topicPath3,SHType.deviceCode,21);
strncat(topicPath4,SHType.deviceCode,21);
log(INFO,"topicPath0 %s\r\n",topicPath0);
log(INFO,"topicPath1 %s\r\n",topicPath1);
log(INFO,"topicPath2 %s\r\n",topicPath2);
log(INFO,"topicPath3 %s\r\n",topicPath3);
log(INFO,"topicPath4 %s\r\n",topicPath4);
  

    sysCfg_print();

}




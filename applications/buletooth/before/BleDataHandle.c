#if (USE_NANO_PROTO==0)
#include "socket.h"
#include "BleDataHandle.h"
#include "config.h"
#include "buletooth.h"
#include "beep.h"
#include "open_door.h"
#include "permi_list.h"
#include "open_log.h"
#include "magnet.h"

#include "buletooth.h"
#include "timer.h"

#include "unit.h"
#include "httpbuss.h"
#include "sysCfg.h"
#include "cJSON.h"
extern SystemConfigType   cfg;
/*****************************************************************************
** 
Internal function declaration
*
*/

static uint8_t BleApplicationHandle(const BleAppMsgType *pMsg);
static uint8_t BleAppDataDecry(BleAppMsgType *pRecvData ,  BleAppMsgType *pRtData);
static uint8_t BleDataDecry(uint8_t *pRecvMsg , uint8_t *pRtData , uint8_t *pPwd);
static uint8_t BleDataMarryPwd(uint8_t *pRdata , uint8_t *pPwd);

extern BleUserMsgType           BleUserMsg;



/*****************************************************************************
** 
command function declaration
*
*/
uint8_t BleSetDeviceInitInfo( BleAppMsgType *pMsg); 
uint8_t BleSetOpenTime( BleAppMsgType *pMsg);
uint8_t BleModifyPairPwd( BleAppMsgType *pMsg);
uint8_t BleModifyUserPwd( BleAppMsgType *pMsg);
uint8_t BleGetOpenDoorCmd( BleAppMsgType *pMsg);
uint8_t BleSetMangnetAlarmTime( BleAppMsgType *pMsg);
uint8_t BleGetDeviceInitInfo( BleAppMsgType *pMsg);
uint8_t BleGetMangnetStatus( BleAppMsgType *pMsg);
uint8_t BleAppPair( BleAppMsgType *pMsg);
uint8_t BleRestoreDevice( BleAppMsgType *pMsg);


uint8_t BleSetDevicePara(BleAppMsgType *pMsg);
uint8_t BleSetDeviceInstallInfo(BleAppMsgType *pMsg);
uint8_t BleModifyFinish(BleAppMsgType *pMsg);
uint8_t BleModifyPermission(BleAppMsgType *pMsg);
uint8_t BleGetDeviceInstallStatus(BleAppMsgType *pMsg);
uint8_t BleGetDevicePara(BleAppMsgType *pMsg);
/*****************************************************************************
** 
Internal variable declaration
*
*/ 
uint16_t const AnalysisSize[]={68,132,644};
uint8_t OpenRecordPos , OpenRecordNum;
uint8_t const StrRefer[]="0123456789ABCDEF";
uint8_t DevicePrefix[] ={0x4C ,0X00,0X02,0X15};
uint8_t DeviceUUID[] = {0xF1 ,0xCE,0xBB,0x2A,0xB2,0xF8,0x47,0x92,0x85,0x7A,0x2D,0x26,0x8E,0x4F,0x6D,0x94};
uint16_t DeviceMajor = 0x00;
uint16_t DeviceMinor = 0x00;
uint8_t DeviceCheckRssi = 0xBC;


AppHandleArryType gAppTaskArry[]=
{
  {0x01 , BleSetDeviceInitInfo}, //设置设备信息
  {0x02 , BleSetOpenTime}, //设置开门延时
  {0x03 , BleModifyPairPwd},//修改蓝牙匹配密码
  {0x04 , BleModifyUserPwd},//修改开门固定密码
  {0x08 , BleGetOpenDoorCmd},//蓝牙开门
  {0x09 , BleSetMangnetAlarmTime},//设置门磁报警时间
  {0x10 , BleGetDeviceInitInfo},//获取设备信息
  {0x11 , BleGetMangnetStatus},//获取门磁状态
  {0x13 , BleAppPair}, //蓝牙配对
  {0x14 , BleRestoreDevice}, //设备重置
  
  {0x15 , BleSetDevicePara}, //设置设备参数
  {0x16 , BleSetDeviceInstallInfo}, //设备注册
  {0x17 , BleModifyFinish}, //设备修改完成
  {0x18 , BleModifyPermission}, //修改权限验证
  {0x19 , BleGetDeviceInstallStatus}, //获取设备是否安装
  {0x20 , BleGetDevicePara}, //获取设备参数
};
const uint8_t gAppTasksCnt = sizeof (gAppTaskArry) / sizeof (gAppTaskArry[0]);

static uint8_t BleDataMarryPwd(uint8_t *pRdata , uint8_t *pPwd)
{
    uint8_t i = 0 , Pwd[6];
    
    for( i = 0 ; i < 3 ; i++)
    {
        Pwd[i*2] = (pPwd[i] >>4 &0x0f);
        Pwd[i*2+1] = pPwd[i]&0x0f;
    }
    for( i = 0 ; i < 6 ; i++)
    {
        if(pRdata[i+PAIR_PASSWORD_OFFSET] != Pwd[i])
        {
            return FALSE;
        }
    }
    return TRUE;
}
static uint8_t BleDataDecry(uint8_t *pRecvMsg , uint8_t *pRtData , uint8_t *pPwd)
{
    uint8_t i =0 ,DecryKey[8],DataHexTemp[300], DataDecryTemp[300];
    
    makeKeyData(DecryKey,pPwd);
    
    for(i=0;i<24;i++)  
    {
      DataHexTemp[i]= strToInt(*(pRecvMsg+i*2))*16+strToInt(*(unsigned char *)(pRecvMsg+i*2+1));            
    }
    
    for(i=0;i<3;i++)
    {
        DecryStr(DataHexTemp+i*8,DecryKey,DataDecryTemp+i*8);
    }
    for(i=0;i<24;i++)
    {
        *(pRtData+i*2)=*(DataDecryTemp+i)/16;
        *(pRtData+i*2+1)=*(DataDecryTemp+i)%16;
    }

    *(pRtData)=(((*(pRtData))<<4)&0xF0)|*(pRtData+TYPE_LOW_OFFSET);   
    
    if( pRtData[0] == 0x00)
    {
        log(DEBUG,"Ble receive command is err = %x" , pRtData[0]);
        //return APP_DECRY_CMD_ERR;
    }
    
    if( BleDataMarryPwd(pRtData ,pPwd ) != TRUE)
    {
        return APP_DECRY_PWD_ERR;
    }
    return APP_OK;
}

static uint8_t BleAppDataDecry(BleAppMsgType *pRecvData ,  BleAppMsgType *pRtData)
{
    uint8_t i =0,*PairPwd,*UserPwd;

    config.read(CFG_PAIR_PWD , (void **)&PairPwd);
    config.read(CFG_USER_PWD , (void **)&UserPwd);
    
    for( i = 0 ; i < sizeof(AnalysisSize); i++)
    {
        if( pRecvData->DataLength == AnalysisSize[i])
        {
            //log(DEBUG,"Data length is rigth , length=%d.\r\n" , pRecvData->DataLength);
            break;
        }
    }
    if( i == sizeof(AnalysisSize))
    {
        log(DEBUG,"Data length is err , length=%d.\r\n" , pRecvData->DataLength);
        return APP_LENGTH_ERR;
    }
    
    if( BleDataDecry(pRecvData->Data , pRtData->Data , PairPwd) == APP_OK)
    {
        //pRtData->ucType = USE_PAIR_PRIV;
        memcpy(pRtData->Data+48 , pRecvData->Data+48 ,pRecvData->DataLength-48 );
        return APP_OK;
    }
    else
    {
        if( BleDataDecry(pRecvData->Data , pRtData->Data , UserPwd) == APP_OK)
        {
            if( pRtData->Data[0] != 0x08)
            {
                log(DEBUG,"Use user operation , only use  iD=08, ID=%x,\r\n" ,pRtData->Data[0]);
                return APP_DECRY_CMD_ERR;
            }
            
            
            //pRtData->ucType = USE_USER_PRIV;
            log(DEBUG,"Only use open command, ID=08.\r\n");
            memcpy(pRtData->Data+48 , pRecvData->Data+48 ,pRecvData->DataLength-48 );
            return APP_OK;
        }
    }
    return APP_DECRY_ERR;
}

static uint8_t BleApplicationHandle(const BleAppMsgType *pMsg)
{
    uint8_t idx =0 ;

    for(idx = 0; idx < gAppTasksCnt; idx++)
    {
        if( pMsg->Data[0] == gAppTaskArry[idx].Id)
        {
            log(DEBUG,"Bt receive command id = %x.\r\n" ,  gAppTaskArry[idx].Id);
            
            

          //  for(int i=0;i<140;i++)
          //  {printf("%02X",pMsg->Data[i]);    if((i+1)%20 ==0)printf("\n"); }
            
            
            if(gAppTaskArry[idx].EventHandlerFn != NULL)
            {
                return (gAppTaskArry[idx].EventHandlerFn((BleAppMsgType*)pMsg));               
            }
            else
            {
                log(DEBUG,"This command has no callback  , CMD = %x.\r\n" , pMsg->Data[0]);
                return APP_NO_COMMAND_ERR;
            }
        }
    }
    log(DEBUG,"This command has no register. cmd = %x.\r\n" ,pMsg->Data[0]);
    return APP_NO_COMMAND_ERR;
}


void BeepOpen( uint8_t Type )
{
    if(Type == APP_OK)
    {
        beep.write(BEEP_NORMAL);  
    }
    else
    {
        if( Type != APP_OK_NO_BEEP)
        {
        beep.write(BEEP_ALARM);   
        }
    }
}

uint8_t BleDataProcess(BleAppMsgType *msg)
{
    BleAppMsgType   BleAppMsg;
    uint8_t  uRt = BLE_INIT_ERR; 
    memset(&BleAppMsg , 0x00 , sizeof(BleAppMsgType));
    memcpy(&BleAppMsg , msg , sizeof(AppDataHeardType));      
    if( (uRt =BleAppDataDecry(msg,&BleAppMsg)) != APP_OK)//解密
    {      
         if( uRt == APP_LENGTH_ERR)
            btModule.send(&BleAppMsg ,"00LOSE",6);
        else         
            btModule.send(&BleAppMsg,ERR_PASSuint16_t,6 );
    }
    else
    { 
        uRt = BleApplicationHandle(&BleAppMsg);//处理
        BeepOpen(uRt);     

    }
   
    return uRt;
}

uint8_t ble_open_door( BleAppMsgType *pMsg )
{
    open_door();
    return TRUE;
}

/////////////////////////////Applicaiton phone command handle.///////////////////////////
/*
* 设置设备信息---最后
*/
uint8_t BleSetDeviceInitInfo( BleAppMsgType *pMsg)
{
    httpRecvMsgType p;
    uint8_t lock_mode = 0xFF ,i = 0 ,CommTemp[14]={0x00};
    uint64_t dwCommunities[4]={0x00,0x00,0x00,0x00};
    SystemCommunitiesType   communityTemp;
    SystemCommunitiesType   *deviceCommunity=NULL;
    
    config.read(CFG_SYS_COMMUN_CODE , (void **)&deviceCommunity);

    if(( pMsg->Data[24] != 'F')&&( pMsg->Data[25] != 'F'))
        lock_mode  = GET_STR_BYTE(pMsg->Data , 24);
    
    for(i = 0 ; i < 8 ; i++)
    {
        CommTemp[i] = GET_STR_BYTE(pMsg->Data , 8+i*2);
    }
                                  
    dwCommunities[0] = (int64_t)(CommTemp[0])<<24|(int64_t)(CommTemp[1])<<16|(int64_t)(CommTemp[2])<<8|(int64_t)(CommTemp[3]);
    dwCommunities[1] = CommTemp[5]<<8|CommTemp[6];
    dwCommunities[2] = CommTemp[7];
    dwCommunities[3] = 0;
    
    log(ERR,"设备类型lock_mode:【0X%X】 ,小区:%llx,楼栋:%llx,单元:%llx\r\n" ,lock_mode, dwCommunities[0],dwCommunities[1],dwCommunities[2]);
    /*lock_mode:0X10 ,小区:b9f06b,楼栋:600,单元:0*/
    memset(&communityTemp  , 0x00 , sizeof(SystemCommunitiesType));
    communityTemp.communities[0].village_id = dwCommunities[0];
    communityTemp.communities[0].building_id = dwCommunities[1];
    communityTemp.communities[0].floor_id = dwCommunities[2];
    communityTemp.communities[0].house_id = 0;
    
    
    lock_mode -= 0x10;
    
#if 1     /*冲突*/
    if(lock_mode != 0xff )
    {
        config.write(CFG_SYS_LOCK_MODE , &lock_mode , false);/*我是围墙机这里必须是2 你给了什么？ 0X12   门口机给的是0X10*/
    }
#endif
    if((aiot_strcmp((uint8_t *)&(communityTemp.communities[0]) , (uint8_t *)&(deviceCommunity->communities[0]) , sizeof(TSLCommunityModel)) == FALSE ))
    {
        log(INFO,"安装位置有变化\n");
    }

    config.write(CFG_SYS_COMMUN_CODE , &communityTemp , false);  
    
    log(ERR,"BLE安装结束\n");
    
    magnet_input_status_init(0);/*一句话 不要在门磁吸合的时候去安装！！ 也就是安装的时候 那个状态 是正常状态 否则门磁吸合没报警 离开了反而报警*/
    
    config.write(CFG_NOTHING , NULL , TRUE);
   
    //socket_compon_init();
    //socket_close_f();
    
    
    btModule.send(pMsg ,"01SUCC",strlen("01SUCC"));          
          
    p.rule = 'A';
    xQueueSend(xHttpRecvQueue, (void*)&p, NULL);
    

        
    return APP_OK;
}

/*
* 设置开门延时时间
*/
uint8_t BleSetOpenTime(BleAppMsgType *pMsg)
{
    uint16_t OpenDelay = 0xffff;
    
    log(DEBUG,"设置开门延时时间\n");
    
    OpenDelay   = (pMsg->Data[8]*10+pMsg->Data[9])*10;
    
    if(OpenDelay != 0xffff )
    {
        config.write(CFG_SYS_OPEN_TIME , &OpenDelay , FALSE);
    }
    
    btModule.send(pMsg ,"02SUCC",strlen("02SUCC"));
    
    return APP_OK;
}

/*
* 修改配对密码
*/

uint8_t BleModifyPairPwd( BleAppMsgType *pMsg)
{
    uint8_t NewSecret[3] , i;

    for( i = 0 ; i < 3; i++)
    {
        NewSecret[i] = GET_BYTE(pMsg->Data , i*2+8);
    }
    
    log(DEBUG,"New pair secret is:%x%x%x\n" , NewSecret[0],NewSecret[1],NewSecret[2]);
    
    config.write(CFG_PAIR_PWD , NewSecret , FALSE);
    btModule.send(pMsg ,"03SUCC",strlen("03SUCC"));
    return APP_OK;
}

/*
* 修改用户密码
*/

uint8_t BleModifyUserPwd( BleAppMsgType *pMsg)
{
    uint8_t NewSecret[3] , i;
    
    for( i = 0 ; i < 3; i++)
    {
        NewSecret[i] = GET_BYTE(pMsg->Data , i*2+14);
    }
    
    log(DEBUG,"New user secret is:%x%x%x\n" , NewSecret[0],NewSecret[1],NewSecret[2]);
    
    config.write(CFG_USER_PWD , NewSecret , FALSE);
    btModule.send(pMsg ,"04SUCC",strlen("04SUCC"));
    return APP_OK;
}

/*
* 蓝牙开门命令
*/
uint8_t BleGetOpenDoorCmd( BleAppMsgType *pMsg)
{
    ble_open_door(pMsg);

    btModule.send(pMsg ,"08SUCC",strlen("08SUCC"));
   
    return APP_OK;
}

/*
* 设置门磁报警时间
*/
uint8_t BleSetMangnetAlarmTime( BleAppMsgType *pMsg)
{
    uint8_t AlarmDelay = 0xFF;
    
    AlarmDelay   = pMsg->Data[8]*10+pMsg->Data[9];
    
        log(DEBUG,"设置门磁报警时间 %d\n",AlarmDelay);
        
    if(AlarmDelay != 0xff )
    {
        config.write(CFG_SYS_ALARM_TIME , &AlarmDelay , FALSE);
    }
    
    
    btModule.send(pMsg ,"09SUCC",strlen("09SUCC"));
    
    return APP_OK;
}




uint8_t BleGetMangnetStatus( BleAppMsgType *pMsg)
{
    uint8_t ManganetStatue = 0, cnt = 0;

    uint8_t SendData[100];

    memset(SendData , 0x00 , sizeof(SendData));

    ManganetStatue = config.read(CFG_SYS_MAGNET_STATUS , NULL); 
    
    memcpy(SendData , "11SUCC" , strlen("11SUCC"));
    cnt+=6;
  
    SendData[cnt++] = GET_MSB_STR(ManganetStatue);
    SendData[cnt++] = GET_LSB_STR(ManganetStatue);
   
    btModule.send(pMsg ,SendData,cnt);
    
    return APP_OK;
}

uint8_t BleAppPair(BleAppMsgType *pMsg)
{
    log(DEBUG,"蓝牙配对成功\n");
    
    btModule.send(pMsg ,"13SUCC",strlen("13SUCC"));
    
    return APP_OK;
}

uint8_t BleRestoreDevice(BleAppMsgType *pMsg)
{
      Reset_HTTPStatus();
      log(DEBUG,"设备恢复出厂设置 \n");
      httpRecvMsgType p;
      btModule.send(pMsg ,"14SUCC",strlen("14SUCC"));
      p.rule = 'B';
      xQueueSend(xHttpRecvQueue, (void*)&p, NULL);
      return APP_OK; 

}

char cj_parse_Para(const char * pJson)
{
     #define TEMSIZE 50
      char tem[TEMSIZE];
      char *p=NULL;
      if(NULL == pJson) return 1;
      cJSON * pRoot = cJSON_Parse(pJson);
      if(NULL == pRoot) 
      {
          const char *error_ptr = cJSON_GetErrorPtr();
          if (error_ptr != NULL) log(ERR,"Error before: %s\n", error_ptr);
          cJSON_Delete(pRoot);
          return 2;
      }

      memset( &cfg.server,0,sizeof(netAttriType));
      
      cJSON * pSub = cJSON_GetObjectItem(pRoot, "httpUrl");
      if(NULL == pSub){cJSON_Delete(pRoot);return 3;}
      printf("【%d】【%s】\r\n",__LINE__,pSub->valuestring);  
// "httpUrl":"192.168.66.34:8211"  可以 优化
      memset(tem,0,TEMSIZE);
      
      if(p = strstr(pSub->valuestring,"://"))
        p+=3;
      else
        p=pSub->valuestring;
      
      sprintf(tem,"%.20s",p);

      p = strstr(tem,":");
      *p = 0;
      cfg.server.httpport = atoi(++p);
      memcpy(cfg.server.net.ip , tem , strlen(tem));
  printf("HTTP----【%d】【%s】\r\n", cfg.server.httpport, cfg.server.net.ip    );  
 
      pSub = cJSON_GetObjectItem(pRoot, "mqttUrl");
      if(NULL == pSub){cJSON_Delete(pRoot);return 3;}
      printf("【%d】【%s】\r\n",__LINE__,pSub->valuestring);  
// "httpUrl":"192.168.66.34:8211"
      memset(tem,0,TEMSIZE);
      if(p = strstr(pSub->valuestring,"://"))
        p+=3;
      else
        p=pSub->valuestring;
    
      sprintf(tem,"%.20s",p);
 
      p = strstr(tem,":");
      *p = 0;
      cfg.server.net.port = atoi(++p);
 
  printf("MQTT----【%d】【%s】\r\n",  cfg.server.net.port, cfg.server.net.ip    );  
 
      
      pSub = cJSON_GetObjectItem(pRoot, "isDHCP");
      if(NULL == pSub){cJSON_Delete(pRoot);return 3;}
      printf("【%d】【%d】\r\n",__LINE__,pSub->valueint);
      
        if(pSub->valueint ==1 )
        {
            printf("----------------------设置为DHCP---------------------\r\n");
            cfg.devIp.dhcpFlag = 1;
        }      
        else
        {
            cfg.devIp.dhcpFlag = 0;
            printf("----------------------设置为静态cfg.devIp.dhcpFlag = 0---------------------\r\n");
            pSub = cJSON_GetObjectItem(pRoot, "ip");
            if(NULL == pSub){cJSON_Delete(pRoot);return 3;}
            printf("【%d】【%s】\r\n",__LINE__,pSub->valuestring); 
            memset(tem,0,TEMSIZE);
            sprintf(tem,"%.16s",pSub->valuestring);
            IPStrTO4ARR(cfg.devIp.ip,(uint8_t *)tem);

            pSub = cJSON_GetObjectItem(pRoot, "mask");
            if(NULL == pSub){cJSON_Delete(pRoot);return 3;}
            printf("【%d】【%s】\r\n",__LINE__,pSub->valuestring); 
            memset(tem,0,TEMSIZE);
            sprintf(tem,"%.16s",pSub->valuestring);
            IPStrTO4ARR(cfg.devIp.mark,(uint8_t *)tem);

            pSub = cJSON_GetObjectItem(pRoot, "gateway");
            if(NULL == pSub){cJSON_Delete(pRoot);return 3;}
            printf("【%d】【%s】\r\n",__LINE__,pSub->valuestring); 
            memset(tem,0,TEMSIZE);
            sprintf(tem,"%.16s",pSub->valuestring);
            IPStrTO4ARR(cfg.devIp.gateway,(uint8_t *)tem);

            pSub = cJSON_GetObjectItem(pRoot, "DNS");
            if(NULL == pSub){cJSON_Delete(pRoot);return 3;}
            printf("【%d】【%s】\r\n",__LINE__,pSub->valuestring); 
            memset(tem,0,TEMSIZE);
            sprintf(tem,"%.16s",pSub->valuestring);
            IPStrTO4ARR(cfg.devIp.dns,(uint8_t *)tem);

            log(DEBUG,"SIP: %d.%d.%d.%d\n", cfg.devIp.ip[0],cfg.devIp.ip[1],cfg.devIp.ip[2],cfg.devIp.ip[3]);
            log(DEBUG,"GAR: %d.%d.%d.%d\n", cfg.devIp.gateway[0],cfg.devIp.gateway[1],cfg.devIp.gateway[2],cfg.devIp.gateway[3]);
            log(DEBUG,"SUB: %d.%d.%d.%d\n", cfg.devIp.mark[0],cfg.devIp.mark[1],cfg.devIp.mark[2],cfg.devIp.mark[3]);
            log(DEBUG,"DNS: %d.%d.%d.%d\n", cfg.devIp.dns[0],cfg.devIp.dns[1],cfg.devIp.dns[2],cfg.devIp.dns[3]);
      }  
     
      
      cJSON_Delete(pRoot);
     #undef TEMSIZE 
      return 0;
}

uint8_t BleSetDevicePara(BleAppMsgType *pMsg)
{
    log(DEBUG,"设置设备参数-各种IP\n");

    char *pd = (char *) pMsg->Data+48;

    printf("BleSetDevicePara[%s]\r\n",pd);
    

    if(cj_parse_Para(pd)==0)
      
    {
      
      socket_close_f();//复位一下
      btModule.send(pMsg ,"15SUCC",strlen("15SUCC"));
    }
    else
       btModule.send(pMsg ,"15FAIL",strlen("15FAIL"));     


    return APP_OK;
} 


extern _ble2http *bh;

char cj_parse_install(const char * pJson)
{

      if(NULL == pJson) return 1;
      cJSON * pRoot = cJSON_Parse(pJson);
      if(NULL == pRoot) 
      {
          const char *error_ptr = cJSON_GetErrorPtr();
          if (error_ptr != NULL) log(ERR,"Error before: %s\n", error_ptr);
          cJSON_Delete(pRoot);
          return 2;
      }

      memset(bh,0,sizeof(_ble2http));
      cJSON * pSub = cJSON_GetObjectItem(pRoot, "deviceName");
      if(NULL == pSub){cJSON_Delete(pRoot);return 3;}
      printf("【%d】【%s】\r\n",__LINE__,(char *)pSub->valuestring);
      sprintf(bh->NAME,"%.20s",(char *)pSub->valuestring);
      
      pSub = cJSON_GetObjectItem(pRoot, "streetID");
      if(NULL == pSub){cJSON_Delete(pRoot);return 3;}
      printf("【%d】【%s】\r\n",__LINE__,(char *)pSub->valuestring);
      sprintf(bh->streetID,"%.32s",(char *)pSub->valuestring);

      pSub = cJSON_GetObjectItem(pRoot, "committeeID");
      if(NULL == pSub){cJSON_Delete(pRoot);return 3;}
      printf("【%d】【%s】\r\n",__LINE__,(char *)pSub->valuestring);
      sprintf(bh->committeeID,"%.32s",(char *)pSub->valuestring);
      
      pSub = cJSON_GetObjectItem(pRoot, "villageID");
      if(NULL == pSub){cJSON_Delete(pRoot);return 3;}
      sprintf(bh->villageID,"%.32s",(char *)pSub->valuestring); 
      
      pSub = cJSON_GetObjectItem(pRoot, "buildingID");
      if(NULL == pSub){cJSON_Delete(pRoot);return 3;}
      log(ERR,"【%d】【%s】\r\n",__LINE__,(char *)pSub->valuestring);
      sprintf(bh->buildingID,"%.32s",(char *)pSub->valuestring);       

      pSub = cJSON_GetObjectItem(pRoot, "longitude");
      if(NULL == pSub){cJSON_Delete(pRoot);return 3;}
      printf("【%d】【%f】\r\n",__LINE__,pSub->valuedouble); 
      bh->longitude = pSub->valuedouble;

      pSub = cJSON_GetObjectItem(pRoot, "latitude");
      if(NULL == pSub){cJSON_Delete(pRoot);return 3;}
      printf("【%d】【%f】\r\n",__LINE__,pSub->valuedouble);
      bh->latitude = pSub->valuedouble;
      
      pSub = cJSON_GetObjectItem(pRoot, "seriesType");
      if(NULL == pSub){cJSON_Delete(pRoot);return 3;}
      printf("【%d】【%d】\r\n",__LINE__,pSub->valueint);
      bh->type = pSub->valueint;
        
      pSub = cJSON_GetObjectItem(pRoot, "productName");
      if(NULL == pSub){cJSON_Delete(pRoot);return 3;}
      printf("【%d】【%s】\r\n",__LINE__,pSub->valuestring);   
      sprintf(bh->productName_productModel,"%.9s",(char *)pSub->valuestring); 
  
      cJSON_Delete(pRoot);
      
      
      return 0;
}

uint8_t BleSetDeviceInstallInfo(BleAppMsgType *pMsg)
{
    log(DEBUG,"设置设备注册信息\n");
    
    char *pd = (char *) pMsg->Data+48;

    printf("BleSetDevicePara\r\n【%s】\r\n",pd);

   if(cj_parse_install(pd)==0)
       btModule.send(pMsg ,"16SUCC",strlen("16SUCC"));
   else
       btModule.send(pMsg ,"16FAIL",strlen("16FAIL"));     
    
    return APP_OK;
}


uint8_t BleModifyFinish(BleAppMsgType *pMsg)
{
    log(DEBUG,"设备修改完成-重装的最后一个[逻辑是--水平是0 维持不变 去复位 水平是1暴力写为0去复位]\n");
    
    char foce =0;
    btModule.send(pMsg ,"17SUCC",strlen("17SUCC"));
    magnet_input_status_init(0);//读一下门磁
    config.write(CFG_DEV_LEVEL , &foce,0);
    
    config.write(CFG_NOTHING , NULL , TRUE);
    
    set_clear_flash(FLASH_ALL_BIT_BUT_CFG);
    
    return APP_OK;
}

uint8_t BleModifyPermission(BleAppMsgType *pMsg)
{
    uint8_t *userPwd;
  
    log(DEBUG,"修改权限验证\n");
    
    config.read(CFG_USER_PWD , (void **)&userPwd ); 
    
    uint8_t userSecret[3] , i;
    
    for( i = 0 ; i < 3; i++)
    {
        userSecret[i] = GET_BYTE(pMsg->Data , i*2+8);
    }
    
    log(DEBUG,"安装工传的 user secret is:%x%x%x\n" , userSecret[0],userSecret[1],userSecret[2]);
    
    log(DEBUG,"本地的 user secret is:%x%x%x\n" , userPwd[0],userPwd[1],userPwd[2]);
    
    if((userPwd[0]== userSecret[0])&&(userPwd[1]== userSecret[1])&&(userPwd[2]== userSecret[2]))
    {
        log(DEBUG,"一样");
        btModule.send(pMsg ,"18SUCC",strlen("18SUCC"));
    }
    else
    {
        log(DEBUG,"不一样");
        btModule.send(pMsg ,"18SUCC",strlen("18FAIL"));
    }
      
    return APP_OK;
}

uint8_t BleGetDeviceInstallStatus(BleAppMsgType *pMsg)
{
    log(DEBUG,"获取设备是否安装\n");

    uint8_t cnt = 0,leve=0;
    uint8_t SendData[100];

    memset(SendData , 0x00 , sizeof(SendData));
 
    leve = config.read(CFG_DEV_LEVEL , NULL);//10没有注册   9注册过

    log(DEBUG,"读内存判断是否BLE下发过获取安装状态leve=%d\n",leve);
    memcpy(SendData , "19SUCC" , strlen("19SUCC"));
    cnt+=6;


    
    if(leve == 1)
    {
        SendData[cnt++] = '0';
        SendData[cnt++] = '9';
    }
    else if(leve == 0)
    {
        SendData[cnt++] = '1';
        SendData[cnt++] = '0';
    }
    
    btModule.send(pMsg ,SendData,cnt);
    /*printf("%s",SendData);*/
    /*APP 逻辑 即使我返回了10 也是已经安装 说明它关联了平台*/
    return APP_OK;
}





/*
* 获取设备信息  BLE有提示 写死算了 烦人
*/
uint8_t BleGetDeviceInitInfo( BleAppMsgType *pMsg)
{
    //uint8_t lock_mode = 0 ,  DeviceNum = 0 , AlarmTime = 0 , OpenDelay = 0 ;
    uint8_t i = 0,cnt = 0; 
    uint8_t SendData[100];
    uint16_t swVer = config.read(CFG_SYS_SW_VERSION , NULL); 
    char versionData[14];
     
    memset(SendData , 0x00 , sizeof(SendData));
    memset(versionData , 0x00 , sizeof(versionData));
    
    sprintf(versionData ,"0000000000%04d" , swVer);
   

    //lock_mode = config.read(CFG_SYS_LOCK_MODE ,  NULL)+0X10; 
    //OpenDelay  = config.read(CFG_SYS_OPEN_TIME ,  NULL)/10; 
    //AlarmTime  = config.read(CFG_SYS_ALARM_TIME , NULL); 
    //DeviceNum  = config.read(CFG_SYS_DEVICE_NUM , NULL); 
    
    memcpy(SendData , "10SUCC" , strlen("10SUCC"));
    cnt+=6;
    
    for(i = 0; i < 14 ; i++)
    {
        SendData[cnt++] = versionData[i];
    }
     
    SendData[cnt++] = '1';//GET_MSB_STR(lock_mode);
    SendData[cnt++] = '2';//GET_LSB_STR(lock_mode);
    SendData[cnt++] = '0';//GET_MSB_STR(DeviceNum);
    SendData[cnt++] = '1';//GET_LSB_STR(DeviceNum);
    SendData[cnt++] = '0';//GET_STR(OpenDelay/16);
    SendData[cnt++] = '2';//GET_STR(OpenDelay%16);
    SendData[cnt++] = '0';//GET_STR(AlarmTime/16);
    SendData[cnt++] = '2';//GET_STR(AlarmTime%16);
    SendData[cnt++] = 0x0b;
    SendData[cnt++] = 0x7c;
    SendData[cnt++] = 0x7d;
    SendData[cnt++] = 0x7e;

    
    btModule.send(pMsg ,SendData,cnt);
    //printf("%s--",SendData);
    return APP_OK;
}


uint16_t BLE_packINFO(uint8_t *buf)
{
    #define TEMSIZE 16
    char tem[TEMSIZE];
    cJSON *root = NULL;

    char *outStr;

    root =  cJSON_CreateObject();
    cJSON_AddStringToObject(root,"httpUrl",         "");
    cJSON_AddStringToObject(root,"mqttUrl",          "");

    cJSON_AddStringToObject(root,"centerSip",     "");
    cJSON_AddStringToObject(root,"sipAccount",  "");
    cJSON_AddStringToObject(root,"sipUrl",         "");
    cJSON_AddStringToObject(root,"sipPwd",      "");

    memset(tem,0,TEMSIZE);
    IP4ARRToStr(cfg.devIp.ip,tem);
    cJSON_AddStringToObject(root,"ip",          tem);
    memset(tem,0,TEMSIZE);
    IP4ARRToStr(cfg.devIp.mark,tem);
    cJSON_AddStringToObject(root,"mask",          tem);
    memset(tem,0,TEMSIZE);
    IP4ARRToStr(cfg.devIp.gateway,tem);
    cJSON_AddStringToObject(root,"gateway",      tem);
    memset(tem,0,TEMSIZE);
    IP4ARRToStr(cfg.devIp.dns,tem);
    cJSON_AddStringToObject(root,"DNS",          tem);


    outStr = cJSON_Print(root);
    cJSON_Delete(root);
    
    memcpy(buf,outStr,strlen(outStr));
    #undef TEMSIZE 
    return strlen(outStr);

}


uint8_t BleGetDevicePara(BleAppMsgType *pMsg)/////////难道直接发送不行吗？？？看A9的
{
    log(DEBUG,"获取设备参数[和以前不同 不知道数据占位 小问题]\n");

    uint16_t cnt = 0;
    uint8_t SendData[DEV_PARA_DATA_LENGTH+64];
    memcpy(SendData , "20SUCC" , strlen("20SUCC"));
    cnt+=6;
    cnt += BLE_packINFO(&SendData[6]);
    SendData[cnt++] = 0x0b;
    SendData[cnt++] = 0x7c;
    SendData[cnt++] = 0x7d;
    SendData[cnt++] = 0x7e;
    btModule.send(pMsg ,SendData,cnt);
    
    return APP_OK;
}

#endif

#ifndef _TSL_DATA_HANDLE_H_
#define _TSL_DATA_HANDLE_H_

#include "swipeTag.h"
#include "sysCfg.h"
#include "rtc.h"
#include "timer.h"



typedef struct _communityType{
    uint8_t communityID[5];     // 小区编号   5字节
    uint8_t buildingID[2];      // 楼栋号     2字节
    uint8_t unitID;             // 单元号     1字节     
}communityType;

typedef struct _tagUserDataType{
    //0区 1块   密码KeyA 111111111111
    communityType community;    // 主住户信息   8字节
    uint8_t house0[2];          //房间号
    uint8_t name[6];            //业主姓名
    //0区 2块
    uint8_t setting;            // 设置    1字节
    uint8_t start[3];           // 发卡时间    3字节
    uint8_t stop[3];            // 截止时间    3字节
    uint8_t crc0;               // 校验    1字节
    uint8_t cardID[4];          //卡号  4字节
    uint8_t version;            //版本  1
    uint8_t cardType;           //卡类  1
    uint8_t crc16_0[2];         //CRC      2
    //1区  0块
    uint8_t id[9];              //身份证      9（当最后一位为x 时   用A代替）
    //2区  0块1块
    communityType communitys[4];// 主住户信息   4*8字节
    //2区  2块
    uint8_t house1[2];          //房间号1
    uint8_t house2[2];          //房间号2
    uint8_t house3[2];          //房间号3
    uint8_t house4[2];          //房间号4
    uint8_t crc1;               // 校验    1字节
    uint8_t fdgs1[4];           //分断      1*4
    uint16_t userFloor;
    uint8_t crc16_1[2];         //CRC 16
}tagUserDataType;               //用户卡 共89字节


typedef struct _tagConfigDataType{
    //0区 1块   密码KeyA 111111111111
    uint8_t phoneTime;          // 通话时间   1字节
    uint8_t openDelayTime;      // 开门延时   1字节 最小单位 0.1秒
    uint8_t alarmDelayTime;     // 报警延时   1字节
    uint8_t beepCnt;            // 振铃次数   1字节
    uint8_t reDialDelayTime;    // 重拨延时   1字节
    uint8_t openDoorButton;     // 开门按钮   1字节
    uint8_t openDoorPrompt;     // 开门提示   1字节
    uint8_t crc1;               // 校验   1字节

    uint8_t deviceAdd;          // 设备地址   1字节
    uint8_t lock;               // 新旧锁具   1字节
    uint8_t floorOffset;      // 楼层偏移   1字节
    uint8_t houseOffset;      // 室号偏移   1字节
    uint8_t alarmFeedback;    // 报警反馈   1字节
    uint8_t cardFeedback;     // 刷卡反馈   1字节
    uint8_t passwordSetting;  // 密码设置   1字节
    uint8_t crc2;             // 校验   1字节
    //0区 2块
    uint8_t setting;          // 设置    1字节用户卡 
    uint8_t start[3];          // 发卡时间    3字节
    uint8_t stop[3];           // 截止时间    3字节
    uint8_t crc0;             // 校验    1字节
    uint8_t cardID[4];        //卡号  4字节
    uint8_t version;          //版本  1
    uint8_t crc16_0[2];       //CRC 16
    uint8_t operation;        //操作  1
    //1区  0块
    uint8_t tempSfz[9];       //temp      9（当最后一位为x 时   用A代替）
    //2区  0块1块2块
    communityType community[5];// 社区居委   8*6字节  
    uint8_t temp[6];
    uint8_t crc16_1[2];         //CRC 16
}tagConfigDataType;             //共89字节 配置卡


typedef struct _tagUnitAdminDataType{
    //0区 1块   密码KeyA 111111111111
    uint8_t CommunityID[5];       // 小区ID   5字节
    uint8_t bianhao;              //编号
    uint8_t buildingUnitID[3];    // 楼栋号+单元号ID    3字节
    uint8_t function;             //功能
    uint8_t name[6];              //持卡人姓名
    //0区 2块
    uint8_t setting;              // 设置    1字节用户卡 
    uint8_t start[3];             // 发卡时间    3字节
    uint8_t stop[3];              // 截止时间    3字节
    uint8_t crc0;                 // 校验    1字节
    uint8_t startHm[2];           //开始时分  2字节
    uint8_t stopHm[2];            //结束时分  2字节
    uint8_t version;              //版本  1
    uint8_t leixing;              //类型  1
    uint8_t crc[2];                 //CRC 16
//1区  0块
    uint8_t ID[9];                  //身份证      9（当最后一位为x 时   用A代替）
//2区  0块1块
    communityType communitys[4];   // 主住户信息   4*8字节
//2区  2块
    uint8_t temp[8];                //
    uint8_t crc1;                 // 校验    1字节
    uint8_t temp1[4];               //分断      1*4
    uint8_t temp2;
    uint8_t crc2[2];                //CRC 16
}tagUnitAdminDataType;              //共16字节 物业卡






/////////////////EXCEL CARD//////////////////

typedef struct 
{  
  uint8_t class;      
  uint8_t type;     
  uint8_t uid[4];       
  uint8_t ID[9];     
  uint8_t crc;    
}headtype;

typedef struct 
{  
  uint8_t group[11];    
  uint8_t endtime[3];     
  uint8_t crc;      
  uint8_t power;   
}bodytype;


typedef union 
{
	uint8_t 	crc[2];
	uint16_t 	crc16;
}CRCtype;


typedef struct 
{
   headtype  head;
   bodytype  body[5];   
   CRCtype   crc;    
}shanghaicardtype;




uint8_t tag_shanghai_card_process( tagBufferType *tag);

#endif
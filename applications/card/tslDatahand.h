#ifndef _TSL_DATA_HANDLE_H_
#define _TSL_DATA_HANDLE_H_

#include "swipeTag.h"
#include "sysCfg.h"
#include "rtc.h"
#include "timer.h"



typedef struct _communityType{
    uint8_t communityID[5];     // С�����   5�ֽ�
    uint8_t buildingID[2];      // ¥����     2�ֽ�
    uint8_t unitID;             // ��Ԫ��     1�ֽ�     
}communityType;

typedef struct _tagUserDataType{
    //0�� 1��   ����KeyA 111111111111
    communityType community;    // ��ס����Ϣ   8�ֽ�
    uint8_t house0[2];          //�����
    uint8_t name[6];            //ҵ������
    //0�� 2��
    uint8_t setting;            // ����    1�ֽ�
    uint8_t start[3];           // ����ʱ��    3�ֽ�
    uint8_t stop[3];            // ��ֹʱ��    3�ֽ�
    uint8_t crc0;               // У��    1�ֽ�
    uint8_t cardID[4];          //����  4�ֽ�
    uint8_t version;            //�汾  1
    uint8_t cardType;           //����  1
    uint8_t crc16_0[2];         //CRC      2
    //1��  0��
    uint8_t id[9];              //���֤      9�������һλΪx ʱ   ��A���棩
    //2��  0��1��
    communityType communitys[4];// ��ס����Ϣ   4*8�ֽ�
    //2��  2��
    uint8_t house1[2];          //�����1
    uint8_t house2[2];          //�����2
    uint8_t house3[2];          //�����3
    uint8_t house4[2];          //�����4
    uint8_t crc1;               // У��    1�ֽ�
    uint8_t fdgs1[4];           //�ֶ�      1*4
    uint16_t userFloor;
    uint8_t crc16_1[2];         //CRC 16
}tagUserDataType;               //�û��� ��89�ֽ�


typedef struct _tagConfigDataType{
    //0�� 1��   ����KeyA 111111111111
    uint8_t phoneTime;          // ͨ��ʱ��   1�ֽ�
    uint8_t openDelayTime;      // ������ʱ   1�ֽ� ��С��λ 0.1��
    uint8_t alarmDelayTime;     // ������ʱ   1�ֽ�
    uint8_t beepCnt;            // �������   1�ֽ�
    uint8_t reDialDelayTime;    // �ز���ʱ   1�ֽ�
    uint8_t openDoorButton;     // ���Ű�ť   1�ֽ�
    uint8_t openDoorPrompt;     // ������ʾ   1�ֽ�
    uint8_t crc1;               // У��   1�ֽ�

    uint8_t deviceAdd;          // �豸��ַ   1�ֽ�
    uint8_t lock;               // �¾�����   1�ֽ�
    uint8_t floorOffset;      // ¥��ƫ��   1�ֽ�
    uint8_t houseOffset;      // �Һ�ƫ��   1�ֽ�
    uint8_t alarmFeedback;    // ��������   1�ֽ�
    uint8_t cardFeedback;     // ˢ������   1�ֽ�
    uint8_t passwordSetting;  // ��������   1�ֽ�
    uint8_t crc2;             // У��   1�ֽ�
    //0�� 2��
    uint8_t setting;          // ����    1�ֽ��û��� 
    uint8_t start[3];          // ����ʱ��    3�ֽ�
    uint8_t stop[3];           // ��ֹʱ��    3�ֽ�
    uint8_t crc0;             // У��    1�ֽ�
    uint8_t cardID[4];        //����  4�ֽ�
    uint8_t version;          //�汾  1
    uint8_t crc16_0[2];       //CRC 16
    uint8_t operation;        //����  1
    //1��  0��
    uint8_t tempSfz[9];       //temp      9�������һλΪx ʱ   ��A���棩
    //2��  0��1��2��
    communityType community[5];// ������ί   8*6�ֽ�  
    uint8_t temp[6];
    uint8_t crc16_1[2];         //CRC 16
}tagConfigDataType;             //��89�ֽ� ���ÿ�


typedef struct _tagUnitAdminDataType{
    //0�� 1��   ����KeyA 111111111111
    uint8_t CommunityID[5];       // С��ID   5�ֽ�
    uint8_t bianhao;              //���
    uint8_t buildingUnitID[3];    // ¥����+��Ԫ��ID    3�ֽ�
    uint8_t function;             //����
    uint8_t name[6];              //�ֿ�������
    //0�� 2��
    uint8_t setting;              // ����    1�ֽ��û��� 
    uint8_t start[3];             // ����ʱ��    3�ֽ�
    uint8_t stop[3];              // ��ֹʱ��    3�ֽ�
    uint8_t crc0;                 // У��    1�ֽ�
    uint8_t startHm[2];           //��ʼʱ��  2�ֽ�
    uint8_t stopHm[2];            //����ʱ��  2�ֽ�
    uint8_t version;              //�汾  1
    uint8_t leixing;              //����  1
    uint8_t crc[2];                 //CRC 16
//1��  0��
    uint8_t ID[9];                  //���֤      9�������һλΪx ʱ   ��A���棩
//2��  0��1��
    communityType communitys[4];   // ��ס����Ϣ   4*8�ֽ�
//2��  2��
    uint8_t temp[8];                //
    uint8_t crc1;                 // У��    1�ֽ�
    uint8_t temp1[4];               //�ֶ�      1*4
    uint8_t temp2;
    uint8_t crc2[2];                //CRC 16
}tagUnitAdminDataType;              //��16�ֽ� ��ҵ��






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
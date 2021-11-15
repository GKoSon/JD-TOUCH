#ifndef _FKDATA_HANDLE_H_
#define _FKDATA_HANDLE_H_

//富凯M1用户卡卡内存储信息格式 
typedef struct tagfukaiUserDataType{
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
    uint8_t doornum;            //门数  1
    uint8_t cardType;           //卡类  1
    uint8_t fdgs;               //分断  1
    uint8_t cost;               //费用  1
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
    uint8_t reserve[3];
}tagfukaiUserDataType;               //用户卡 共89字节

typedef struct _tagfukaiUnitAdminDataType{
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
    uint8_t type;                 //类型  
    uint8_t cardID[4];            //卡号  4字节

}tagfukaiUnitAdminDataType;              //共16字节 物业卡

uint8_t tag_fukai_card_process( tagBufferType *tag);

#endif


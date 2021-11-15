#ifndef _FKDATA_HANDLE_H_
#define _FKDATA_HANDLE_H_

//����M1�û������ڴ洢��Ϣ��ʽ 
typedef struct tagfukaiUserDataType{
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
    uint8_t doornum;            //����  1
    uint8_t cardType;           //����  1
    uint8_t fdgs;               //�ֶ�  1
    uint8_t cost;               //����  1
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
    uint8_t reserve[3];
}tagfukaiUserDataType;               //�û��� ��89�ֽ�

typedef struct _tagfukaiUnitAdminDataType{
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
    uint8_t type;                 //����  
    uint8_t cardID[4];            //����  4�ֽ�

}tagfukaiUnitAdminDataType;              //��16�ֽ� ��ҵ��

uint8_t tag_fukai_card_process( tagBufferType *tag);

#endif


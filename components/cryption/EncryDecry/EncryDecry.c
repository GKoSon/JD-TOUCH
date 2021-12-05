
#include "EncryDecry.h"
#include "string.h"

void makeKey(unsigned char* inKey,unsigned char outKey[][8]);
void initPermutation(unsigned char* inData);
void conversePermutation(unsigned char* inData);

const unsigned char keyConst[8]={0x19,0x86,0x10,0x29,0x20,0x12,0x03,0x03};//秘钥
const unsigned char keyNormal[8]={0x19,0x86,0x10,0x29,0x20,0x12,0x03,0x03};//秘钥

const unsigned char BitIP[64] =
{ 25, 17, 54, 33, 9, 38, 34, 1,
11, 48, 29, 56, 27, 50, 51, 40,
19, 58, 21, 5, 44, 31, 45, 7,
61, 47, 13, 57, 23, 15, 53, 46,
24, 16, 8, 39, 0, 26, 18, 10,
2, 49, 12, 4, 36, 30, 14, 6,
41, 59, 63, 22, 62, 32, 37, 42,
28, 20, 43, 52, 3, 35, 60, 55 };

const unsigned char BitCP[64] =
{ 36, 7, 40, 60, 43, 19, 47, 23,
34, 4, 39, 8, 42, 26, 46, 29,
33, 1, 38, 16, 57, 18, 51, 28,
32, 0, 37, 12, 56, 10, 45, 21,
53, 3, 6, 61, 44, 54, 5, 35,
15, 48, 55, 58, 20, 22, 31, 25,
9, 41, 13, 14, 59, 30, 2, 63,
11, 27, 17, 49, 62, 24, 52, 50 };

const unsigned char BitPMC[8][64]=
{
    { 56, 0, 53, 29, 17, 44, 24, 8, 20, 23, 43, 16, 7, 46, 36, 57, 2, 19, 42, 35, 32, 15, 31, 26, 54, 60, 33, 9, 38, 11, 61, 30, 10, 47,
    40, 5, 52, 25, 41, 27, 62, 63, 6, 58, 13, 21, 3, 28, 18, 49, 55, 59, 39, 51, 12, 37, 14, 1, 4, 34, 22, 45, 48, 50 },

    {63, 10, 47, 58, 39, 38, 51, 42, 23, 54, 3, 21, 14, 55,49, 29, 37, 28, 56, 40, 61, 43, 60, 18, 16, 57, 26, 9, 30, 34,
    11, 33, 1, 27, 53, 12, 36, 48, 52, 22, 46, 8, 45, 44, 59, 15, 5, 6, 13, 24, 35, 31, 2, 62, 41, 0, 4, 25, 50, 20, 7, 17, 32, 19 },

    {8, 5, 46, 4, 39, 44, 63, 52, 2, 54, 56, 62, 21, 32, 50,48, 20, 22, 47, 57, 60, 37, 12, 34, 9, 41, 27, 11, 6, 18, 33, 14,
    24, 31, 28, 55, 36, 23, 16, 40, 51, 25, 61, 43, 17, 3, 35, 53, 0, 7, 10, 58, 15, 1, 13, 19, 38, 45, 29, 42, 49, 26, 59, 30 },

    {26, 52, 25, 7, 48, 49, 56, 30, 27, 11, 22, 47, 8, 16,40, 10, 9, 24, 50, 62, 57, 44, 34, 14, 4, 55, 59, 5, 39, 23, 17, 58, 12,
    3, 63, 43, 6, 20, 51, 42, 45, 28, 31, 54, 53, 1, 41, 35, 13, 60, 21, 61, 19, 2, 46, 15, 36, 33, 18, 37, 0, 32, 38, 29 },

    { 42, 48, 16, 38, 41, 57, 53, 3, 52, 14, 61, 33, 26, 19, 32, 58, 10, 1, 9, 24, 43, 8, 15, 5, 56, 2, 40, 36, 7, 0, 17, 20, 45, 37, 6,
    13, 25, 34, 11, 27, 30, 12, 63, 31, 28, 47, 4, 51, 62, 22, 55, 44, 29, 35, 59, 23, 46, 50, 39, 60, 49, 18, 21, 54 },

    { 51, 44, 45, 12, 10, 19, 9, 57, 53, 0, 49, 8, 29, 7, 22, 36, 13, 58, 35, 15, 50, 23, 59, 52, 63, 4, 30, 43, 26, 33, 42, 1, 14, 24,
    55, 38, 5, 32, 48, 28, 21, 31, 17, 46, 41, 47, 60, 25, 20, 11, 61, 3, 6, 16, 2, 40, 39, 18, 62, 37, 34, 27, 56, 54 },

    { 19, 61, 20, 15, 0, 59, 60, 12, 10, 16, 35, 36, 34, 5, 27, 8, 43, 3, 54, 7, 57, 58, 26, 56, 13, 1, 23, 50, 11, 6, 25, 22, 31, 44,
    62, 53, 14, 33, 39, 48, 52, 2, 40, 41, 29, 17, 18, 4, 47, 28, 63, 24, 9, 32, 21, 46, 49, 30, 38, 37, 51, 45, 42, 55 },

    { 33, 24, 29, 28, 30, 51, 20, 25, 0, 57, 22, 34, 13, 44, 31, 17, 49, 16, 18, 50, 4, 48, 5, 38, 41, 12, 63, 26, 55, 37, 52, 60, 27, 9,
    21, 19, 45, 39, 54, 15, 53, 7, 43, 46, 62, 11, 14, 36, 56, 1, 10, 23, 42, 61, 8, 35, 40, 59, 47, 32, 2, 58, 6, 3 }

};


//unsigned char key[8]={0x19,0x86,0x10,0x29,0x20,0x12,0x03,0x03};//秘钥


//********************************************
//函数名:MAIN
//功能:
//入口参数:无
//出口参数:无
//********************************************

void makeKeyData(unsigned char *pkey ,unsigned char *pTable)
{
  unsigned char ucKey[3];
  unsigned char i=0;
  ucKey[0] = *(pTable+0);
  ucKey[1] = *(pTable+1);
  ucKey[2] = *(pTable+2);
  for(i=0;i<3;i++) pkey[i] = ucKey[i];
  for(i=0;i<3;i++) pkey[3+i] = ucKey[i];
  for(i=0;i<2;i++) pkey[6+i] = ucKey[i];
}

void EncryStr(unsigned char* str, unsigned char* key, unsigned char* presult)//加密函数8个字节
{
  unsigned char subkey[8][8];
  makeKey(key,subkey);
  initPermutation(str);
  desData(0,str,presult,subkey);
}

void DecryStr(unsigned char* str,unsigned char* key,unsigned char* presult)//解密函数8个字节
{
  unsigned char subkey[8][8];
  makeKey(key, subkey);
  desData(1,str,presult,subkey);
  conversePermutation(presult);
}

void desData(unsigned char desMode,unsigned char* inData, unsigned char* outData,unsigned char (*subkey)[8])
{
 
    int i;
    int j=0;
    if(desMode==0)/*加密*/
    {
        for(i=0;i<8;i++)
        {
            for(j=0;j<8;j++)
                inData[j]=inData[j]^ subkey[i][j];
        }
        for(j=0;j<8;j++)
            outData[j]=inData[j];
    }
    else if(desMode==1)/*解密*/
    {
        for(i=7;i>=0;i--)
            for(j=0;j<8;j++)
                inData[j]=inData[j]^ subkey[i][j];
        for(j=0;j<8;j++)
            outData[j]=inData[j];
    }
}

void makeKey(unsigned char* inKey,unsigned char outKey[][8])
{
  unsigned char newData[8]={0,0,0,0,0,0,0,0};
    int i,j,k;
    memset(outKey,0,sizeof(outKey));
    for(i=0;i<8;i++)
    {
        for(j=0;j<64;j++)
        {
            if((inKey[BitPMC[i][j]>>3]&(1<<(7-(BitPMC[i][j]&7))))!=0)
                newData[j>>3]=newData[j>>3]|(1<<(7-(j&7)));

        }
        for(k=0;k<8;k++)
            outKey[i][k] = newData[k];
        memset(newData,0,sizeof(newData));
    }
}

void initPermutation(unsigned char* inData)
{

	unsigned char newData[8]={0,0,0,0,0,0,0,0};
	int i;
	for(i=0;i<64;i++)
	{
		if((inData[BitIP[i]>>3]&(1<<(7-(BitIP[i]&7))))!=0)
			newData[i>>3]=newData[i>>3]|(1<<(7-(i&7)));
	}
	for(i=0;i<8;i++)
	{
		inData[i]=newData[i];
	}

	
}

void conversePermutation(unsigned char* inData)
{
    unsigned char newData[8]={0,0,0,0,0,0,0,0};
    int i;
    for(i=0;i<64;i++)
    {
        if((inData[BitCP[i]>>3]&(1<<(7-(BitCP[i]&7))))!=0)
            newData[i>>3]=newData[i>>3]|(1<<(7-(i&7)));
    }
    for(i=0;i<8;i++)
    {
        inData[i]=newData[i];
    }
}




#include "unit.h"

void Decryptionr(unsigned char* SourceData,unsigned char *Key,unsigned char* PurposeData)
{
  int i,j;
  unsigned char key[8]={0},tem[16]={0},tem2[16]={0};
  unsigned char uckey64[8][8],ucMw[8][8];
  /*维持原UID*/
  //for(i=0;i<8;i++) key[i]=Key[i];
  /*翻转*/
  //for(i=0;i<8;i++) key[i]=Key[7-i];
  /*翻转*/
  memcpy_up(tem,Key,8);
  for(i=0;i<16;i++) tem2[i]=tem[15-i];
  printf("Decryptionr key[%.16s]\r\n",tem2);
  memcpy_down(key,tem2,16);
  log_arry(DEBUG,"[CARD]key,UID" , key , 8);
  for(i=0;i<8;i++)
  {
     uckey64[0][i]=*(key+i);
  }
  for(i=0;i<8;i++)
  {
     uckey64[1][i]=*(key+7-i);
  }
  for(i=0;i<7;i++)
  {
     uckey64[2][i]=*(key+i+1);
  }
     uckey64[2][7]=*(key+0);
  for(i=0;i<6;i++)
  {
     uckey64[3][i]=*(key+i+2);
  }

  for(i=0;i<2;i++)
  {
     uckey64[3][6+i]=*(key+i);
  }
  for(i=0;i<5;i++)
  {
     uckey64[4][i]=*(key+i+3);
  }
  for(i=0;i<3;i++)
  {
     uckey64[4][5+i]=*(key+i);
  }
  for(i=0;i<4;i++)
  {
     uckey64[5][i]=*(key+i+4);
  }
  for(i=0;i<4;i++)
  {
     uckey64[5][4+i]=*(key+i);
  }
  for(i=0;i<3;i++)
  {
     uckey64[6][i]=*(key+i+5);
  }
  for(i=0;i<5;i++)
  {
     uckey64[6][3+i]=*(key+i);
  }
  for(i=0;i<2;i++)
  {
     uckey64[7][i]=*(key+i+6);
  }
  for(i=0;i<6;i++)
  {
     uckey64[7][2+i]=*(key+i);
  }
  
  for(j=0;j<8;j++)
  {
    for(i=0;i<8;i++)ucMw[j][i]=SourceData[j*8+i];
  }
  
  for(j=0;j<8;j++)DecryStr(ucMw[j],uckey64[j],ucMw[j]);
  for(j=0;j<8;j++)
  {
    for(i=0;i<8;i++)PurposeData[j*8+i]=ucMw[j][i];
  }
}


void Eecryptionr(unsigned char* SourceData,unsigned char *Key,unsigned char* PurposeData)
{
  int i,j;
  unsigned char key[8]={0},tem[16]={0},tem2[16]={0};
  unsigned char uckey64[8][8],ucMw[8][8];

  for(i=0;i<8;i++) key[i]=Key[7-i];

  log_arry(DEBUG,"[CARD]key,UID" , key , 8);
  for(i=0;i<8;i++)
  {
     uckey64[0][i]=*(key+i);
  }
  for(i=0;i<8;i++)
  {
     uckey64[1][i]=*(key+7-i);
  }
  for(i=0;i<7;i++)
  {
     uckey64[2][i]=*(key+i+1);
  }
     uckey64[2][7]=*(key+0);
  for(i=0;i<6;i++)
  {
     uckey64[3][i]=*(key+i+2);
  }

  for(i=0;i<2;i++)
  {
     uckey64[3][6+i]=*(key+i);
  }
  for(i=0;i<5;i++)
  {
     uckey64[4][i]=*(key+i+3);
  }
  for(i=0;i<3;i++)
  {
     uckey64[4][5+i]=*(key+i);
  }
  for(i=0;i<4;i++)
  {
     uckey64[5][i]=*(key+i+4);
  }
  for(i=0;i<4;i++)
  {
     uckey64[5][4+i]=*(key+i);
  }
  for(i=0;i<3;i++)
  {
     uckey64[6][i]=*(key+i+5);
  }
  for(i=0;i<5;i++)
  {
     uckey64[6][3+i]=*(key+i);
  }
  for(i=0;i<2;i++)
  {
     uckey64[7][i]=*(key+i+6);
  }
  for(i=0;i<6;i++)
  {
     uckey64[7][2+i]=*(key+i);
  }
  
  for(j=0;j<8;j++)
  {
    for(i=0;i<8;i++)ucMw[j][i]=SourceData[j*8+i];
  }
  
  for(j=0;j<8;j++)EncryStr(ucMw[j],uckey64[j],ucMw[j]);
  for(j=0;j<8;j++)
  {
    for(i=0;i<8;i++)PurposeData[j*8+i]=ucMw[j][i];
  }
}



void DEecryptionr_test(void)
{
/*一组数据*/
uint8_t A1[98]={
0x92,0x33,0x84,0xE0,0x5E,0xBB,0x2E,0xC8,0xCD,0xC9,
0xA1,0x71,0x97,0x04,0x21,0xF5,0x10,0x28,0x97,0xB3,
0x6E,0xE9,0xE9,0x6D,0x89,0x7F,0x53,0xC6,0x6A,0x21,
0x38,0x68,0x94,0xEA,0xD4,0x62,0x99,0x3B,0x17,0x97,
0x98,0xF3,0xE3,0x73,0xB2,0x2A,0x92,0x5C,0x5A,0x6B,
0xB1,0x83,0x60,0xCC,0xB2,0xE7,0xCF,0x5E,0xD1,0x74,
0xB6,0xCE,0xD2,0x63,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x40,0x97};

uint8_t A2[8]={0x57,0x74,0x8A,0xA8,0x06,0x23,0x02,0xE0};

log_arry(DEBUG,"A卡密文" ,A1 , 98 );
log_arry(DEBUG,"A2" ,A2 , 8 );

Decryptionr(A1,A2,A1);

log_arry(DEBUG,"AB卡公共明文" ,A1 , 98 );


uint8_t B2[8]={0xF1,0xD4,0x71,0x37,0x04,0x23,0x02,0xE0};
log_arry(DEBUG,"B2" ,B2 , 8 );
Eecryptionr(A1,B2,A1);

log_arry(DEBUG,"B卡密文" ,A1 , 98 );

Decryptionr(A1,B2,A1);

log_arry(DEBUG,"AB卡公共明文" ,A1 , 98 );

}
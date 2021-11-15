#include "hwConfig.h"
#include "mifare.h"
#include "Drv95HF.H"

#include "AnyID_Mifare_Auth.h"
#include "lib_ConfigManager.h"
#include <string.h>
#include "model.h"

ISO14443A_MIFAREAUTH MifareCard;
ST95HF_FRAME	MifareCommand;

uint8_t SelGuard = 0;
#if 1

#define POLYNOMIAL                      0x8408   //x^16 + x^12 + x^5 + 1
#define PRESET_VALUE                    0xFFFF
unsigned short Reader_CalCrc(unsigned char *pFrame, unsigned char len)
{
    unsigned short crc = 0;
    unsigned char i = 0, j = 0;
    crc = PRESET_VALUE;
    for(i = 0; i< len; i++)
    {
        crc = crc ^ pFrame[i];
        for(j = 0; j < 8; j++)
        {
            if(crc & 0x0001)
            {
                crc = (crc >> 1) ^ POLYNOMIAL;
            }
            else
            {
                crc = (crc >> 1);

            }
        }
    }
    crc = ~crc;
    return crc;
}

/*unsigned char ST95HF_WriteByte(unsigned char byte)
{
  return SPI_SendReceiveByte(byte);//SPI_SendReceiveByte(ST95HF_SPI, byte);
}*/
void ST95HF_WriteFrame(ST95HF_FRAME *pFrame)
{
    unsigned char i = 0;

	 nSpi->ChipSelet(DISABLE);
    //ST95HF_CS_Low();

    //send control
    nSpi->SendReceiveByte(ST95HF_COMM_CMD_WRITE);

    //数据
    nSpi->SendReceiveByte(pFrame->parms.cmd);
    nSpi->SendReceiveByte(pFrame->len);
    for(i = 0; i < pFrame->len; i++)
    {
        nSpi->SendReceiveByte(pFrame->frame[i]);
    }
	 nSpi->ChipSelet(ENABLE);
    //ST95HF_CS_High();//(ST95HF_CS.Port)->BSRR = ST95HF_CS.Pin;
}

unsigned char ST95HF_Polling(unsigned char flag, unsigned long timeout)
{
    unsigned char reg = 0;
    unsigned long delay = 0;
    unsigned char rlt;

    flag = flag & (ST95HF_POLLING_WRITE_FLAG | ST95HF_POLLING_READ_FLAG);

    timeout = timeout >> 3; //每次需要10us时间
    if(timeout == 0)
    {
        timeout = 2;
    }

    //在ISO14443A协议中，有一个FDT设定的时间，应用程序延时的时间timeout一定要大于FDT值
    //所以这里使用延时时间大于10,但是计算还是用10us计算
    //就可以保证FDT<timeout
    nSpi->ChipSelet(DISABLE);//ST95HF_CS_Low();
    while(((reg & flag) == 0x00) && (delay < timeout))//
    {
        reg = nSpi->SendReceiveByte(ST95HF_COMM_CMD_POLLING);        //如果用1.1M的时钟进行通信，该部分消耗10us时间不到
        delay++;
        //ST95HF_SpecDelay10us();                                 //这里补充延时使延时的时间超过10us一点，大约是12.5us
    }
	 nSpi->ChipSelet(ENABLE);
    //ST95HF_CS_High();//(ST95HF_CS.Port)->BSRR = ST95HF_CS.Pin;

    if(delay == timeout)
    {
        rlt = ST95HF_ERR_TIMEOUT;
    }
    else
    {
        rlt = ST95HF_ERR_SUCCESS;
    }

    return rlt;
}

unsigned char ST95HF_ReadFrame(ST95HF_FRAME *pFrame)
{
    unsigned char i = 0;
    unsigned char state = 0;

    state = ST95HF_ERR_SUCCESS;

	 nSpi->ChipSelet(DISABLE);
    //ST95HF_CS_Low();

    nSpi->SendReceiveByte(ST95HF_COMM_CMD_READ);
    pFrame->parms.code = nSpi->SendReceiveByte(ST95HF_DUMMY_BYTE);
    pFrame->len = nSpi->SendReceiveByte(ST95HF_DUMMY_BYTE);
    if(pFrame->len > ST95HF_FRAME_LEN)
    {
        state = ST95HF_ERR_LENGTH;
    }
    else
    {
        for(i = 0; i < pFrame->len; i++)
        {
            pFrame->frame[i] = nSpi->SendReceiveByte(ST95HF_DUMMY_BYTE);
        }
    }
	 nSpi->ChipSelet(ENABLE);
    //ST95HF_CS_High();//(ST95HF_CS.Port)->BSRR = ST95HF_CS.Pin;

    return state;
}

unsigned char ST95HF_Command(ST95HF_FRAME *pFrame)
{
    unsigned char err = 0x00;

    err = ST95HF_Polling(ST95HF_POLLING_READ_FLAG, 0);
    if(err == ST95HF_ERR_SUCCESS)
    {
        //读取上次没有读取的遗留数据，例如上次出错了
        unsigned char len = 0;
        unsigned char i = 0;

		  nSpi->ChipSelet(DISABLE);
        //ST95HF_CS_Low();//(ST95HF_CS.Port)->BRR = ST95HF_CS.Pin
        
        nSpi->SendReceiveByte(ST95HF_COMM_CMD_READ);
        nSpi->SendReceiveByte(ST95HF_DUMMY_BYTE);// rspcode
        len = nSpi->SendReceiveByte(ST95HF_DUMMY_BYTE);
        for(i = 0; i < len; i++)
        {
            nSpi->SendReceiveByte(ST95HF_DUMMY_BYTE);
        }
		  nSpi->ChipSelet(ENABLE);
        //ST95HF_CS_High();//(ST95HF_CS.Port)->BSRR = ST95HF_CS.Pin;
    }

    err = ST95HF_Polling(ST95HF_POLLING_WRITE_FLAG, ISO14443A_TO_TIME);
    if(err == ST95HF_ERR_SUCCESS)
    {
        ST95HF_WriteFrame(pFrame );

        err = ST95HF_Polling(ST95HF_POLLING_READ_FLAG, ISO14443A_TO_TIME);
        if(err == ST95HF_ERR_SUCCESS)
        {
            err = ST95HF_ReadFrame(pFrame );
            if(err != ST95HF_ERR_SUCCESS)
            {
                err = pFrame->parms.code;
            }
        }
    }

    return err;
}

///////////////////////Mifare  M1卡读卡///////////////////////////
#define ISO14443A_PRESET_VALUE      0x6363
#define ISO14443A_POLYNOMIAL        0x8408   //x^16 + x^12 + x^5 + 1
unsigned short ISO14443A_CalCrc(unsigned short *pFrame, unsigned char len)
{
    unsigned short crc = 0;     
    unsigned char i = 0, j = 0;
    unsigned char temp = 0;
    crc = ISO14443A_PRESET_VALUE;    
    for(i = 0; i < len; i++)
    {
        temp = pFrame[i] & 0xFF;
        crc = crc ^ temp;
        for(j = 0; j < 8; j++)
        {
            if(crc & 0x0001)
            {
                crc = (crc >> 1) ^ ISO14443A_POLYNOMIAL;
            }
            else
            {
                crc = (crc >> 1);
            }    
        }        
    }    
    return crc;
}
BOOL ISO14443A_CheckCrc(unsigned short *pFrame, unsigned char len)
{
    unsigned short crc1 = 0, crc2 = 0;
    BOOL b = FALSE;
    if(len > 2)
    {
        crc1 = ISO14443A_CalCrc(pFrame, len - 2);
        crc2 = pFrame[len - 2] & 0xFF;
        crc2 += ((pFrame[len - 1] << 8) & 0xFF00); 

        if(crc1 == crc2)
        {
            b = TRUE;
        }
    }

    return b;
}

unsigned short ISO14443A_AddOddBit(unsigned char dat)
{
    unsigned char odd = 0;
    unsigned short temp = 0;
    unsigned char i = 0;

    temp = dat & 0xFF;
    for(i = 0; i < 8; i++)
    {
        odd ^= (temp & 0x01);
        temp >>= 1;
    }
    temp = dat & 0xFF;
    temp |= (((~odd) & 0x01) << 8);
    return temp;
}
BOOL ISO14443A_CheckParity(unsigned short *pFrame, unsigned char len)
{
    BOOL b = TRUE;
    unsigned char i = 0, j = 0;
    unsigned char odd = 0;
    unsigned short dat = 0;
    for(i = 0; i < len; i++)
    {
        odd = 0;
        dat = pFrame[i] & 0x1FF;
        for(j = 0; j < 9; j++)
        {
            odd ^= (dat & 0x01);
            dat >>= 1;
        }
        if((odd & 0x01) == 0x00)
        {
            b = FALSE;
            break;
        }
    }
    return b;
}

//pUid  卡号  authMode 0x60   pKey=0xff,0xff,0xff,0xff,0xff,0xff     blockAddr=0x01(16个字节=32个字符)//0x02//
unsigned char ISO14443A_AuthM1(unsigned char *pUID, unsigned char authMode, unsigned char *pKey, unsigned char blockAddr)//Auth==认证
{
    unsigned char state = ST95HF_ERR_SUCCESS;
    unsigned char txLen = 0;
    unsigned long iv = 0;
    unsigned short crc = 0;
    int i = 0;
    unsigned short frame[18] = {0};
    //三重认证第一步：请求认证，获取随机数
    txLen = 0;
    frame[txLen++] = authMode;
    frame[txLen++] = blockAddr;
    crc = ISO14443A_CalCrc(frame, txLen);
    frame[txLen++] = (crc >> 0) & 0xFF;
    frame[txLen++] = (crc >> 8) & 0xFF;

    for(i = 0; i < txLen; i++)
    {
        frame[i] = ISO14443A_AddOddBit(frame[i]);
    }
    if(MifareCard.crypto1On == 1)
    {
        Mifare_Cipher(frame, txLen, 0 );
    }
    MifareCommand.parms.cmd = ST95HF_CMD_SEND_RECEIVE;
    //MifareCommand.parms.cmd = ST95HF_CMD_SEND_RECEIVE;
    for(i = 0; i < txLen; i++)
    {
        MifareCommand.frame[(i << 1) + 0] = (frame[i] >> 0) & 0xFF;
        MifareCommand.frame[(i << 1) + 1] = (frame[i] >> 1) & 0x80;
    }
    txLen = txLen << 1;
    MifareCommand.frame[txLen++] = ST95HF_SENDRCV_14443A_FLG_PARITY_MOD | ST95HF_SENDRCV_14443A_FLG_LSB_BIT8;  //标准帧 + 主机处理校验位
    
    //MifareCommand.to = ISO14443A_TO_TIME;
    MifareCommand.len = txLen;
    state = ST95HF_Command(&MifareCommand );
    
    if(state == ST95HF_ERR_SUCCESS)
    {
        if(MifareCommand.parms.code == ST95HF_SENDRCV_RSPCODE_DAT)
        {
            MifareCard.lfsr = *((unsigned long long *)pKey);
            MifareCard.uid = ARRAY_TO_UINT32(pUID);
            MifareCard.nr = 0xFFFFFFFF;   
            
            for(i = 0; i < 4; i++)
            {
                frame[i] = MifareCommand.frame[i * 2 + 1] & 0x80;
                frame[i] <<= 1;
                frame[i] |= (MifareCommand.frame[i * 2 + 0] & 0xFF);
            }
            //获取随机数
             MifareCard.nt = ARRAY_TO_UINT32(frame);
            if( MifareCard.crypto1On == 1)
            {
                //如果是嵌套认证，需要解密一次
                iv =  MifareCard.uid ^  MifareCard.nt;
                 MifareCard.nt =  MifareCard.nt ^ MIFARE_update_word(iv, 1 );
            }
        }
        else if(MifareCommand.parms.code == ST95HF_SENDRCV_RSPCODE_ERR)
        {
            state = ST95HF_ERR_RSPCODE;
        }
        else
        {
            state = MifareCommand.parms.code;
        }
    }

    if( MifareCard.crypto1On == 0) 
    {
        //初始化编码器和ks1
        iv =  MifareCard.uid ^  MifareCard.nt;
        MIFARE_update_word(iv, 0 );
    }
    //nr xor ks1，并生成ks2
    UINT32_TO_ARRAY_WITH_PARITY( MifareCard.nr, frame);
    for(i = 3; i >= 0; i--)
    {                           /* Same as in MIFARE_update_word, but with added parity */
        frame[3 - i] = frame[3 - i] ^ MIFARE_update_byte(( MifareCard.nr >> (i * 8)) & 0xff, 0 );
        frame[3 - i] ^= mf20( MifareCard.lfsr) << 8;
    } 

    //suc2(nt) xor ks2，并生成ks3，以后的编码和解码都用ks3完成
     MifareCard.ar = prng_next(64 );
    UINT32_TO_ARRAY_WITH_PARITY( MifareCard.ar, frame + 4);
    for(i = 0; i < 4; i++)
    {
        frame[i + 4] = frame[i + 4] ^ MIFARE_update_byte(0, 0 );
        frame[i + 4] = frame[i + 4] ^ (mf20( MifareCard.lfsr) << 8);
    }

    txLen = 0;
    MifareCommand.parms.cmd = ST95HF_CMD_SEND_RECEIVE;
    for(i = 0; i < 8; i++)
    {
        MifareCommand.frame[txLen++] = frame[i] & 0xFF;
        MifareCommand.frame[txLen++] = (frame[i] >> 1) & 0x80;
    }
    MifareCommand.frame[txLen++] = ST95HF_SENDRCV_14443A_FLG_PARITY_MOD | ST95HF_SENDRCV_14443A_FLG_LSB_BIT8;  //标准帧 + crc
    
//    MifareCommand.to = ISO14443A_TO_TIME;
    MifareCommand.len = txLen;
    state = ST95HF_Command(&MifareCommand );
    
    if(state == ST95HF_ERR_SUCCESS)
    {
        if(MifareCommand.parms.code == ST95HF_SENDRCV_RSPCODE_DAT)
        {
            unsigned long ta1 = 0, ta2 = 0;
            for(i = 0; i < 4; i++)
            {
                frame[i] = MifareCommand.frame[i * 2 + 1] & 0x80;
                frame[i] <<= 1;
                frame[i] |= (MifareCommand.frame[i * 2 + 0] & 0xFF);
            }
            ta1 = ARRAY_TO_UINT32(frame);
            ta2 = prng_next(96 ) ^ MIFARE_update_word(0, 0 );
            if(ta1 == ta2)
            {
                 MifareCard.crypto1On = 1;
            }
            else
            {
                 MifareCard.crypto1On = 0;
            }
        }
        else if(MifareCommand.parms.code == ST95HF_SENDRCV_RSPCODE_ERR)
        {
            state = ST95HF_ERR_RSPCODE;
        }
        else
        {
            state = MifareCommand.parms.code;
        }
    }    

    return state;
}
unsigned char ISO14443A_ReadMifareBlock(unsigned char blockAddr, unsigned char *pBlock )
{
    unsigned char state = ST95HF_ERR_SUCCESS;
    unsigned char txLen = 0;
    unsigned short frame[18] = {0};
    unsigned short crc = 0;
    unsigned char i = 0;
    
    frame[txLen++] = ISO14443A_AddOddBit(ISO14443A_CMD_READ);
    frame[txLen++] = ISO14443A_AddOddBit(blockAddr);
    crc = ISO14443A_CalCrc(frame, txLen);
    frame[txLen++] = ISO14443A_AddOddBit((crc >> 0) & 0xFF);
    frame[txLen++] = ISO14443A_AddOddBit((crc >> 8) & 0xFF);
    
    if( MifareCard.crypto1On == 1)
    {
        Mifare_Cipher(frame, txLen, 0 );
    }
    
    MifareCommand.parms.cmd = ST95HF_CMD_SEND_RECEIVE; 
    for(i = 0; i < txLen; i++)
    {
        MifareCommand.frame[(i << 1) + 0] = (frame[i] >> 0) & 0xFF;
        MifareCommand.frame[(i << 1) + 1] = (frame[i] >> 1) & 0x80;
    }
    txLen = txLen << 1;
    MifareCommand.frame[txLen++] = ST95HF_SENDRCV_14443A_FLG_PARITY_MOD | ST95HF_SENDRCV_14443A_FLG_LSB_BIT8;  
    
//    MifareCommand.to = ISO14443A_TO_TIME;
    MifareCommand.len = txLen;
    state = ST95HF_Command(&MifareCommand );
    if(state == ST95HF_ERR_SUCCESS)
    {
        if(MifareCommand.parms.code == ST95HF_SENDRCV_RSPCODE_DAT)
        {
            for(i = 0; i < ISO14443A_M1BLOCK_LEN + 2; i++)
            {
                frame[i] = MifareCommand.frame[i * 2 + 1] & 0x80;
                frame[i] <<= 1;
                frame[i] |= (MifareCommand.frame[i * 2 + 0] & 0xFF);
            }
            if( MifareCard.crypto1On == 1)
            {
                Mifare_Cipher(frame, ISO14443A_M1BLOCK_LEN + 2, 0 );
            }
            if(ISO14443A_CheckParity(frame, ISO14443A_M1BLOCK_LEN + 2))
            {
                if(ISO14443A_CheckCrc(frame, ISO14443A_M1BLOCK_LEN + 2))
                {
                    for(i = 0; i < ISO14443A_M1BLOCK_LEN; i++)
                    {
                        pBlock[i] = frame[i] & 0xFF;
                    }
                }
                else
                {
                    state = ST95HF_ERR_CRC;
                }
            }
            else
            {
                state = ST95HF_ERR_PARITY;
            }
        }
        else if(MifareCommand.parms.code == ST95HF_SENDRCV_RSPCODE_BIT)
        {
            frame[0] = MifareCommand.frame[0] & 0x0F;
            if( MifareCard.crypto1On == 1)
            {
                Mifare_Cipher(frame, 1, ISO14443A_ACK_LEN );
            }
            state = ST95HF_ERR_NAK;
        }
        else if(MifareCommand.parms.code == ST95HF_SENDRCV_RSPCODE_ERR)
        {
            state = ST95HF_ERR_RSPCODE;
        }
        else
        {
            state = MifareCommand.parms.code;
        }
    }
    return state;
}

unsigned char ISO14443A_WriteMifareBlock16(unsigned char blockAddr, unsigned char *pBlock )
{
    unsigned char state = ST95HF_ERR_SUCCESS;
    unsigned char txLen = 0;
    unsigned short frame[18] = {0};
    unsigned short crc = 0;
    unsigned char i = 0;
    
    frame[txLen++] = ISO14443A_AddOddBit(ISO14443A_CMD_WRITE16);
    frame[txLen++] = ISO14443A_AddOddBit(blockAddr);
    crc = ISO14443A_CalCrc(frame, txLen);
    frame[txLen++] = ISO14443A_AddOddBit((crc >> 0) & 0xFF);
    frame[txLen++] = ISO14443A_AddOddBit((crc >> 8) & 0xFF);
    
    if( MifareCard.crypto1On == 1)
    {
        Mifare_Cipher(frame, txLen, 0 );
    }
    
    MifareCommand.parms.cmd = ST95HF_CMD_SEND_RECEIVE; 
    for(i = 0; i < txLen; i++)
    {
        MifareCommand.frame[(i << 1) + 0] = (frame[i] >> 0) & 0xFF;
        MifareCommand.frame[(i << 1) + 1] = (frame[i] >> 1) & 0x80;
    }
    txLen = txLen << 1;
    MifareCommand.frame[txLen++] = ST95HF_SENDRCV_14443A_FLG_PARITY_MOD | ST95HF_SENDRCV_14443A_FLG_LSB_BIT8;  
    
//    MifareCommand.to = ISO14443A_TO_TIME;
    MifareCommand.len = txLen;
    state = ST95HF_Command(&MifareCommand );
    if(state == ST95HF_ERR_SUCCESS)
    {
        if(MifareCommand.parms.code == ST95HF_SENDRCV_RSPCODE_BIT)
        {
            frame[0] = MifareCommand.frame[0] & 0x0F;
            if( MifareCard.crypto1On == 1)
            {
                Mifare_Cipher(frame, 1, ISO14443A_ACK_LEN );
            }
            if((frame[0] & ISO14443A_ACK_MASK) == ISO14443A_ACK_OK)
            {
                txLen = 0;
                for(i = 0; i < ISO14443A_M1BLOCK_LEN; i++)
                {
                    frame[txLen++] = ISO14443A_AddOddBit(pBlock[i]);  
                }
                crc = ISO14443A_CalCrc(frame, txLen);
                frame[txLen++] = ISO14443A_AddOddBit((crc >> 0) & 0xFF);
                frame[txLen++] = ISO14443A_AddOddBit((crc >> 8) & 0xFF);
                
                if( MifareCard.crypto1On == 1)
                {
                    Mifare_Cipher(frame, txLen, 0 );
                }
                
                MifareCommand.parms.cmd = ST95HF_CMD_SEND_RECEIVE; 
                for(i = 0; i < txLen; i++)
                {
                    MifareCommand.frame[(i << 1) + 0] = (frame[i] >> 0) & 0xFF;
                    MifareCommand.frame[(i << 1) + 1] = (frame[i] >> 1) & 0x80;
                }
                txLen = txLen << 1;
                MifareCommand.frame[txLen++] = ST95HF_SENDRCV_14443A_FLG_PARITY_MOD | ST95HF_SENDRCV_14443A_FLG_LSB_BIT8;  
                
//                MifareCommand.to = ISO14443A_TO_TIME;
                MifareCommand.len = txLen;
                state = ST95HF_Command(&MifareCommand );
                if(state == ST95HF_ERR_SUCCESS)
                {
                    if(MifareCommand.parms.code == ST95HF_SENDRCV_RSPCODE_BIT)
                    {
                        frame[0] = MifareCommand.frame[0] & 0x0F;
                        if( MifareCard.crypto1On == 1)
                        {
                            Mifare_Cipher(frame, 1, ISO14443A_ACK_LEN );
                        }
                        if((frame[0] & ISO14443A_ACK_MASK) != ISO14443A_ACK_OK)
                        {
                            state = ST95HF_ERR_NAK;
                        }
                    }
                    else if(MifareCommand.parms.code == ST95HF_SENDRCV_RSPCODE_ERR)
                    {
                        state = ST95HF_ERR_RSPCODE;
                    }
                    else
                    {
                        state = MifareCommand.parms.code;
                    }
                }
            }
        }
        else if(MifareCommand.parms.code == ST95HF_SENDRCV_RSPCODE_ERR)
        {
            state = ST95HF_ERR_RSPCODE;
        }
        else
        {
            state = MifareCommand.parms.code;
        }
    }
    return state;
}

unsigned char ISO14443A_Init_r322(unsigned long fdt )
{
    unsigned char state = ST95HF_ERR_SUCCESS;
    
    MifareCommand.parms.cmd = ST95HF_CMD_SEL_PROTOCOL;            //选择协议命令0x02
    MifareCommand.len = 4; //protocol datarate              //选择协议命令长度
    
    MifareCommand.frame[0] = ST95HF_PROTOCOL_ISO14443A;   //14443A
    MifareCommand.frame[1] = ST95HF_ISO14443A_TXRATE_106 | ST95HF_ISO14443A_RXRATE_106;             //设置发送和接收的波特率
    //FWT = (2^PP)*(MM+1)*(DD+128)*32/13.56 μs
    //fwt = (2^2)*(0+1)*(0+128)*32/13.56 = 1.208ms
    MifareCommand.frame[2] = 0x02; //pp
    MifareCommand.frame[3] = 0x00; //mm
    
    fdt /= 1208;
    while(fdt > 255)
    {
        fdt >>= 1;
        MifareCommand.frame[2] += 1; //增加2倍
    }
    MifareCommand.frame[3] = fdt;
    
//    MifareCommand.to = ST95HF_POLLING_DELAY;
    state = ST95HF_Command(&MifareCommand );
    if(state == ST95HF_ERR_SUCCESS)
    {
        if(MifareCommand.parms.code != ST95HF_RSP_CODE_OK)
        {
            state = MifareCommand.parms.code;
        }
    }
    
    return state;
}

unsigned char ReadMifareBlockKey11(unsigned char blockAddr,unsigned char* pR,unsigned char uLen ,uint8_t *TagUID)
{
	uint8_t keyType=0x60;//keyA ?? 
  //uint8_t tempBuffer[16] = {0};
//#if (USE_DEBUG)  
  //uint8_t tempBufferStr[33] = {0};
//#endif
  uint8_t key[6] = {0x11,0x11,0x11,0x11,0x11,0x11};
//  uint8_t key[6] = {0xff,0xff,0xff,0xff,0xff,0xff};
  uint8_t state = ST95HF_ERR_SUCCESS;
  uint8_t uId[10];
  uint8_t i;
  
  
  if(SelGuard == 1)//家庭锁
  {
        key[0]=0x17;key[1]=0x78;key[2]=0x36;key[3]=0x13;key[4]=0x68;key[5]=0x61;
  }
  memset(uId,0,10);
  for(i=0;i<4;i++)
  {
        uId[i]=TagUID[i];
  }

  ISO14443A_Init_r322(ISO14443A_FDT_READ );
  state = ISO14443A_AuthM1(uId, keyType, key, blockAddr );
  
  if(state == ST95HF_ERR_SUCCESS)
  {
      state = ISO14443A_ReadMifareBlock(blockAddr, pR );
      return state;
      /*if(state == ST95HF_ERR_SUCCESS)
      {
        //for(i=0;i<uLen;i++)  *(pR++)=tempBuffer[i];
        return 0;
      }*/
  }
  return 1;//g_sReaderRspFrame.len;
}

unsigned char ReadMifareBlockKeyFF(unsigned char blockAddr,unsigned char* pR,unsigned char uLen,uint8_t *TagUID )
{
  /*unsigned char key[6] = {0xff,0xff,0xff,0xff,0xff,0xfF};
  return subR25611(blockAddr,pR,uLen,key);*/
	uint8_t keyType=0x60;//keyA ?? 
  //uint8_t tempBuffer[16] = {0};
//  uint8_t key[6] = {0x11,0x11,0x11,0x11,0x11,0x11};
  uint8_t key[6] = {0xff,0xff,0xff,0xff,0xff,0xff};
  uint8_t state = ST95HF_ERR_SUCCESS;
  uint8_t uId[10];
  uint8_t i;
  memset(uId,0,10);
  for(i=0;i<4;i++)uId[i]=TagUID[i];
  
  ISO14443A_Init_r322(ISO14443A_FDT_READ );
  state = ISO14443A_AuthM1(uId, keyType, key, blockAddr );
  
  if(state == ST95HF_ERR_SUCCESS)
  {
      state = ISO14443A_ReadMifareBlock(blockAddr, pR );
      
      /*if(state == ST95HF_ERR_SUCCESS)
      {
        for(i=0;i<uLen;i++)  *(pR++)=tempBuffer[i];
        
        return 0;
      }*/
  }

  return state;
}

unsigned char ST95HF_Iso14443BCheckResponse(ST95HF_FRAME *pFrame)
{
    unsigned char flag = 0;
    unsigned char state = ST95HF_ERR_SUCCESS;

    if(pFrame->parms.code == ST95HF_SENDRCV_RSPCODE_DAT)
    {
        if(pFrame->len > 0)
        {
            //最后一个字节表示接收的数据帧是否有效
            flag = pFrame->frame[pFrame->len - 1];
            //冲突位有点问题，有时候数据正确，但是现实冲突位
            if(flag & ST95HF_SENDRCV_14443B_ERR_CRC)
            {
                state = ST95HF_ERR_CRC;
            }
        }
        else
        {
            state = ST95HF_ERR_LENGTH;
        }
    }
    else if(pFrame->parms.code == ST95HF_SENDRCV_RSPCODE_ERR)
    {
        state = ST95HF_ERR_RSPCODE;
    }
    else
    {
        state = pFrame->parms.code;
    }

    return state;
}


uint8_t WriteMifareBlockKey11(uint8_t blockAddr,uint8_t* pW,uint8_t uLen,uint8_t *TagUID )
{
	uint8_t keyType=0x60;
	uint8_t tempBuffer[16] = {0};
	uint8_t key[6] = {0x11,0x11,0x11,0x11,0x11,0x11};
	//uint8_t key[6] = {0xff,0xff,0xff,0xff,0xff,0xff};
	uint8_t state = ST95HF_ERR_SUCCESS;
	uint8_t uId[4];
	uint8_t i;
        if(SelGuard == 1)
        {
          key[0]=0x17;key[1]=0x78;key[2]=0x36;key[3]=0x13;key[4]=0x68;key[5]=0x61;
        }
	for(i=0;i<4;i++)uId[i]=TagUID[i];
	memcpy(tempBuffer, pW, uLen);
	if(uLen<16)memset(&tempBuffer[uLen], 0xff, ISO14443A_M1BLOCK_LEN-uLen);

	ISO14443A_Init_r322(ISO14443A_FDT_WRITE );
	state = ISO14443A_AuthM1(uId, keyType, key, blockAddr );

	if(state == ST95HF_ERR_SUCCESS)
	{
		state = ISO14443A_WriteMifareBlock16(blockAddr, tempBuffer );

		if(state == ST95HF_ERR_SUCCESS)
		{
			return 0;
		}
	}
	return 1;//g_sReaderRspFrame.len;
}


uint8_t WriteMifareBlockKeyFF(uint8_t blockAddr ,uint8_t *TagUID )
{
	uint8_t keyType=0x60;
	uint8_t tempBuffer[16] = {0x11,0x11,0x11,0x11,0x11,0x11,
		0xff,0x07,0x80,0x69,
		TagUID[0],TagUID[1],TagUID[2],TagUID[3],0xff,0xff};
        if(SelGuard==1)//家庭锁
        {
           tempBuffer[0] = 0x17;tempBuffer[1] = 0x78;tempBuffer[2] = 0x36;tempBuffer[3] = 0x13;tempBuffer[4] = 0x68;tempBuffer[5] = 0x61;
        }          
	uint8_t key[6] = {0xff,0xff,0xff,0xff,0xff,0xff};
	uint8_t state = ST95HF_ERR_SUCCESS;
	uint8_t uId[10];
	uint8_t i;
	memset(uId,0,10);
	for(i=0;i<4;i++)
	{
		uId[i]=TagUID[i];
		tempBuffer[10+i] = TagUID[i];
	}

	ISO14443A_Init_r322(ISO14443A_FDT_WRITE );
	state = ISO14443A_AuthM1(uId, keyType, key, (blockAddr/4)*4 +3 );

	if(state == ST95HF_ERR_SUCCESS)
	{
		state = ISO14443A_WriteMifareBlock16((blockAddr/4)*4 +3, tempBuffer );
		if(state == ST95HF_ERR_SUCCESS)
		{
			return 0;
		}
	}
	return 1;//g_sReaderRspFrame.len;
}


unsigned char Write14443A_Block(uint8_t block, uint8_t* uW256 ,tagBufferType *tag )
{
  uint8_t uD[16];
  if(ReadMifareBlockKey11(block,uD,16 ,tag->UID)!=0)
  {
      TagHuntingResetM1(tag);
    if(ReadMifareBlockKeyFF(block,uD,16,tag->UID)!=0)return 1;
    if(WriteMifareBlockKeyFF(block ,tag->UID)!=0)return 1;
  }
  
  if((WriteMifareBlockKey11(block,uW256,16,tag->UID)!=0))return 1;
      return 0;
}

/******************************************************************************
*函数名 Read14443A_Block
*函数功能描述 ： 读取富凯卡片存储块信息
*函数参数1 ：block 块序号
*函数参数2 ：ubuff 数据缓存
*函数参数3 ：tag 卡片信息
*函数返回值 ：读取结果
*作者 ： 汪林海
*函数创建日期 ：2017年12月27日
*函数修改日期 ：无
*修改人 ： 无
*修改原因 ： 无
*版本 ：1.0
*历史版本 ：无
*******************************************************************************/
unsigned char Read14443A_Block(uint8_t block, uint8_t* ubuff ,tagBufferType *tag )
{
  //uint8_t uD[16];
  
  //MifareCard.crypto1On = 0;
  
  if(ReadMifareBlockKey11(block,ubuff,16 ,tag->UID)!=0)
  {
      TagHuntingResetM1(tag);
    if(ReadMifareBlockKeyFF(block,ubuff,16,tag->UID)!=0)return 1;
  }

  return 0;
}


/******************************************************************************
*函数名 read_fukai_card_data
*函数功能描述 ： 读取富凯卡片存储信息
*函数参数1 ：ubuff 数据存储缓存
*函数参数2 ：tag 卡片信息
*函数返回值 ：读取结果
*作者 ： 汪林海
*函数创建日期 ：2017年12月27日
*函数修改日期 ：无
*修改人 ： 无
*修改原因 ： 无
*版本 ：1.0
*历史版本 ：无
*******************************************************************************/
unsigned char read_fukai_card_data(uint8_t* ubuff ,tagBufferType *tag )
{
  uint8_t udata[16];
  
  MifareCard.crypto1On = 0;
  if(Read14443A_Block(1,udata,tag) != 0)     //读取第1块数据
  return false;  
  memcpy(&ubuff[0], udata, 16);
  
  if(Read14443A_Block(2,udata,tag) != 0)    //读取第2块数据       
  return false; 
  memcpy(&ubuff[16], udata, 16);
  
  if(Read14443A_Block(4,udata,tag) != 0)   //读取第4块数据  
  return false; 
  memcpy(&ubuff[32], udata, 9);
  
  if(Read14443A_Block(8,udata,tag) != 0)   //读取第8块数据
  return false; 
  memcpy(&ubuff[41], udata, 16);                
  
  if(Read14443A_Block(9,udata,tag) != 0)   //读取第9块数据
  return false; 
  memcpy(&ubuff[57], udata, 16);
  
  if(Read14443A_Block(10,udata,tag) != 0)  //读取第10块数据
  return false; 
  memcpy(&ubuff[73], udata, 16);
  
  return true;
}
#endif



unsigned char ISO14443B_GetUID(unsigned char *pUid)
{
    unsigned char state = ST95HF_ERR_SUCCESS;
	
    
    MifareCommand.parms.cmd = ST95HF_CMD_SEND_RECEIVE; 
    MifareCommand.len = 0;
    
    MifareCommand.frame[ MifareCommand.len++] = 0x00;  //apn
    MifareCommand.frame[ MifareCommand.len++] = 0x36;
    MifareCommand.frame[ MifareCommand.len++] = 0x00;
    MifareCommand.frame[ MifareCommand.len++] = 0x00;
    MifareCommand.frame[ MifareCommand.len++] = 0x08;
    
    state = ST95HF_Command( &MifareCommand);

    if(state == ST95HF_ERR_SUCCESS)
    {
        state = ST95HF_Iso14443BCheckResponse( &MifareCommand);
        if(state == ST95HF_ERR_SUCCESS)
        {
            memcpy(pUid,  MifareCommand.frame, ISO14443B_SIZE_UID);
        }
    }
    return state;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

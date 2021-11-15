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

    //����
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

    timeout = timeout >> 3; //ÿ����Ҫ10usʱ��
    if(timeout == 0)
    {
        timeout = 2;
    }

    //��ISO14443AЭ���У���һ��FDT�趨��ʱ�䣬Ӧ�ó�����ʱ��ʱ��timeoutһ��Ҫ����FDTֵ
    //��������ʹ����ʱʱ�����10,���Ǽ��㻹����10us����
    //�Ϳ��Ա�֤FDT<timeout
    nSpi->ChipSelet(DISABLE);//ST95HF_CS_Low();
    while(((reg & flag) == 0x00) && (delay < timeout))//
    {
        reg = nSpi->SendReceiveByte(ST95HF_COMM_CMD_POLLING);        //�����1.1M��ʱ�ӽ���ͨ�ţ��ò�������10usʱ�䲻��
        delay++;
        //ST95HF_SpecDelay10us();                                 //���ﲹ����ʱʹ��ʱ��ʱ�䳬��10usһ�㣬��Լ��12.5us
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
        //��ȡ�ϴ�û�ж�ȡ���������ݣ������ϴγ�����
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

///////////////////////Mifare  M1������///////////////////////////
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

//pUid  ����  authMode 0x60   pKey=0xff,0xff,0xff,0xff,0xff,0xff     blockAddr=0x01(16���ֽ�=32���ַ�)//0x02//
unsigned char ISO14443A_AuthM1(unsigned char *pUID, unsigned char authMode, unsigned char *pKey, unsigned char blockAddr)//Auth==��֤
{
    unsigned char state = ST95HF_ERR_SUCCESS;
    unsigned char txLen = 0;
    unsigned long iv = 0;
    unsigned short crc = 0;
    int i = 0;
    unsigned short frame[18] = {0};
    //������֤��һ����������֤����ȡ�����
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
    MifareCommand.frame[txLen++] = ST95HF_SENDRCV_14443A_FLG_PARITY_MOD | ST95HF_SENDRCV_14443A_FLG_LSB_BIT8;  //��׼֡ + ��������У��λ
    
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
            //��ȡ�����
             MifareCard.nt = ARRAY_TO_UINT32(frame);
            if( MifareCard.crypto1On == 1)
            {
                //�����Ƕ����֤����Ҫ����һ��
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
        //��ʼ����������ks1
        iv =  MifareCard.uid ^  MifareCard.nt;
        MIFARE_update_word(iv, 0 );
    }
    //nr xor ks1��������ks2
    UINT32_TO_ARRAY_WITH_PARITY( MifareCard.nr, frame);
    for(i = 3; i >= 0; i--)
    {                           /* Same as in MIFARE_update_word, but with added parity */
        frame[3 - i] = frame[3 - i] ^ MIFARE_update_byte(( MifareCard.nr >> (i * 8)) & 0xff, 0 );
        frame[3 - i] ^= mf20( MifareCard.lfsr) << 8;
    } 

    //suc2(nt) xor ks2��������ks3���Ժ�ı���ͽ��붼��ks3���
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
    MifareCommand.frame[txLen++] = ST95HF_SENDRCV_14443A_FLG_PARITY_MOD | ST95HF_SENDRCV_14443A_FLG_LSB_BIT8;  //��׼֡ + crc
    
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
    
    MifareCommand.parms.cmd = ST95HF_CMD_SEL_PROTOCOL;            //ѡ��Э������0x02
    MifareCommand.len = 4; //protocol datarate              //ѡ��Э�������
    
    MifareCommand.frame[0] = ST95HF_PROTOCOL_ISO14443A;   //14443A
    MifareCommand.frame[1] = ST95HF_ISO14443A_TXRATE_106 | ST95HF_ISO14443A_RXRATE_106;             //���÷��ͺͽ��յĲ�����
    //FWT = (2^PP)*(MM+1)*(DD+128)*32/13.56 ��s
    //fwt = (2^2)*(0+1)*(0+128)*32/13.56 = 1.208ms
    MifareCommand.frame[2] = 0x02; //pp
    MifareCommand.frame[3] = 0x00; //mm
    
    fdt /= 1208;
    while(fdt > 255)
    {
        fdt >>= 1;
        MifareCommand.frame[2] += 1; //����2��
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
  
  
  if(SelGuard == 1)//��ͥ��
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
            //���һ���ֽڱ�ʾ���յ�����֡�Ƿ���Ч
            flag = pFrame->frame[pFrame->len - 1];
            //��ͻλ�е����⣬��ʱ��������ȷ��������ʵ��ͻλ
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
        if(SelGuard==1)//��ͥ��
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
*������ Read14443A_Block
*������������ �� ��ȡ������Ƭ�洢����Ϣ
*��������1 ��block �����
*��������2 ��ubuff ���ݻ���
*��������3 ��tag ��Ƭ��Ϣ
*��������ֵ ����ȡ���
*���� �� ���ֺ�
*������������ ��2017��12��27��
*�����޸����� ����
*�޸��� �� ��
*�޸�ԭ�� �� ��
*�汾 ��1.0
*��ʷ�汾 ����
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
*������ read_fukai_card_data
*������������ �� ��ȡ������Ƭ�洢��Ϣ
*��������1 ��ubuff ���ݴ洢����
*��������2 ��tag ��Ƭ��Ϣ
*��������ֵ ����ȡ���
*���� �� ���ֺ�
*������������ ��2017��12��27��
*�����޸����� ����
*�޸��� �� ��
*�޸�ԭ�� �� ��
*�汾 ��1.0
*��ʷ�汾 ����
*******************************************************************************/
unsigned char read_fukai_card_data(uint8_t* ubuff ,tagBufferType *tag )
{
  uint8_t udata[16];
  
  MifareCard.crypto1On = 0;
  if(Read14443A_Block(1,udata,tag) != 0)     //��ȡ��1������
  return false;  
  memcpy(&ubuff[0], udata, 16);
  
  if(Read14443A_Block(2,udata,tag) != 0)    //��ȡ��2������       
  return false; 
  memcpy(&ubuff[16], udata, 16);
  
  if(Read14443A_Block(4,udata,tag) != 0)   //��ȡ��4������  
  return false; 
  memcpy(&ubuff[32], udata, 9);
  
  if(Read14443A_Block(8,udata,tag) != 0)   //��ȡ��8������
  return false; 
  memcpy(&ubuff[41], udata, 16);                
  
  if(Read14443A_Block(9,udata,tag) != 0)   //��ȡ��9������
  return false; 
  memcpy(&ubuff[57], udata, 16);
  
  if(Read14443A_Block(10,udata,tag) != 0)  //��ȡ��10������
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

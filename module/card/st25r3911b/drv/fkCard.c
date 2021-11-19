#include "fkCard.h"
#include "unit.h"
#include "rfal_analogConfig.h"
#include "rfal_rf.h"
#include "mcc.h"
#include "iso14443a.h"
#include "string.h"

/*definition for mifare trial*/
uint8_t authentication_key;
uint16_t numBytesReceived;
uint8_t mifare_request[2];
iso14443AProximityCard_t card;
uint8_t perform_ac = 1;
uint16_t *responseLength;
#define MCC_READ_TIMEOUT             1  

ReturnCode err = ERR_NONE;


void st25ResetMifareCard(tagBufferType *tag)
{
    iso14443ACommand_t cmd =ISO14443A_CMD_REQA;        
    
    err = iso14443AInitialize();
    
    err = iso14443ASelect(cmd, &card, perform_ac);
    
    if(err == ERR_NONE)
    {    
        if (perform_ac)
        {
            tag->sak = card.sak[0];
            memcpy(tag->UID , card.uid , card.actlength);
            tag->UIDLength = card.actlength;
        }
    }
    
    err = mccInitialize();

    sys_delay(10);
}

/*!
 *****************************************************************************
 * \brief First step of authentication from the reader side.
 *
 * \note The array pointed to by \a uid must be at least 4 bytes long.

 *****************************************************************************
 */
ReturnCode MifareAuthenticationkey11(tagBufferType *tag,uint8_t blockNum)    
{    
    uint8_t block;
    uint8_t uidLength;
    uint8_t key_A[6] = {0x11,0x11,0x11,0x11,0x11,0x11};
   
    authentication_key = MCC_AUTH_KEY_A;
    uidLength = tag->UIDLength;    
    block = blockNum;
    
    if(authentication_key == MCC_AUTH_KEY_A)
    {
        err = mccAuthenticateStep1(authentication_key,
                block,
                tag->UID, 
                uidLength, 
                key_A);
        if (ERR_NONE != err)
        {
                return err;
        }
    }
    err = mccAuthenticateStep2(0x11223344);
    
    return err;
}

/*!
 *****************************************************************************
 * \brief First step of authentication from the reader side.
 *
 * \note The array pointed to by \a uid must be at least 4 bytes long.

 *****************************************************************************
 */
ReturnCode MifareAuthenticationkeyFF(tagBufferType *tag,uint8_t blockNum)    
{    
    uint8_t block;
    uint8_t uidLength;
    uint8_t key_A[6] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
   
    authentication_key = MCC_AUTH_KEY_A;
    uidLength = tag->UIDLength;    
    block = blockNum;
    
    if(authentication_key == MCC_AUTH_KEY_A)
    {
        err = mccAuthenticateStep1(authentication_key,
                block,
                tag->UID, 
                uidLength, 
                key_A);
        if (ERR_NONE != err)
        {
                return err;
        }
    }
    err = mccAuthenticateStep2(0x11223344);
    
    return err;
}
    
uint8_t st25ReadFkCardBlock(uint8_t blockNum,uint8_t* ubuff,tagBufferType *tag)
{
    /* MiFare read block command. */
    if (err ==0)
    {
        mifare_request[0] = MCC_READ_BLOCK;
        mifare_request[1] = blockNum; //BLOCK number

        err = mccSendRequest(mifare_request,
            sizeof(mifare_request),
            ubuff,
            *responseLength,
            &numBytesReceived,
            MCC_READ_TIMEOUT,
            false);
            
        *responseLength = numBytesReceived;
     }
    return err;
}

uint8_t  st25ReadFkCard( tagBufferType *tag )
{
    uint8_t udata[32];
    
    /* Mifaire read block trial.*/
    *responseLength = 0;
    
    /* Configure demoboard for MiFare. */
    err = mccInitialize();
    sys_delay(10);
    
    
    
    if(MifareAuthenticationkey11(tag,0))            //认证第0~3块
    { 
      st25ResetMifareCard(tag);
      MifareAuthenticationkeyFF(tag,0);
    }  
    
    if(st25ReadFkCardBlock(1,udata,tag))    //读取第1块数据
    return false;   
    memcpy(&tag->buffer[0], udata, 16);
    
    if(st25ReadFkCardBlock(2,udata,tag))    //读取第2块数据
    return false;    
    memcpy(&tag->buffer[16], udata, 16);    
    
    
    
    
    
    if(MifareAuthenticationkey11(tag,4))            //认证第4~7块    
    { 
      st25ResetMifareCard(tag);
      MifareAuthenticationkeyFF(tag,4);
    }
    
    if(st25ReadFkCardBlock(4,udata,tag))    //读取第4块数据
    return FALSE;                           
    memcpy(&tag->buffer[32], udata, 9);
    
    
    
    
    
    
    if(MifareAuthenticationkey11(tag,8))            //认证第8~11块
    { 
      st25ResetMifareCard(tag);
      MifareAuthenticationkeyFF(tag,8);
    }  
    
    if(st25ReadFkCardBlock(8,udata,tag))    //读取第8块数据
    return false;     
    memcpy(&tag->buffer[41], udata, 16);
    
    if(st25ReadFkCardBlock(9,udata,tag))    //读取第9块数据
    return false;    
    memcpy(&tag->buffer[57], udata, 16);
    
    if(st25ReadFkCardBlock(10,udata,tag))   //读取第10块数据
    return false;   
    memcpy(&tag->buffer[73], udata, 16);
    
    return true;
}
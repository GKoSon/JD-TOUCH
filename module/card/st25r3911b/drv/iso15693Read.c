#include "stdint.h"
#include "rfal_rf.h"
#include "rfal_nfca.h"
#include "rfal_nfcb.h"
#include "rfal_nfcf.h"
#include "rfal_nfcv.h"
#include "rfal_st25tb.h"
#include "rfal_nfcDep.h"
#include "rfal_iso15693_2.h"
#include "iso15693_3.h"
#include "iso14443a.h"
#include "rfal_isoDep.h"
#include "swipeTag.h"


iso15693ProximityCard_t iso15693Cards;

uint8_t st25WriteISO15693Data(uint8_t* data,uint8_t Length)
{
    ReturnCode err = ERR_NONE;
    uint8_t writeBuffer[100]={0} ,  Flag = 0;
 
    
    memset(writeBuffer , 0xFF , 100);
    memcpy(writeBuffer , data , Length);//Length 控制在100以内 我写100是4的整数倍而已
    iso15693PiccMemoryBlock_t memBlock;
    
    for(char i=0;i<25;i++)
    {
      memcpy(memBlock.data,&writeBuffer[4*i],4);
      memBlock.blocknr=i;
      memBlock.actualSize = 4;
      iso15693WriteSingleBlock(&iso15693Cards,Flag,&memBlock);
    }
    //log(INFO,"memBlock.errorCode = %d  \n" , memBlock.errorCode);

    return err;
}

uint8_t st25ReadISO15693Data( uint8_t Address , uint8_t Length ,uint8_t *Respone)
{
    ReturnCode err = ERR_NONE;
    uint8_t readBuffer[128] ,  resFlags = 0;
    uint16_t readLen = 0;
    
    memset(readBuffer , 0x00 , 128);
    
    err = iso15693ReadMultipleBlocks(&iso15693Cards,0, 32, &resFlags,readBuffer, 128, &readLen  );
    if (ERR_NONE == err)
    {
        //log(INFO,"Read leng = %d\n" , readLen);//127
        //log_arry(DEBUG,"Read Data:" , readBuffer , readLen);
        if(readLen >= Length )
        {
            memcpy( Respone , readBuffer , Length);
        }
        else
        {
            log(WARN,"读取卡片数据长度小于准备读取的长度，Read Len=%d\n" , readLen);
        }
    }
    else
    {
        log(DEBUG,"err=%d\n" , err);
    }
    
    return err;
}



uint8_t st25ReadISO15693TagUid(  tagBufferType *card )
{
    ReturnCode err = ERR_NONE;
    uint8_t actcnt;
    uint8_t i;
    
    err = iso15693Initialize(false, false);
    
    if (ERR_NONE == err)
    {
        memset(&iso15693Cards , 0x00 , sizeof(iso15693ProximityCard_t) );
        
        
        log(DEBUG,"【ISO15693】iso15693SelectPicc: %d \r\n",iso15693SelectPicc(&iso15693Cards));
        iso15693GetRng(&iso15693Cards);
        
        err = iso15693Inventory(
                    ISO15693_NUM_SLOTS_1,
                    0,
                    NULL,
                    &iso15693Cards,
                    sizeof(iso15693Cards)/sizeof(iso15693ProximityCard_t),
                    &actcnt);
        if (actcnt > 0)
        {                    
           //log(DEBUG,"ISO15693/NFC-V card(s) found. Cnt: %d UID: ", actcnt);
            for (i = 0; i < actcnt; i++)
            {
                /* flags, dsfid, uid */
               // log(DEBUG,"【ISO15693】flags: %d   dsfid: %d\r\n", iso15693Cards.flags,iso15693Cards.dsfid);
                memcpy(card->UID , iso15693Cards.uid , ISO15693_UID_LENGTH);
                card->UIDLength = ISO15693_UID_LENGTH;
                log_arry(DEBUG,"1--uid:" , iso15693Cards.uid ,ISO15693_UID_LENGTH);
                /*
                iso15693Cards.uid[0]=card->UID[7];
                iso15693Cards.uid[1]=card->UID[6];
                iso15693Cards.uid[2]=card->UID[5];
                iso15693Cards.uid[3]=card->UID[4];
                iso15693Cards.uid[4]=card->UID[3];
                iso15693Cards.uid[5]=card->UID[2];
                iso15693Cards.uid[6]=card->UID[1];
                iso15693Cards.uid[7]=card->UID[0];
*/
                
#if 1                
/*           
iso15693PiccSystemInformation_t sysInfo;
uint16_t sysInfoLen=0;
iso15693GetPiccSystemInformation(&iso15693Cards,&sysInfo,&sysInfoLen);

log(DEBUG,"【ISO15693】flags: %d \
infoFlags: %d \
dsfid:%d \
afi:%d \
memNumBlocks: %d \
memBlockSize: %d \
icReference:  %d ", sysInfo.flags,sysInfo.infoFlags,sysInfo.dsfid,sysInfo.afi,sysInfo.memNumBlocks,sysInfo.memBlockSize,sysInfo.icReference);\
log_arry(DEBUG,"uid:" , sysInfo.uid , ISO15693_UID_LENGTH);
//上面是63  3 我们可能是2K的卡

 */

/*        

iso15693PiccMemoryBlock_t memBlock;
memBlock.blocknr=0;//必须这里指定 读那个块
iso15693ReadSingleBlock(&iso15693Cards,&memBlock);  
log(DEBUG,"【ISO15693】flags: %d \
errorCode: %d \
securityStatus: %d \
blocknr: %d \
actualSize: %d ", memBlock.flags,memBlock.errorCode,memBlock.securityStatus,memBlock.blocknr,memBlock.actualSize);\
log_arry(DEBUG,"data:" , memBlock.data , memBlock.actualSize);

memBlock.blocknr=1;//必须这里指定 读那个块
iso15693ReadSingleBlock(&iso15693Cards,&memBlock); 
log(DEBUG,"【ISO15693】flags: %d \
errorCode: %d \
securityStatus: %d \
blocknr: %d \
actualSize: %d ", memBlock.flags,memBlock.errorCode,memBlock.securityStatus,memBlock.blocknr,memBlock.actualSize);\
log_arry(DEBUG,"data:" , memBlock.data , memBlock.actualSize);

*/     




//log(DEBUG,"【ISO15693】iso15693SelectPicc: %d \r\n",iso15693SelectPicc(&iso15693Cards));
 





iso15693GetRng(&iso15693Cards);
iso15693WritePwd(&iso15693Cards);
return TRACK_NONE;
#endif
                return TRACK_NFCTYPE5;
            }

        }
    }
    
    return TRACK_NONE;
}




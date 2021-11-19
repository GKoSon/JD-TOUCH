#include "EncryDecry.h"
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
#include "iso14443b.h"
#include "iso14443b_st25tb.h"
#include "swipeTag.h"
#include "iso14443bRead.h"



uint8_t st25Iso14443bReadUid( tagBufferType *tag )
{
    ReturnCode            err;
    iso14443BProximityCard_t card;
    iso14443BAttribParameter_t param;
    iso14443BAttribAnswer_t answer;
    
    uint8_t AttriB[ISO14443B_ATQB_SIZE]= {
                                /* Parameter 1 */   
                                TR0_64_FS                   | 
                                TR1_64_FS                   |
                                EOF_REQUIRED                |
                                SOF_REQUIRED                ,
                                /* Parameter 2 */                          
                                MAX_FRAME_SIZE_256_BYTES    |         
                                PCD_TO_PICC_106K            |
                                PICC_TO_PCD_106K            ,
                                /* Parameter 3 */              
                                TR2_32_FS                   |
                                PICC_COMPLIANT_ISO14443_4   ,
                                /* Parameter 4 */                          
                                CID_0                         
        };
    
    uint8_t uidParma[]={0x00,0x36,0x00,0x00,0x08};
    uint8_t rxBuf[128];
    uint16_t rxLen = 0;
    
    err = iso14443BInitialize();
    if (ERR_NONE == err)
    {    //ISO14443B_CMD_REQB
        err = iso14443BSelect(ISO14443B_CMD_WUPB ,&card, RFAL_NFCB_AFI ,ISO14443B_SLOT_COUNT_1 );
        if (ERR_NONE == err)
        { 
            memcpy((uint8_t *)&param , AttriB , 4 );
            err = iso14443BEnterProtocolMode(&card, &param, &answer);
            if (ERR_NONE == err)
            {
                err =rfalTransceiveBlockingTxRx(uidParma, sizeof(uidParma), rxBuf, 128, &rxLen,
                                           RFAL_TXRX_FLAGS_DEFAULT, RFAL_NFCB_FWTSENSB + RFAL_NFCB_DFWT_10 );
                if (ERR_NONE == err)
                {
                    //log_arry(DEBUG,"found. UID:", rxBuf, rxLen);
                    memcpy(tag->UID , rxBuf , 8);
                    tag->UIDLength = 8; 
                    
                    return TRACK_NFCTYPE4B;
                }
            }
        }
    }
    return TRACK_NONE;
}





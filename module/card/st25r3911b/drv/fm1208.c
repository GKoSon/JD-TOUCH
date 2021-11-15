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
#include "fm1208.h"
#include "cqBus.h"


#define chartonumber(x) (x-'0')
#define numbertochar(x) (x+'0')

#define ISO14443_CMD_DESELECT  0xca /*!< command DESELECT */

extern rfalIsoDepApduTxRxParam iso14443L4TxRxParams; 
rfalIsoDepApduBufFormat iso14443TxBuf;
rfalIsoDepApduBufFormat iso14443RxBuf;
rfalIsoDepBufFormat     iso14443TmpBuf;          /*!< Temp buffer for Rx I-Blocks (internal)   */


ReturnCode iso14443TransmitAndReceiveL4(const uint8_t* txbuf,
                                    uint16_t txlen,
                                    uint8_t* rxbuf,
                                    uint16_t rxlen,
                                    uint16_t* actrxlength)
{
    ReturnCode err;

    *actrxlength = 0;

    memcpy(iso14443TxBuf.apdu,txbuf, txlen); 
    iso14443L4TxRxParams.txBuf = &iso14443TxBuf; 
    iso14443L4TxRxParams.txBufLen = txlen; 
    iso14443L4TxRxParams.rxBuf = &iso14443RxBuf; 
    iso14443L4TxRxParams.rxBufLen = &rxlen; 
    iso14443L4TxRxParams.tmpBuf = &iso14443TmpBuf; 

    err = rfalIsoDepStartApduTransceive( iso14443L4TxRxParams );
    if (err) return err;

    while(ERR_BUSY == (err= rfalIsoDepGetApduTransceiveStatus()))
    {
        rfalWorker();
    }
    if (ERR_NONE == err)
    {
        *actrxlength = rxlen;
        memcpy(rxbuf,iso14443RxBuf.apdu,*actrxlength); 
    }
    return err;
}



uint8_t st25WriteAndRead14443aData( uint8_t *data , uint8_t len , uint8_t *response , uint16_t *rxSize)
{
	uint8_t rx[128];
	uint16_t rxLen = 0;
	uint16_t err = 0;
	
	memset(rx , 0x00 , 128);  
	err = iso14443TransmitAndReceiveL4( data,len, rx, 128, &rxLen  );
	if( err == ERR_NONE )
	{
		log_arry(INFO,"data:" , rx , rxLen);
		memcpy(response ,rx , rxLen -1 );
		*rxSize = rxLen -1 ;
	}
	else
	{
		//log(INFO,"[%s]err = %d\n" , __func__ , err);
		return FALSE;
	}
		
	return TRUE;
}




uint8_t read_fm1208_card_data(uint8_t *keydata,uint8_t *readdata)
{

         return TRUE;
}

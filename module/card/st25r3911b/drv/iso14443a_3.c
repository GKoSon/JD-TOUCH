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
#include "iso14443a_3.h"


uint8_t st25WriteAndReadIso14443aData( uint8_t *data , uint8_t len , uint8_t *response , uint16_t *rxSize)
{
	uint8_t rx[128];
	uint16_t rxLen = 0;
	uint16_t err = 0;
	
	memset(rx , 0x00 , 128);  
	err = rfalTransceiveBlockingTxRx( data,len, rx, 128, &rxLen , RFAL_TXRX_FLAGS_DEFAULT, 2097152 );
	if( err == ERR_NONE )
	{
		//log_arry(INFO,"data:" , rx , rxLen);
		
		memcpy(response ,rx+1 , rxLen -1 );
		*rxSize = rxLen -1 ;
	}
	else
	{
		//log(INFO,"[%s]err = %d\n" , __func__ , err);
		return FALSE;
	}
		
	return TRUE;
}



uint8_t iso14443a_read_uuid(  tagBufferType *card )
{
	iso14443AProximityCard_t iso14443Acard;
	ReturnCode err = ERR_NONE;
	uint8_t perform_ac = 1;
	
	
	rfalNfcaPollerInitialize();  
        err =  rfalFieldOnAndStartGT();
	if( err != ERR_NONE)
	{
		log(WARN,"ST25ÇÐ»»14443AÐ­ÒéÊ§°Ü,err=%d\n" , err);
		return err;
	}
	
	memset(&iso14443Acard ,0x00 , sizeof(iso14443AProximityCard_t));
	err = iso14443ASelect(ISO14443A_CMD_REQA, &iso14443Acard, perform_ac);
	if(err == ERR_NONE)
	{
		//log_arry(DEBUG,"ISO14443A/NFC-A card found. ATQA:", iso14443Acard.atqa, 2);
		if (perform_ac)
		{
			//log_arry(DEBUG,"sak:",iso14443Acard.sak, 3);
			//log_arry(DEBUG,"uuid:",iso14443Acard.uid, iso14443Acard.actlength);
			
			card->sak = iso14443Acard.sak[0];
			memcpy(card->UID , iso14443Acard.uid , iso14443Acard.actlength);
			card->UIDLength = iso14443Acard.actlength;
				
			 return TRACK_NFCTYPE4A;
		}
	}
	//log(DEBUG,"read iso14443a card uid, err=%d\n" , err);
	return TRACK_NONE;
}


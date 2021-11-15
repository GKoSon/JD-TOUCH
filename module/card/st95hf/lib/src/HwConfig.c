#include "HwConfig.h"
#include "lib_iso14443Bpcd.h"
#include "lib_iso14443A.h"
#include "spi.h"

extern ISO14443A_CARD ISO14443A_Card;

bool updateFlash = false;

uint32_t 							FileSize = 0;
uint32_t 							FileSizeReceive = 0;
uint32_t 							CRCFile = 0;
uint32_t 							CRCCalc = 0;
uint32_t 							ElapsedTimeMs = 0;
uint8_t								FILE_TRANSFER_END=0;



/* TT1 (PCD only)*/
//uint8_t TT1Tag[NFCT1_MAX_TAGMEMORY];
/* TT2 */
//uint8_t TT2Tag[NFCT2_MAX_TAGMEMORY];

/* TT3 */
//uint8_t TT3Tag[NFCT3_MAX_TAGMEMORY];
//uint8_t *TT3AttribInfo = TT3Tag, *TT3NDEFfile = &TT3Tag[NFCT3_ATTRIB_INFO_SIZE];

/* TT4 */
//uint8_t CardCCfile [NFCT4_MAX_CCMEMORY];
//uint8_t CardNDEFfileT4A [NFCT4_MAX_NDEFMEMORY];
//uint8_t CardNDEFfileT4B [NFCT4_MAX_NDEFMEMORY]; 

/* TT5 (PCD only)*/
//uint8_t TT5Tag[NFCT5_MAX_TAGMEMORY];



/**
* @brief  buffer to exchange data with the RF tranceiver.
*/
//uint8_t				u95HFBuffer [RFTRANS_95HF_MAX_BUFFER_SIZE+3];
uint8_t				u95HFBuffer [RFTRANS_95HF_MAX_ECHO+3];



/* Variable to know IC version */
IC_VERSION IcVers = QIN;  /* default set last IC version */

static int8_t HwConfig_ReadVersion(uint8_t *pResponse)
{
	uc8 DataToSend[] = {IDN	,0x00};

	/* send the command to the PICC and retrieve its response */
	drv95HF_SendReceive(DataToSend, pResponse);

	return MANAGER_SUCCESSCODE;
}

static int16_t HwConfig_Poresquence( void )
{
	uint16_t NthAttempt=0;
	uc8 command[]= {ECHO};
	
	/* Power up sequence: Pulse on IRQ_IN to select UART or SPI mode */
	drv95HF_SendIRQINPulse();
	
	/* SPI Reset */
	if(drv95HF_GetSerialInterface() == RFTRANS_95HF_INTERFACE_SPI)
	{
		drv95HF_ResetSPI();		
	}
	
	do{
	
		/* send an ECHO command and checks response */
		drv95HF_SendReceive(command, u95HFBuffer);

		if (u95HFBuffer[0]==ECHORESPONSE)
			return MANAGER_SUCCESSCODE;	

		/* if the SPI interface is selected then send a reset command*/
		if(drv95HF_GetSerialInterface() == RFTRANS_95HF_INTERFACE_SPI)
		{	
			drv95HF_ResetSPI();				
		}
#ifdef CR95HF		
		/* if the UART interface is selected then send 255 ECHO commands*/
		else if(drv95HF_GetSerialInterface() == RFTRANS_95HF_INTERFACE_UART)
		{
			do {
				/* send an ECHO command and checks response */
				drv95HF_SendReceive(command, u95HFBuffer);
				if (u95HFBuffer[0] == ECHORESPONSE)
					return MANAGER_SUCCESSCODE;	
                
			}while(NthAttempt++ < RFTRANS_95HF_MAX_ECHO);
		}
#endif /* CR95HF */
	} while (u95HFBuffer[0]!=ECHORESPONSE && NthAttempt++ <5);
	return MANAGER_ERRORCODE_PORERROR;	
}

extern ISO14443A_MIFAREAUTH MifareCard;
extern void ConfigManager_Start( void );

uint8_t TagHuntingResetM1( tagBufferType *tag)
{
	// Start the config manager
	ConfigManager_Start();

	/******* NFC type 2 and 4A ********/

		PCD_FieldOff();
		nSpi->DelayMs(5);
		ISO14443A_Init();
		if(ISO14443A_IsPresent() == RESULTOK)
		{
            //if(nfc->TagVar->TagType==TongCard)
            {
                //goto HsjError;
            }
            
			if(ISO14443A_Anticollision() == RESULTOK)
			{
				tag->UIDLength = ISO14443A_Card.UIDsize;

				if ((ISO14443A_Card.SAK == 0x08) || (ISO14443A_Card.SAK == 0x28))
				{
					memcpy(tag->UID,ISO14443A_Card.UID,ISO14443A_Card.UIDsize);
					memset(&MifareCard, 0, sizeof(ISO14443A_MIFAREAUTH));
					return TAG_FUKAI_CARD;
				}
			}
		}
//HsjError:
	PCD_FieldOff();
	return 0;
}

uint8_t st95hf_verification( void )
{
    return((IcVers==QJE)?TRUE:FALSE);
}

void  st95hf_hal_init( void )
{
	MX_SPI2_ST95_Init();
	
	/* Initialize HW according to protocol to use */
	Drv95HF_Init();
	
	/* initilialize the RF transceiver */
	if (HwConfig_Poresquence( ) != MANAGER_SUCCESSCODE)
	{
		/* nothing to do, this is a trap for debug purpose you can use it to detect HW issue */
		/* or GPIO config issue */
		log(ERR,"ST95HF initilialize is error.\r\n");        
	}
	HwConfig_ReadVersion(u95HFBuffer);
	IcVers = (IC_VERSION) (u95HFBuffer[ROM_CODE_REVISION_OFFSET]);
    
}

                  

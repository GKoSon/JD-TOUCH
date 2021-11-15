#include "Drv95HF.h"

/* ConfigStructure */ 										 
drv95HF_ConfigStruct			drv95HFConfig;

/**
 *	@brief  Send a negative pulse on IRQin pin
 *  @param  none
 *  @retval None
 */
void drv95HF_SendIRQINPulse(void)
{
	if (drv95HFConfig.uInterface == RFTRANS_95HF_INTERFACE_SPI)
	{
		/* Send a pulse on IRQ_IN */
		nSpi->ChipWakeUp(ENABLE);
		delayHighPriority_ms(1);
		nSpi->ChipWakeUp(DISABLE);
		delayHighPriority_ms(1);
		nSpi->ChipWakeUp(ENABLE);
	}
#ifdef CR95HF	
	else if (drv95HFConfig.uInterface == RFTRANS_95HF_INTERFACE_UART)
	{
	 	/* Send a pulse on IRQ_IN (UART_TX) */
		UART_SendByte(RFTRANS_95HF_UART, 0x00);
	}
#endif /* CR95HF */
	
	/* Need to wait 10ms after the pulse before to send the first command */
	delayHighPriority_ms(10);

}

/**
 *	@brief  This function sends a reset command over SPI bus
 *  @param  none
 *  @retval None
 */
static void drv95HF_SendSPIResetByte(void)
{
	/* Send reset control byte */
	nSpi->SendReceiveByte(RFTRANS_95HF_COMMAND_RESET);
}


/**
 *	@brief  Send a reset sequence over SPI bus (Reset command ,wait ,negative pulse on IRQin).
 *  @param  None
 *  @retval None
 */
void drv95HF_ResetSPI ( void )
{	
	/* Deselect Rftransceiver over SPI */
	nSpi->ChipSelet(ENABLE);
	delayHighPriority_ms(1);
	/* Select 95HF device over SPI */
	nSpi->ChipSelet(DISABLE);
	/* Send reset control byte	*/
	drv95HF_SendSPIResetByte();
	/* Deselect 95HF device over SPI */
	nSpi->ChipSelet(ENABLE);
	delayHighPriority_ms(3);

	/* send a pulse on IRQ_in to wake-up 95HF device */
	drv95HF_SendIRQINPulse();
	delayHighPriority_ms(10);  /* mandatory before issuing a new command */

	drv95HFConfig.uState = RFTRANS_95HF_STATE_READY;
	
}


/**
 *	@brief  This function returns the Interface selected(UART or SPI)
 *  @param  none
 *  @retval RFTRANS_INTERFACE_UART : the UART interface is selected
 *  @retval RFTRANS_INTERFACE_SPI : the SPI interface is selected
 */
uint8_t drv95HF_GetSerialInterface ( void )
{
	return drv95HFConfig.uInterface;
}

/*	@brief  This function sends a command over SPI bus
 *  @param  *pData : pointer on data to send to the xx95HF
 *  @retval void
 */
void drv95HF_SendSPICommand(uc8 *pData)
{
	uint8_t DummyBuffer[MAX_BUFFER_SIZE];
	uint16_t bufferlength = 0;
	  	
	/*  Select xx95HF over SPI  */
	nSpi->ChipSelet(DISABLE);

	/* Send a sending request to xx95HF  */
	nSpi->SendReceiveByte(RFTRANS_95HF_COMMAND_SEND);

	if(*pData == ECHO)
	{
		/* Send a sending request to xx95HF */ 
		nSpi->SendReceiveByte(ECHO);
	}
	else
	{
		if( pData[RFTRANS_95HF_COMMAND_OFFSET] == 0x24 )
		{
			bufferlength = pData[RFTRANS_95HF_LENGTH_OFFSET] + RFTRANS_95HF_DATA_OFFSET + 256;
		}
		else if (pData[RFTRANS_95HF_COMMAND_OFFSET] == 0x44)
		{
			bufferlength = pData[RFTRANS_95HF_LENGTH_OFFSET] + RFTRANS_95HF_DATA_OFFSET + 512;
		}
		else
		{
			bufferlength = pData[RFTRANS_95HF_LENGTH_OFFSET] + RFTRANS_95HF_DATA_OFFSET;
		}
        
        if( bufferlength <  MAX_BUFFER_SIZE)
        {
             /* Transmit the buffer over SPI */
        #ifdef USE_DMA	
            SPI_SendReceiveBufferDMA(RFTRANS_95HF_SPI, pData, bufferlength, DummyBuffer);
        #else
            nSpi->SendReceive(pData, bufferlength, DummyBuffer);
        #endif       
        }

	}
	
	/* Deselect xx95HF over SPI  */
	nSpi->ChipSelet(ENABLE);
}

/**
 *	@brief  This function polls 95HF chip until a response is ready or
 *				  the counter of the timeout overflows
 *  @retval PCD_POLLING_TIMEOUT : The time out was reached 
 *  @retval PCD_SUCCESS_CODE : A response is available
 */
static int8_t drv95HF_SPIPollingCommand( void )
{
	uint8_t Polling_Status = 0;
	uint16_t dTimeOut = 200;
	if (drv95HFConfig.uSpiMode == RFTRANS_95HF_SPI_POLLING)
	{

		do{
			/* in case of an HID interuption during the process that can desactivate the timeout */
			/* Enable the Time out timer */
			//TIM_Cmd(TIMER_TIMEOUT, ENABLE);
			
			nSpi->ChipSelet(DISABLE);
			nSpi->DelayMs(1);	
            
			/*  poll the 95HF transceiver until he's ready ! */
			Polling_Status  = nSpi->SendReceiveByte(RFTRANS_95HF_COMMAND_POLLING);
			
			Polling_Status &= RFTRANS_95HF_FLAG_DATA_READY_MASK;
	
            //HAL_IWDG_Refresh(&hiwdg);
            
		}	while( Polling_Status 	!= RFTRANS_95HF_FLAG_DATA_READY && --dTimeOut );
		
		nSpi->ChipSelet(ENABLE);
	}	
	else if (drv95HFConfig.uSpiMode == RFTRANS_95HF_SPI_INTERRUPT)
	{
		/* Wait a low level on the IRQ pin or the timeout  */
		//while( (uDataReady == false) & (uTimeOut == false) )
		{	}		
	}
	if ( dTimeOut == 0 )
		return RFTRANS_95HF_POLLING_TIMEOUT;

	return RFTRANS_95HF_SUCCESS_CODE;	
}

/**
 *	@brief  This fucntion recovers a response from 95HF device
 *  @param  *pData : pointer on data received from 95HF device
 *  @retval None
 */
void drv95HF_ReceiveSPIResponse(uint8_t *pData)
{
	uint8_t DummyBuffer[MAX_BUFFER_SIZE];
	uint16_t lengthToRead = 0;

	/* Select 95HF transceiver over SPI */
	nSpi->ChipSelet(DISABLE);

	/* Request a response from 95HF transceiver */
	nSpi->SendReceiveByte(RFTRANS_95HF_COMMAND_RECEIVE);

	/* Recover the "Command" byte */
	pData[RFTRANS_95HF_COMMAND_OFFSET] = nSpi->SendReceiveByte( DUMMY_BYTE);

	if(pData[RFTRANS_95HF_COMMAND_OFFSET] == ECHO)
	{
		pData[RFTRANS_95HF_LENGTH_OFFSET]  = 0x00;
		/* In case we were in listen mode error code cancelled by user (0x85 0x00) must be retrieved */
		pData[RFTRANS_95HF_LENGTH_OFFSET+1] = nSpi->SendReceiveByte( DUMMY_BYTE);
		pData[RFTRANS_95HF_LENGTH_OFFSET+2] = nSpi->SendReceiveByte( DUMMY_BYTE);
	}
	else if(pData[RFTRANS_95HF_COMMAND_OFFSET] == 0xFF)
	{
		pData[RFTRANS_95HF_LENGTH_OFFSET]  = 0x00;
		pData[RFTRANS_95HF_LENGTH_OFFSET+1] = nSpi->SendReceiveByte( DUMMY_BYTE);
		pData[RFTRANS_95HF_LENGTH_OFFSET+2] = nSpi->SendReceiveByte( DUMMY_BYTE);
	}
	else
	{
		/* Recover the "Length" byte */
		pData[RFTRANS_95HF_LENGTH_OFFSET]  = nSpi->SendReceiveByte( DUMMY_BYTE);
		/* Checks the data length */
		if( !( ( ( pData[RFTRANS_95HF_COMMAND_OFFSET] & 0xE0) == 0x80 ) && (pData[RFTRANS_95HF_LENGTH_OFFSET] == 0x00) ) )
        {
              lengthToRead = (uint16_t)(pData[RFTRANS_95HF_COMMAND_OFFSET] & 0x60);
              lengthToRead = (lengthToRead << 3) + pData[RFTRANS_95HF_LENGTH_OFFSET];
              
              if(  lengthToRead < MAX_BUFFER_SIZE)
              {
                /* Recover data 	*/
            #ifdef USE_DMA
                  SPI_SendReceiveBufferDMA(RFTRANS_95HF_SPI, DummyBuffer, lengthToRead, &pData[RFTRANS_95HF_DATA_OFFSET]);    
            #else
                  nSpi->SendReceive(DummyBuffer, lengthToRead, &pData[RFTRANS_95HF_DATA_OFFSET]);
            #endif
              }
		}
	}

	/* Deselect xx95HF over SPI */
	nSpi->ChipSelet(ENABLE);
	
}

/**
 *	@brief  This function send a command to 95HF device over SPI or UART bus and receive its response
 *  @param  *pCommand  : pointer on the buffer to send to the 95HF device ( Command | Length | Data)
 *  @param  *pResponse : pointer on the 95HF device response ( Command | Length | Data)
 *  @retval RFTRANS_95HF_SUCCESS_CODE : the function is succesful
 */
int8_t  drv95HF_SendReceive(uc8 *pCommand, uint8_t *pResponse)
{		
	u8 command = *pCommand;
	
	/* if we want to send a command we are not expected a interrupt from RF event */
	if(drv95HFConfig.uSpiMode == RFTRANS_95HF_SPI_INTERRUPT)
	{	
		//drvInt_Enable_Reply_IRQ();
		nSpi->ChipInterr(ENABLE);
	}
	
	if(drv95HFConfig.uInterface == RFTRANS_95HF_INTERFACE_SPI)
	{
		/* First step  - Sending command 	*/
		drv95HF_SendSPICommand(pCommand);
		/* Second step - Polling	*/
		if (drv95HF_SPIPollingCommand( ) != RFTRANS_95HF_SUCCESS_CODE)
		{	
			*pResponse =RFTRANS_95HF_ERRORCODE_TIMEOUT;
			return RFTRANS_95HF_POLLING_TIMEOUT;	
		}
		/* Third step  - Receiving bytes */
		drv95HF_ReceiveSPIResponse(pResponse);
	}
#ifdef CR95HF	
	else if(drv95HFConfig.uInterface == RFTRANS_95HF_INTERFACE_UART)
	{
		/* First step  - Sending command	*/
		drv95HF_SendUARTCommand(pCommand);
		/* Second step - Receiving bytes */
		drv95HF_ReceiveUARTResponse(pResponse);
	}
#endif /* CR95HF */
	
	/* After listen command is sent an interrupt will raise when data from RF will be received */
	if(command == LISTEN)
	{	
		if(drv95HFConfig.uSpiMode == RFTRANS_95HF_SPI_INTERRUPT)
		{		
			//drvInt_Enable_RFEvent_IRQ( );
		}
	}

	return RFTRANS_95HF_SUCCESS_CODE; 
}
/**
* @brief  	Initilize the 95HF device config structure
* @param  	None
* @retval 	None
*/
void drv95HF_InitConfigStructure (void)
{
	drv95HFConfig.uInterface = RFTRANS_95HF_INTERFACE_SPI;
	drv95HFConfig.uSpiMode = RFTRANS_95HF_SPI_POLLING;
	drv95HFConfig.uState = RFTRANS_95HF_STATE_POWERUP;
	drv95HFConfig.uCurrentProtocol = RFTRANS_95HF_PROTOCOL_UNKNOWN;
	drv95HFConfig.uMode = RFTRANS_95HF_MODE_UNKNOWN;
}


void Drv95HF_Init( void )
{
	drv95HF_InitConfigStructure();
	nSpi->SpiInit();
}





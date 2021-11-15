#include "HalNfcSpi.h"



void *nfcSpi = NULL;


void HalSpiInit( void )
{
    nfcSpi = halSpi.open("spi2");
    
    if( nfcSpi == NULL )
    {
        log_err("´ò¿ªSPI2Ê§°Ü\n");
    }
    
    halSpi.init(nfcSpi);
        
    
    pin_ops.pin_mode(ST95_IQRI_PIN , PIN_MODE_OUTPUT);  
    halSpi.chip_select(nfcSpi , ENABLE);	
    pin_ops.pin_write(ST95_IQRI_PIN , PIN_HIGH);
}


uint8_t SPI_SendReceiveByte(uint8_t data) 
{	
    return (halSpi.send_reveive_byte(nfcSpi ,data ));
}



void SPI_SendReceiveBuffer(uc8 *pCommand, uint16_t length, uint8_t *pResponse)
{
   // halSpi.send_reveive_buff(nfcSpi , (uint8_t *)pCommand,  length,  pResponse );
    
    HAL_SPI_TransmitReceive(nfcSpi, (uint8_t*)pCommand, (uint8_t *)pResponse, length, 100);
    
}

void HalSpiSetCs( uint8_t NewState )
{
 
    if ( NewState == DISABLE)
    {
        halSpi.chip_select(nfcSpi , ENABLE);	
    }
    else
    {
        halSpi.chip_select(nfcSpi , DISABLE);	
    }

}

void HalSpiSetIrqIn( uint8_t NewState )
{
	pin_ops.pin_write(ST95_IQRI_PIN , NewState);
}

void HalIntSet( uint8_t NewState)
{

}

void HalDelayMs( uint32_t DelayTime )
{
	sys_delay(DelayTime);
}

void HalDelayUs(uint32_t DelayTime)
{
	sys_delay(DelayTime/1000);
}

HalNfcType HalNfc = 
{
	.SpiInit = HalSpiInit,
	//.InterruptInit = HalST95InterruptInit,
	.SendReceiveByte = SPI_SendReceiveByte,
	.SendReceive = SPI_SendReceiveBuffer,
	.ChipSelet = HalSpiSetCs,
	.ChipWakeUp = HalSpiSetIrqIn ,
	.ChipInterr = HalIntSet,
	.DelayMs = HalDelayMs,
	.DelayUs = HalDelayUs,
};

HalNfcType *nSpi = &HalNfc;

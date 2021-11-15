#include "HalNfcSpi.h"

void HalST95InterruptInit( void );

HalGpioInfoType HalSpiSck=
{
    .port       = GPIOB,
    .pin        = GPIO_Pin_3,
    .rcc        = RCC_AHB1Periph_GPIOB,
    .source     = GPIO_PinSource3,
    .af         = GPIO_AF_SPI3,
    .rcc_init   = RCC_AHB1PeriphClockCmd,
};

HalGpioInfoType HalSpiMiso=
{
    .port       = GPIOB,
    .pin        = GPIO_Pin_4,
    .rcc        = RCC_AHB1Periph_GPIOB,
    .source     = GPIO_PinSource4,
    .af         = GPIO_AF_SPI3,
    .rcc_init   = RCC_AHB1PeriphClockCmd,
};

HalGpioInfoType HalSpiMosi=
{
    .port       = GPIOC,
    .pin        = GPIO_Pin_12,
    .rcc        = RCC_AHB1Periph_GPIOC,
    .source     = GPIO_PinSource12,
    .af         = GPIO_AF_SPI3,
    .rcc_init   = RCC_AHB1PeriphClockCmd,
};

HalGpioInfoType HalSpiCs=
{
    .port       = GPIOA,
    .pin        = GPIO_Pin_15,
    .rcc        = RCC_AHB1Periph_GPIOA,
    .rcc_init   = RCC_AHB1PeriphClockCmd,
};

HalGpioInfoType HalSpiIrqIn=
{
    .port       = GPIOD,
    .pin        = GPIO_Pin_0,
    .rcc        = RCC_AHB1Periph_GPIOD,
    .source     = 0,
    .af         = 0,
    .rcc_init   = RCC_AHB1PeriphClockCmd,
};

HalSpiInfoType	HalSpi =
{
	.port		= SPI3,
	.rcc        = RCC_APB1Periph_SPI3,
	.rcc_init    = RCC_APB1PeriphClockCmd,
};



void HalSpiInit( void )
{

	SPI_InitTypeDef  SPI_InitStructure;
	GPIO_InitTypeDef GPIO_InitStructure;
	
	//CLOCK
	HalSpi.rcc_init(HalSpi.rcc, ENABLE);
	HalSpiSck.rcc_init(HalSpiSck.rcc, ENABLE);
    HalSpiMiso.rcc_init(HalSpiMiso.rcc, ENABLE);
	HalSpiMosi.rcc_init(HalSpiMosi.rcc, ENABLE);
	HalSpiIrqIn.rcc_init(HalSpiIrqIn.rcc , ENABLE);
    HalSpiCs.rcc_init(HalSpiCs.rcc, ENABLE);
	
	//GPIO
	GPIO_PinAFConfig(HalSpiSck.port, HalSpiSck.source, HalSpiSck.af);
	GPIO_PinAFConfig(HalSpiMiso.port, HalSpiMiso.source, HalSpiMiso.af);
    GPIO_PinAFConfig(HalSpiMosi.port, HalSpiMosi.source, HalSpiMosi.af);
	
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_NOPULL;//GPIO_PuPd_DOWN;
	//sck
	GPIO_InitStructure.GPIO_Pin = HalSpiSck.pin;
	GPIO_Init(HalSpiSck.port, &GPIO_InitStructure);
	//msio
	GPIO_InitStructure.GPIO_Pin = HalSpiMiso.pin;
	GPIO_Init(HalSpiMiso.port, &GPIO_InitStructure);
	//mosi
	GPIO_InitStructure.GPIO_Pin = HalSpiMosi.pin;
	GPIO_Init(HalSpiMosi.port, &GPIO_InitStructure);
	//cs
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;//GPIO_PuPd_NOPULL;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Pin = HalSpiCs.pin;
	GPIO_Init(HalSpiCs.port, &GPIO_InitStructure);
	//IRQ_IN
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
	GPIO_InitStructure.GPIO_Pin = HalSpiIrqIn.pin;
	GPIO_Init(HalSpiIrqIn.port, &GPIO_InitStructure);
	
	HalSetGpio(&HalSpiCs, ENABLE);
	HalSetGpio(&HalSpiIrqIn ,ENABLE);
	
	SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
	SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
	SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
	SPI_InitStructure.SPI_CPOL = SPI_CPOL_High;
	SPI_InitStructure.SPI_CPHA = SPI_CPHA_2Edge;
	SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
	SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_32;
	SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
	SPI_InitStructure.SPI_CRCPolynomial = 7;
	SPI_Init(HalSpi.port, &SPI_InitStructure);

	SPI_Cmd(HalSpi.port, ENABLE);
	
}



HalGpioInfoType HalSpiIrqOut=
{
    .port       = GPIOD,
    .pin        = GPIO_Pin_1,
    .rcc        = RCC_AHB1Periph_GPIOD,
    .source     = 0,
    .af         = 0,
    .rcc_init    = RCC_AHB1PeriphClockCmd,
};


HalExitInfoType	HalIrqOut = 
{
	.Port		= EXTI_PortSourceGPIOD,
	.Source		= GPIO_PinSource1,
	.Irq		= EXTI1_IRQn,
	.Line		= EXTI_Line1,
	.Trigger	= EXTI_Trigger_Rising_Falling,
};

void HalST95InterruptInit( void )
{
	GPIO_InitTypeDef GPIO_InitStructure;	
    NVIC_InitTypeDef NVIC_InitStructure;
	EXTI_InitTypeDef EXTI_InitStructure;
	
	GPIO_InitStructure.GPIO_Pin = HalSpiIrqOut.pin;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_Speed =GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(HalSpiIrqOut.port, &GPIO_InitStructure);

    EXTI_InitStructure.EXTI_Line = HalIrqOut.Line;
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = HalIrqOut.Trigger;
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_Init(&EXTI_InitStructure);
    SYSCFG_EXTILineConfig(HalIrqOut.Port,HalIrqOut.Source);
    
    NVIC_InitStructure.NVIC_IRQChannel = HalIrqOut.Irq;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0; //抢断优先级为0
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;    //响应优先级为0
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure); 

}


uint8_t SPI_SendReceiveByte(uint8_t data) 
{	
	while((st95_SPI->SR & SPI_I2S_FLAG_TXE)== 0);
	st95SpiSendData(data);
	while((st95_SPI->SR & SPI_I2S_FLAG_RXNE)== 0);
	return st95SpiReceiveData(st95_SPI);;
}



void SPI_SendReceiveBuffer(uc8 *pCommand, uint16_t length, uint8_t *pResponse)
{
	for(uint16_t i=0; i<length; i++)
		pResponse[i] = SPI_SendReceiveByte(pCommand[i]);
}

void HalSpiSetCs( uint8_t NewState )
{
	HalSetGpio(&HalSpiCs, NewState);	
}

void HalSpiSetIrqIn( uint8_t NewState )
{
	HalSetGpio(&HalSpiIrqIn ,NewState);
}

void HalIntSet( uint8_t NewState)
{

}

void HalDelayMs( uint32_t DelayTime )
{
	vTaskDelay(DelayTime);
}

void HalDelayUs(uint32_t DelayTime)
{
	vTaskDelay(DelayTime/1000);
}

HalNfcType HalNfc = 
{
	.SpiInit = HalSpiInit,
	.InterruptInit = HalST95InterruptInit,
	.SendReceiveByte = SPI_SendReceiveByte,
	.SendReceive = SPI_SendReceiveBuffer,
	.ChipSelet = HalSpiSetCs,
	.ChipWakeUp = HalSpiSetIrqIn ,
	.ChipInterr = HalIntSet,
	.DelayMs = HalDelayMs,
	.DelayUs = HalDelayUs,
};

HalNfcType *nSpi = &HalNfc;

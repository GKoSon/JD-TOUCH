#include "hal_spi.h"


void *hal_spi_init( void *port)
{
    return port;
    
}

void hal_spi_set_gpio(GPIO_TypeDef *gpio , uint16_t pin , uint8_t statue )
{
	if( statue )
	{
		HAL_GPIO_WritePin(gpio, pin ,  GPIO_PIN_RESET );
	}
	else
	{
		HAL_GPIO_WritePin(gpio, pin , GPIO_PIN_SET );         
	}  
}


void hal_spi_set_cs( void *port , uint8_t NewState )
{
    SPI_HandleTypeDef *spi = (SPI_HandleTypeDef *)port;
    
    if(spi->Instance == SPI1 )
    {
        hal_spi_set_gpio(GPIOA, GPIO_PIN_4, NewState);	
    }
    else if(spi->Instance == SPI2 )
    {
        hal_spi_set_gpio(GPIOB, GPIO_PIN_12, NewState);
    }
    else if(spi->Instance == SPI3 )
    {
        hal_spi_set_gpio(GPIOB , GPIO_PIN_6 , NewState);
    }
    
}

uint8_t hal_spi_send_and_receive_byte(void *port , uint8_t  data) 
{	
    SPI_HandleTypeDef *spi = (SPI_HandleTypeDef *)port;
    uint8_t aRxBuffer = 0;


    if( HAL_SPI_TransmitReceive(spi, (uint8_t*)&data, (uint8_t *)&aRxBuffer, 1, 100) == HAL_OK)
    {
        return aRxBuffer;
    }
    
    return 0xFF;
}

uint8_t hal_spi_send_and_receive_buffer(void *port , uint8_t *pCommand, uint16_t length, uint8_t *pResponse)
{
    SPI_HandleTypeDef *spi = (SPI_HandleTypeDef *)port;

    if(  HAL_SPI_Receive(spi, (uint8_t *)pResponse, length, 100) == HAL_OK)
    {
        return HAL_OK;
    }

    return 0xFF;
	
}


device_spi   spi_t[]=
{
    {"spi1" , &hspi1},
    {"spi2" , &hspi2},
    {"spi3" , &hspi3},
};


void *spi_open( char *name )
{
    for( uint8_t i = 0 ; i < sizeof (spi_t) / sizeof (spi_t[0]);  i++)
    {
        if( strcmp(name , spi_t[i].name) == 0 )
        {
            return spi_t[i].spi;
        }
    }
    
    return NULL;
}

halSpiType    halSpi=
{
    spi_open,
    hal_spi_init,
    hal_spi_set_cs,
    hal_spi_send_and_receive_byte,
    hal_spi_send_and_receive_buffer,
};
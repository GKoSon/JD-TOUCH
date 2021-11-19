#include "st25Drv.h"
#include "st25_spi.h"
#include "unit.h"
#include "bsp_gpio.h"
#include "st25r3911_interrupt.h"
#include "rfal_analogConfig.h"
#include "rfal_rf.h"
#include "iso15693Read.h"
#include "iso14443a_3.h"
#include "iso14443bRead.h"
#include "spi.h"
#include "fkCard.h"
#include "fm1208.h"




extern __IO uint8_t deviceUseChipModule;


void st25_hal_init( void )
{
    MX_SPI2_ST25_Init();
    spiInit(&hspi2);
    
    pin_ops.pin_mode(ST25_IRQ_PIN , PIN_MODE_INPUT_PULLUP);
    pin_ops.pin_exit(ST25_IRQ_PIN , st25r3911Isr);
    
    pin_ops.pin_mode(ST25_CS_PIN , GPIO_MODE_OUTPUT_PP);
    
    /* Initalize RFAL */
      rfalAnalogConfigInitialize();
      if( rfalInitialize() == ERR_NONE )
    {
        log(DEBUG,"[CARD]RFAL initialization succeeded..\n");
    }
    else
    {
        log(WARN,"[CARD]RFAL initialization failed..\n");
    }

}

void st25FieldOff( tagBufferType *tag )
{
    if( tag->type != TAG_PHONE_CARD)
    {
        rfalFieldOff();
    }
    sys_delay(50);
}

tagObjType    st25Tag = 
{
    .init    = st25_hal_init,
    .iso15693_get_uid = st25ReadISO15693TagUid,
    .iso15693_read_data = st25ReadISO15693Data,
    .iso14443a_get_uid = iso14443a_read_uuid,
    .iso14443b_get_uid = st25Iso14443bReadUid,
    .read_m1_data = st25ReadFkCard,
    .fm1208_read_data = read_fm1208_card_data,
    .turnOffField  = st25FieldOff,
};


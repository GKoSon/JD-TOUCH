#ifndef _ANYID_R322_ST95HF_HL_
#define _ANYID_R322_ST95HF_HL_

//#include "types.h"
//#include "AnyID_R322_Config.h"
//#include "drv_interrupt.h"
/*extern const PORT_INF ST95HF_SPI_PORT;
extern const PORT_INF ST95HF_CS;
extern const PORT_INF ST95HF_IRQIN;*/

//#define  ST95HF_CS_Low()	RFTRANS_95HF_NSS_LOW()//st95_CS_LOW()
//#define  ST95HF_CS_High()	RFTRANS_95HF_NSS_HIGH();//st95_CS_HIGH()
/*#define ST95HF_CS_Low()         (ST95HF_CS.Port)->BRR = ST95HF_CS.Pin
#define ST95HF_CS_High()        (ST95HF_CS.Port)->BSRR = ST95HF_CS.Pin*/

//#define ST95HF_IRQIN_Low()      RFTRANS_95HF_IRQIN_LOW()//(ST95HF_IRQIN.Port)->BRR = ST95HF_IRQIN.Pin
//#define ST95HF_IRQIN_High()     RFTRANS_95HF_IRQIN_HIGH()//(ST95HF_IRQIN.Port)->BSRR = ST95HF_IRQIN.Pin

//#define ST95HF_BYTE_TIME        8
//#define ST95HF_SPI              RFTRANS_95HF_SPI//SPI1

void ST95HF_InitInterface(void);
void ST95HF_SelectInterface(void);
uint8_t ST95HF_WriteByte(uint8_t byte);
void ST95HF_Delayms(uint32_t ms);


#endif

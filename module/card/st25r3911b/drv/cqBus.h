#ifndef CHONGQINGBUS_H_
#define CHONGQINGBUS_H_

#define	err_chk( a )	do{ memset(rx , 0x00 , 256); rxLen= 0 ; if( (a) != TRUE) return FALSE;}while(0)


uint8_t st25WriteAndReadIso14443aData( uint8_t *data , uint8_t len , uint8_t *response , uint16_t *rxSize);
uint8_t st25ReadChongQingBusCardData(uint8_t *respone);

#endif


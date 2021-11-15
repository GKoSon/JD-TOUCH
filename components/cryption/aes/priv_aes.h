#ifndef _PRIV_AES_CBC_H_
#define _PRIV_AES_CBC_H_

#include <stdio.h>
#include <stdint.h>


typedef enum
{
    CONFIG_KEY = 1,
    CONFIG_IV,
}aesConfigEnum;

typedef struct 
{
	void        (*init)		    ( void );
	int32_t		(*decrypt)	    (uint8_t* inputMsg,uint16_t inputLength,uint8_t  *outputMsg,uint16_t *outputLength);
	int32_t		(*encrypt)	    (uint8_t* inputMsg,uint16_t inputLength,uint8_t  *outputMsg,uint16_t *outputLength);
    int32_t		(*decrypt_key)  (uint8_t* inputMsg,uint16_t inputLength,uint8_t  *outputMsg,uint16_t *outputLength , uint8_t *key);  
    uint8_t     (*config)   	(uint8_t mode , uint8_t *msg);
}aesType;


extern aesType aes;

#endif


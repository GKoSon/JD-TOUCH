#ifndef _AES_FUNS_H_
#define _AES_FUNS_H_

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
	uint8_t		(*decrypt)	    (uint8_t* inputMsg,uint8_t inputLength,uint8_t  *outputMsg,uint8_t *outputLength);
	uint8_t		(*encrypt)	    (uint8_t* inputMsg,uint8_t inputLength,uint8_t  *outputMsg,uint8_t *outputLength);
    uint8_t     (*decrypt_key)  (uint8_t* inputMsg,uint8_t inputLength,uint8_t  *outputMsg,uint8_t *outputLength , uint8_t *key);  
    uint8_t     (*config)   ( uint8_t mode , uint8_t *msg);
}aesType;


#define CRL_AES128_KEY   16 /*!< Number of bytes (uint8_t) necessary to store an AES key of 128 bits. */
#define CRL_AES128_EXPANDED_KEY  44 /*!< Number of ints (uint32_t) necessary to store an expanded AES key of 128 bits. */
#define CRL_AES192_KEY   24 /*!< Number of bytes (uint8_t) necessary to store an AES key of 192 bits. */
#define CRL_AES192_EXPANDED_KEY  52 /*!< Number of ints (uint32_t) necessary to store an expanded AES key of 192 bits. */
#define CRL_AES256_KEY   32 /*!< Number of bytes (uint8_t) necessary to store an AES key of 256 bits. */
#define CRL_AES256_EXPANDED_KEY  60 /*!< Number of ints (uint32_t) necessary to store an expanded AES key of 256 bits. */

extern aesType aes;

#endif


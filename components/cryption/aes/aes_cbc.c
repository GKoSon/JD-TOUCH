#include "stdint.h"
#include "stdbool.h"
#include "string.h"
#include "mbedtls/aes.h"  
#include "unit.h"
#include "aes_cbc.h"
#include "string.h"
#include <stdlib.h>

uint8_t iv[16] = { 0xA, 1, 0xB, 5, 4, 0xF, 7, 9, 0x17, 3, 1, 6, 8, 0xC, 0xD, 91 };;

//uint8_t key[16] ={'1','2','3','4','5','6','7','8','9','0','1','2','3','4','5','6'};
uint8_t key[16] ={'k','e','y','k','e','y','k','e','y','k','e','y','k','e','y','1'};     //likingfit key

void aes_cbc_init(void)
{
    //mbedtls_aes_init( &aes_ctx );  
    
    //mbedtls_aes_setkey_enc( &aes_ctx, key, 128);  
}


static uint16_t pkcs7_encode_padding(uint8_t *buf,int buflen, uint8_t *paddingBuf )
{
  uint16_t length;
  
  length = 16 - (buflen%16);
  
  memcpy(paddingBuf , buf , buflen);
    
  memset(paddingBuf +buflen , length , length);
    
  return buflen+length;

}

static uint16_t pkcs7_dncode_padding(uint8_t *buf, int buflen,uint8_t *paddingBuf )
{
  uint16_t length = 0;
  
  if(buf[buflen-1] > 16 )   return 0xFe;
  
  length = buflen - buf[buflen-1];
  
  if( length > 0 )	
  {
  	memcpy(paddingBuf , buf , length);
  }
  else
  {
	  paddingBuf[0] = 0;
	  length = 1;
  }
    
  return length;

}

uint8_t aes_crypt_cbc_enc(uint8_t* inMsg, uint16_t inLen,uint8_t  *oData,uint16_t *oLen)
{
    mbedtls_aes_context ctx; 
    uint8_t tempIv[16] , tempKey[16];
    
   // if( inLen > 240)  return false;
    *oLen = 0;
	
    memcpy(tempIv , iv , 16);
    memcpy(tempKey , key , 16);
    
    mbedtls_aes_init( &ctx );  
    mbedtls_aes_setkey_enc( &ctx, tempKey, 128); 
    
    *oLen = pkcs7_encode_padding(inMsg , inLen ,oData );
    
    if( mbedtls_aes_crypt_cbc( &ctx, MBEDTLS_AES_ENCRYPT, *oLen, tempIv, oData, oData) < 0)
    {
        log(WARN,"AES padding? ,LEN=%d\n" , *oLen);

        mbedtls_aes_free( &ctx );
        return false;
    }
        
    mbedtls_aes_free( &ctx );
    return true;
}

uint8_t aes_crypt_cbc_dec(uint8_t* inMsg, uint16_t inLen,uint8_t  *oData,uint16_t *oLen)
{
    mbedtls_aes_context ctx;  
    uint8_t rt = false;
	uint8_t tempIv[16] , tempKey[16] , *paddingBuff = NULL;
	
	paddingBuff = malloc(sizeof(uint8_t)*inLen);
	if( paddingBuff == NULL )
	{
		log_err("%sÄÚ´æ²»¹»\n" ,__func__);
		soft_system_resert(__func__);
	}
   
    memcpy(tempIv , iv , 16);
    memcpy(tempKey , key , 16);
	memset(paddingBuff , 0x00 , inLen);
	
    mbedtls_aes_init( &ctx );  
    mbedtls_aes_setkey_dec( &ctx, tempKey, 128); 
    
    //log_arry(WARN,"AES DATA " , inMsg , inLen);
    
    if( mbedtls_aes_crypt_cbc( &ctx, MBEDTLS_AES_DECRYPT, inLen, tempIv, inMsg, paddingBuff)== 0)
    {
        //log_arry(WARN,"AES DEC " , paddingBuff , inLen);
        
	  	*oLen = pkcs7_dncode_padding(paddingBuff , inLen , oData);

		if( *oLen == 0xfe)
        {
            *oLen = 0;
			log(WARN,"padding fail\n");
            goto exit;
        }
        //log_arry(WARN,"AES PAD " , oData , *oLen);
        rt =  true;
        goto exit;
    }
	
exit:
    mbedtls_aes_free( &ctx );
	free(paddingBuff);
    return  rt;
}

uint8_t aes_crypt_cbc_dec_key(uint8_t* inMsg, uint16_t inLen,uint8_t  *oData,uint16_t *oLen , uint8_t *inKey)
{
    mbedtls_aes_context ctx;  
    uint8_t rt = 0;
    uint8_t tempIv[16] , tempKey[16];
    

    memcpy(tempIv , iv , 16);
    memcpy(tempKey , inKey , 16);
    
    mbedtls_aes_init( &ctx );  
    mbedtls_aes_setkey_dec( &ctx, tempKey, 128);
    
    if( mbedtls_aes_crypt_cbc( &ctx, MBEDTLS_AES_DECRYPT, inLen, tempIv, inMsg, inMsg) == 0)
    {
	  	*oLen = pkcs7_dncode_padding(inMsg , inLen , oData);
		if( *oLen == 0xfe)
        {
            *oLen = 0;
             goto exit;
        }
        rt =  true;
        goto exit;
    }

exit:
    mbedtls_aes_free( &ctx );
    return  rt;	
}

uint8_t aes_cbc_config( uint8_t mode , uint8_t *msg)
{
    switch(mode)
    {
        case CONFIG_KEY:
        {
            memcpy(key , msg , 16);
        }break;
        default:
        {
            return false;
        }break;
    }
    
    return true;
}


aesType aes=
{
	.decrypt        = aes_crypt_cbc_dec,
	.encrypt        = aes_crypt_cbc_enc,
    .decrypt_key    = aes_crypt_cbc_dec_key,
    .config         = aes_cbc_config,
};

//INIT_EXPORT(aes_cbc_init , "aes");


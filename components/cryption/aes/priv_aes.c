#include "stdint.h"
#include "stdbool.h"
#include "string.h"
#include "unit.h"
#include "priv_aes.h"
#include "crypto.h"


//static uint8_t iv[16] = { 0xA, 1, 0xB, 5, 4, 0xF, 7, 9, 0x17, 3, 1, 6, 8, 0xC, 0xD, 91 };
uint8_t iv[CRL_AES_BLOCK] = { '1','1','1','1','1','1','1','1','1','1','1','1','1','1','1','1'};
uint8_t key[CRL_AES128_KEY] ={'1','1','1','1','1','1','1','1','1','1','1','1','1','1','1','1'};

void aes_cbc_init(void)
{

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

int32_t STM32_AES_CBC_Encrypt(uint8_t* InputMessage,
                              uint32_t InputMessageLength,
                              uint8_t  *AES128_Key,
                              uint8_t  *InitializationVector,
                              uint32_t  IvLength,
                              uint8_t  *OutputMessage,
                              uint32_t *OutputMessageLength)
{
	AESCBCctx_stt AESctx;

	uint32_t error_status = AES_SUCCESS;

	int32_t outputLength = 0;

	memset(&AESctx , 0x00 , sizeof(AESCBCctx_stt));
	
	AESctx.mFlags = E_SK_DEFAULT;

	AESctx.mKeySize = CRL_AES128_KEY;

	AESctx.mIvSize = IvLength;

	error_status = AES_CBC_Encrypt_Init(&AESctx, AES128_Key, InitializationVector );

	if (error_status == AES_SUCCESS)
	{
		error_status = AES_CBC_Encrypt_Append(&AESctx,
											  InputMessage,
											  InputMessageLength,
											  OutputMessage,
											  &outputLength);

		log_arry(DEBUG,"aaa" ,OutputMessage , outputLength);
		if (error_status == AES_SUCCESS)
		{
			*OutputMessageLength = outputLength;

			error_status = AES_CBC_Encrypt_Finish(&AESctx, OutputMessage + *OutputMessageLength, &outputLength);

			*OutputMessageLength += outputLength;
		}
	}

	return error_status;
}


int32_t aes_crypt_cbc_enc(uint8_t* inMsg, uint16_t inLen,uint8_t  *oData,uint16_t *oLen)
{

	uint32_t error_status = AES_SUCCESS , len;
	uint8_t paddingBuf[256] , paddingLen = 0;
	uint8_t tempIv[16] , tempKey[16];
    
    memcpy(tempIv , iv , 16);
    memcpy(tempKey , key , 16);
	
	uint8_t iv1[CRL_AES_BLOCK] = { '1','1','1','1','1','1','1','1','1','1','1','1','1','1','1','1'};
	uint8_t key1[CRL_AES128_KEY] ={'1','1','1','1','1','1','1','1','1','1','1','1','1','1','1','1'};

	memset(paddingBuf , 0x00 , 256);
	paddingLen = pkcs7_encode_padding(inMsg ,inLen ,  paddingBuf);
	log(DEBUG,"paddin len = %d\n" , paddingLen);
	error_status = STM32_AES_CBC_Encrypt(paddingBuf , paddingLen , key1 , iv1 , sizeof(iv1) , oData , &len);
	if( error_status == AES_SUCCESS)	
	{
		*oLen = len;
	}
	else
	{
		*oLen = 0;
	}
	return error_status;

}


int32_t STM32_AES_CBC_Decrypt(uint8_t* InputMessage,
                              uint32_t InputMessageLength,
                              uint8_t  *AES128_Key,
                              uint8_t  *InitializationVector,
                              uint32_t  IvLength,
                              uint8_t  *OutputMessage,
                              uint32_t *OutputMessageLength)
{
	AESCBCctx_stt AESctx;

	uint32_t error_status = AES_SUCCESS;

	int32_t outputLength = 0;

	/* Set flag field to default value */
	AESctx.mFlags = E_SK_DEFAULT;

	/* Set key size to 24 (corresponding to AES-192) */
	AESctx.mKeySize = 16;

	/* Set iv size field to IvLength*/
	AESctx.mIvSize = IvLength;

	/* Initialize the operation, by passing the key.
	* Third parameter is NULL because CBC doesn't use any IV */
	error_status = AES_CBC_Decrypt_Init(&AESctx, AES128_Key, InitializationVector );

	/* check for initialization errors */
	if (error_status == AES_SUCCESS)
	{
		/* Decrypt Data */
		error_status = AES_CBC_Decrypt_Append(&AESctx,
											  InputMessage,
											  InputMessageLength,
											  OutputMessage,
											  &outputLength);

		if (error_status == AES_SUCCESS)
		{
		  
			*OutputMessageLength = outputLength;

			error_status = AES_CBC_Decrypt_Finish(&AESctx, OutputMessage + *OutputMessageLength, &outputLength);

			*OutputMessageLength += outputLength;
		}
	}

	return error_status;
}

int32_t aes_crypt_cbc_dec(uint8_t* inMsg, uint16_t inLen,uint8_t  *oData,uint16_t *oLen)
{
	
	uint32_t error_status = AES_SUCCESS;
	uint8_t paddingBuff[256] ;
	uint32_t paddinglen = 0;
	uint8_t tempIv[16] , tempKey[16];
    
    memcpy(tempIv , iv , 16);
    memcpy(tempKey , key , 16);
	
	uint8_t iv1[CRL_AES_BLOCK] = { '1','1','1','1','1','1','1','1','1','1','1','1','1','1','1','1'};
	uint8_t key1[CRL_AES128_KEY] ={'1','1','1','1','1','1','1','1','1','1','1','1','1','1','1','1'};

	
	
	memset(paddingBuff , 0x00 , 256);
	error_status = STM32_AES_CBC_Decrypt(inMsg ,inLen , key1 , iv1 , sizeof(iv1) , paddingBuff , &paddinglen );
	
	if( error_status == AES_SUCCESS )
	{
		*oLen = pkcs7_dncode_padding(paddingBuff , paddinglen , oData);
		if( *oLen ==  0xFE )
		{
			*oLen = 0;
			return AES_ERR_BAD_PARAMETER;
		}
	}
	
    return  error_status;
}

int32_t aes_crypt_cbc_dec_key(uint8_t* inMsg, uint16_t inLen,uint8_t  *oData,uint16_t *oLen , uint8_t *inKey)
{

	uint32_t error_status = AES_SUCCESS;
	uint8_t paddingBuff[256];
	uint32_t paddinglen = 0;
	
	error_status = STM32_AES_CBC_Decrypt(inMsg ,inLen , inKey , iv , sizeof(iv) , paddingBuff , &paddinglen );
	
	if( error_status == AES_SUCCESS )
	{
		*oLen = pkcs7_dncode_padding(paddingBuff , paddinglen , oData);
		if( *oLen ==  0xFE )
		{
			*oLen = 0;
			return AES_ERR_BAD_PARAMETER;
		}
	}
	
    return  error_status;
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


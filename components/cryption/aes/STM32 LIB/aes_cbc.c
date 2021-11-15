#include "aes_funs.h"
#include "stdbool.h"
#include "string.h"
#include "component.h"
#include "crypto.h"

uint8_t Key[CRL_AES128_KEY] =
  {
    '1','2','3','4','5','6','7','8','9','0','1','2','3','4','5','6'
  };

uint8_t IV[CRL_AES_BLOCK] =
 {0xA, 1, 0xB, 5, 4, 0xF, 7, 9, 0x17, 3, 1, 6, 8, 0xC, 0xD, 91};

void aes_init( void );
uint8_t aes_cbc_config( uint8_t mode , uint8_t *msg);
uint8_t aes_cbc_decrypt(uint8_t* inputMsg,uint8_t inputLength,uint8_t  *outputMsg,uint8_t *outputLength);
uint8_t aes_cbc_encrypt(uint8_t* inputMsg,uint8_t inputLength,uint8_t  *outputMsg,uint8_t *outputLength);
uint8_t aes_cbc_key_decrypt(uint8_t* inputMsg,uint8_t inputLength,uint8_t  *outputMsg,uint8_t *outputLength , uint8_t *key);

aesType aes=
{
	.init           = aes_init,
	.decrypt        = aes_cbc_decrypt,
	.encrypt        = aes_cbc_encrypt,
    .decrypt_key    = aes_cbc_key_decrypt,
    .config         = aes_cbc_config,
};

/**
  * @brief  AES CBC Decryption example.
  * @param  InputMessage: pointer to input message to be decrypted.
  * @param  InputMessageLength: input data message length in byte.
  * @param  AES192_Key: pointer to the AES key to be used in the operation
  * @param  InitializationVector: pointer to the Initialization Vector (IV)
  * @param  IvLength: IV length in bytes.
  * @param  OutputMessage: pointer to output parameter that will handle the decrypted message
  * @param  OutputMessageLength: pointer to decrypted message length.
  * @retval error status: can be AES_SUCCESS if success or one of
  *         AES_ERR_BAD_INPUT_SIZE, AES_ERR_BAD_OPERATION, AES_ERR_BAD_CONTEXT
  *         AES_ERR_BAD_PARAMETER if error occured.
  */
int32_t STM32_AES_CBC_Decrypt(uint8_t* InputMessage,
                        uint8_t InputMessageLength,
                        uint8_t  *AES192_Key,
                        uint8_t  *InitializationVector,
                        uint8_t  IvLength,
                        uint8_t  *OutputMessage,
                        uint8_t *OutputMessageLength)
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
  error_status = AES_CBC_Decrypt_Init(&AESctx, AES192_Key, InitializationVector );

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
      /* Write the number of data written*/
      *OutputMessageLength = outputLength;
      /* Do the Finalization */
      error_status = AES_CBC_Decrypt_Finish(&AESctx, OutputMessage + *OutputMessageLength, &outputLength);
      /* Add data written to the information to be returned */
      *OutputMessageLength += outputLength;
    }
  }

  return error_status;
}

/**
  * @brief  AES CBC Encryption example.
  * @param  InputMessage: pointer to input message to be encrypted.
  * @param  InputMessageLength: input data message length in byte.
  * @param  AES192_Key: pointer to the AES key to be used in the operation
  * @param  InitializationVector: pointer to the Initialization Vector (IV)
  * @param  IvLength: IV length in bytes.
  * @param  OutputMessage: pointer to output parameter that will handle the encrypted message
  * @param  OutputMessageLength: pointer to encrypted message length.
  * @retval error status: can be AES_SUCCESS if success or one of
  *         AES_ERR_BAD_INPUT_SIZE, AES_ERR_BAD_OPERATION, AES_ERR_BAD_CONTEXT
  *         AES_ERR_BAD_PARAMETER if error occured.
  */
int32_t STM32_AES_CBC_Encrypt(uint8_t* InputMessage,
                        uint8_t InputMessageLength,
                        uint8_t  *AES192_Key,
                        uint8_t  *InitializationVector,
                        uint8_t  IvLength,
                        uint8_t  *OutputMessage,
                        uint8_t *OutputMessageLength)
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
    error_status = AES_CBC_Encrypt_Init(&AESctx, AES192_Key, InitializationVector );

    /* check for initialization errors */
    if (error_status == AES_SUCCESS)
    {
        /* Encrypt Data */
        error_status = AES_CBC_Encrypt_Append(&AESctx,
                                          InputMessage,
                                          InputMessageLength,
                                          OutputMessage,
                                          &outputLength);

        if (error_status == AES_SUCCESS)
        {
          /* Write the number of data written*/
          *OutputMessageLength = outputLength;
          /* Do the Finalization */
          error_status = AES_CBC_Encrypt_Finish(&AESctx, OutputMessage + *OutputMessageLength, &outputLength);
          /* Add data written to the information to be returned */
          *OutputMessageLength += outputLength;
        }
    }

    return error_status;
}




static uint8_t PKCS7_Encode_Padding(uint8_t * buf,int buflen,uint8_t * paddingBuf )
{
  uint8_t length;
  
  length = 16 - (buflen%16);
  
  memcpy(paddingBuf , buf , buflen);
    
  memset(paddingBuf +buflen , length , length);
    
  return buflen+length;

}

static uint8_t PKCS7_Dncode_Padding(uint8_t *buf, int buflen,uint8_t *paddingBuf )
{
  uint8_t length = 0;
  
  if(buf[buflen-1] > 16 )   return 0xFe;
  
  length = buflen - buf[buflen-1];
  

  memcpy(paddingBuf , buf , length);

    
  return length;

}



uint8_t aes_cbc_encrypt(uint8_t* inputMsg,uint8_t inputLength,uint8_t  *outputMsg,uint8_t *outputLength)
{
    int32_t status = AES_SUCCESS;
    
    uint8_t paddingBuff[256] , paddingLength = 0 ;
    
    if( inputLength > 240)  return false;
    
    memset(paddingBuff , 0x00 , 256);
    
    paddingLength = PKCS7_Encode_Padding(inputMsg , inputLength ,paddingBuff );
    
    STM32_AES_CBC_Encrypt(paddingBuff ,paddingLength, Key , IV, sizeof(IV), outputMsg ,outputLength );
    
    if (status == AES_SUCCESS)
    {
        return true;
    }
    else
    {

        return false;
    }
}

uint8_t aes_cbc_key_decrypt(uint8_t* inputMsg,uint8_t inputLength,uint8_t  *outputMsg,uint8_t *outputLength , uint8_t *key)
{
    int32_t status = AES_SUCCESS;    
	
    uint8_t paddingBuff[256] , paddingLength = 0 ;
	
	memset(paddingBuff , 0x00 , 256);
	
	status = STM32_AES_CBC_Decrypt( inputMsg , inputLength , key, IV, sizeof(IV), paddingBuff,&paddingLength);
	
	if (status == AES_SUCCESS)
    {
	  	*outputLength = PKCS7_Dncode_Padding(paddingBuff , paddingLength , outputMsg);
		
        if( *outputLength == 0xfe)
        {

            return false;
        }


        return true;
    }
    else
    {

        return false;
    }
}

uint8_t aes_cbc_decrypt(uint8_t* inputMsg,uint8_t inputLength,uint8_t  *outputMsg,uint8_t *outputLength)
{
    int32_t status = AES_SUCCESS;    
	
    uint8_t paddingBuff[256] , paddingLength = 0 ;
	
	memset(paddingBuff , 0x00 , 256);
	
	status = STM32_AES_CBC_Decrypt( inputMsg , inputLength , Key, IV, sizeof(IV), paddingBuff,&paddingLength);
	
	if (status == AES_SUCCESS)
    {
	  	*outputLength = PKCS7_Dncode_Padding(paddingBuff , paddingLength , outputMsg);
		if( *outputLength == 0xfe)
        {

            *outputLength = 0;
            return false;
        }

        return true;
    }
    else
    {

        return false;
    }	
}

uint8_t aes_cbc_config( uint8_t mode , uint8_t *msg)
{
    switch(mode)
    {
        case CONFIG_KEY:
        {
            memcpy(Key , msg , CRL_AES128_KEY);
        }break;
        case CONFIG_IV:
        {
            memcpy(IV , msg , CRL_AES_BLOCK);
        }break;
        default:
        {
            return false;
        }break;
    }
    
    return true;
}

void aes_init( void )
{
    Crypto_DeInit();
}

INIT_EXPORT(aes_init , "aes");


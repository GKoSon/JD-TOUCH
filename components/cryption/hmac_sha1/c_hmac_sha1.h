#ifndef _C_HMAC_SHA1_H_
#define _C_HMAC_SHA1_H_


void sha1   (  
			unsigned char *message,  
			int message_length,  
			unsigned char *digest  
			)  ;


void hmac_sha1(  
				unsigned char *key,  
				int key_length,  
				unsigned char *data,  
				int data_length,  
				unsigned char *digest  
				); 

#endif

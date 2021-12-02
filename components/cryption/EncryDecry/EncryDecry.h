#ifndef __ENCRYDECRY_H
#define __ENCRYDECRY_H

extern void makeKeyData(unsigned char *pkey ,unsigned char *pTable);
extern unsigned char intToStr(unsigned char dData);
extern unsigned char strToInt(unsigned char dData);
extern void Decryption16_32(unsigned char* SourceData,unsigned char* key,unsigned char* PurposeData,unsigned char uType);
extern void Encryption16_32(unsigned char* SourceData,unsigned char* key,unsigned char* PurposeData,unsigned char uType);
extern void Decryptionr(unsigned char* SourceData,unsigned char* key,unsigned char* PurposeData);
extern void Eecryptionr(unsigned char* SourceData,unsigned char* key,unsigned char* PurposeData);
extern void DEecryptionr_test(void);
extern void desData(unsigned char desMode,unsigned char* inData, unsigned char* outData,unsigned char (*subkey)[8]);
extern void DecryStr(unsigned char* str,unsigned char* key,unsigned char* presult);//解密函数8个字节
#endif

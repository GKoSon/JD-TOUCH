
#ifndef _CRC32_h
#define _CRC32_h

#include "stdint.h"

extern const uint32_t gdwCrc32Table[256];
//创建余式表的函数
extern void init_crc32_table(uint32_t* crc32Tbl);

//采用常量余式表的CRC32校验函数
extern uint32_t crc32(uint8_t* pucBuff, uint32_t dwLen);

//CRC32更新函数
extern void crc32_updata(uint32_t* pdwCrc32, uint8_t* pucBuff, uint32_t dwLen);

//采用变量余式表的CRC32校验函数
extern uint32_t crc32_tbl(uint32_t* pdwCrc32Tbl, uint8_t* pucBuff, uint32_t dwLen);

#endif

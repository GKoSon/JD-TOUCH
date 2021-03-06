/**
  ******************************************************************************
  * File Name          : main.c
  * Description        : Main program body
  ******************************************************************************
  * This notice applies to any and all portions of this file
  * that are not between comment pairs USER CODE BEGIN and
  * USER CODE END. Other portions of this file, whether 
  * inserted by the user or by software development tools
  * are owned by their respective copyright owners.
  *
  * Copyright (c) 2017 STMicroelectronics International N.V. 
  * All rights reserved.
  *
  * Redistribution and use in source and binary forms, with or without 
  * modification, are permitted, provided that the following conditions are met:
  *
  * 1. Redistribution of source code must retain the above copyright notice, 
  *    this list of conditions and the following disclaimer.
  * 2. Redistributions in binary form must reproduce the above copyright notice,
  *    this list of conditions and the following disclaimer in the documentation
  *    and/or other materials provided with the distribution.
  * 3. Neither the name of STMicroelectronics nor the names of other 
  *    contributors to this software may be used to endorse or promote products 
  *    derived from this software without specific written permission.
  * 4. This software, including modifications and/or derivative works of this 
  *    software, must execute solely and exclusively on microcontroller or
  *    microprocessor devices manufactured by or for STMicroelectronics.
  * 5. Redistribution and use of this software other than as permitted under 
  *    this license is void and will automatically terminate your rights under 
  *    this license. 
  *
  * THIS SOFTWARE IS PROVIDED BY STMICROELECTRONICS AND CONTRIBUTORS "AS IS" 
  * AND ANY EXPRESS, IMPLIED OR STATUTORY WARRANTIES, INCLUDING, BUT NOT 
  * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A 
  * PARTICULAR PURPOSE AND NON-INFRINGEMENT OF THIRD PARTY INTELLECTUAL PROPERTY
  * RIGHTS ARE DISCLAIMED TO THE FULLEST EXTENT PERMITTED BY LAW. IN NO EVENT 
  * SHALL STMICROELECTRONICS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
  * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
  * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, 
  * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF 
  * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING 
  * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
  * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "stm32l4xx_hal.h"
#include "cmsis_os.h"
#include "adc.h"
#include "iwdg.h"
#include "rtc.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* USER CODE BEGIN Includes */
#include "unit.h"
#include "serial\serial.h"
#include "bl_spi_flash.h"
#include "syscfg.h"
#include "crc16.h"
#include "crc32.h"
#include "mqtt_ota.h"
#include "chipFlash.h"
/* USER CODE END Includes */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
typedef  void       (*pFunction)(void);
#define             APPLICATION_ADDRESS    (uint32_t)0x0800A000

/* Private variables ---------------------------------------------------------*/
SystemConfigType    cfg;

pFunction           JumpToApplication;
uint32_t            JumpAddress;
void                *console_port = NULL;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void MX_FREERTOS_Init(void);
void MX_NVIC_Init(void);

/* USER CODE BEGIN PFP */
/* Private function prototypes -----------------------------------------------*/

/* USER CODE END PFP */

extern void spi_flash_init( void );
/* USER CODE BEGIN 0 */
#define DEBUG_LOG(type, message)                                           \
do                                                                            \
{                                                                             \
    if (type)                                                                 \
        printf message;                                                   \
}                                                                             \
while (0)
#define LOG_DEBUG                                                      0
#define INFO_DEBUG                                                     1

/*????*/ 
#define uint8_t  unsigned char
#define uint16_t unsigned short
#define uint32_t unsigned int
#define uint64_t unsigned long long

#define FASTLZ_VERSION 0x000100
#define FASTLZ_VERSION_MAJOR     0
#define FASTLZ_VERSION_MINOR     0
#define FASTLZ_VERSION_REVISION  0
#define FASTLZ_VERSION_STRING "0.1.0"
 
int fastlz_compress(const void* input, int length, void* output);
 
int fastlz_decompress(const void* input, int length, void* output, int maxout); 
 
int fastlz_compress_level(int level, const void* input, int length, void* output);
 
 
#if !defined(FASTLZ__COMPRESSOR) && !defined(FASTLZ_DECOMPRESSOR)
 
/*
 * Always check for bound when decompressing.
 * Generally it is best to leave it defined.
 */
#define FASTLZ_SAFE
 
/*
 * Give hints to the compiler for branch prediction optimization.
 */
#if defined(__GNUC__) && (__GNUC__ > 2)
#define FASTLZ_EXPECT_CONDITIONAL(c)    (__builtin_expect((c), 1))
#define FASTLZ_UNEXPECT_CONDITIONAL(c)  (__builtin_expect((c), 0))
#else
#define FASTLZ_EXPECT_CONDITIONAL(c)    (c)
#define FASTLZ_UNEXPECT_CONDITIONAL(c)  (c)
#endif
 
/*
 * Use inlined functions for supported systems.
 */
#if defined(__GNUC__) || defined(__DMC__) || defined(__POCC__) || defined(__WATCOMC__) || defined(__SUNPRO_C)
#define FASTLZ_INLINE inline
#elif defined(__BORLANDC__) || defined(_MSC_VER) || defined(__LCC__)
#define FASTLZ_INLINE __inline
#else 
#define FASTLZ_INLINE
#endif
 
/*
 * Prevent accessing more than 8-bit at once, except on x86 architectures.
 */
#if !defined(FASTLZ_STRICT_ALIGN)
#define FASTLZ_STRICT_ALIGN
#if defined(__i386__) || defined(__386)  /* GNU C, Sun Studio */
#undef FASTLZ_STRICT_ALIGN
#elif defined(__i486__) || defined(__i586__) || defined(__i686__) /* GNU C */
#undef FASTLZ_STRICT_ALIGN
#elif defined(_M_IX86) /* Intel, MSVC */
#undef FASTLZ_STRICT_ALIGN
#elif defined(__386)
#undef FASTLZ_STRICT_ALIGN
#elif defined(_X86_) /* MinGW */
#undef FASTLZ_STRICT_ALIGN
#elif defined(__I86__) /* Digital Mars */
#undef FASTLZ_STRICT_ALIGN
#endif
#endif
 
/*
 * FIXME: use preprocessor magic to set this on different platforms!
 */
typedef unsigned char  flzuint8;
typedef unsigned short flzuint16;
typedef unsigned int   flzuint32;
 
/* prototypes */
int fastlz_compress(const void* input, int length, void* output);
int fastlz_compress_level(int level, const void* input, int length, void* output);
int fastlz_decompress(const void* input, int length, void* output, int maxout);
 
#define MAX_COPY       32
#define MAX_LEN       264  /* 256 + 8 */
#define MAX_DISTANCE 8192
 
#if !defined(FASTLZ_STRICT_ALIGN)
#define FASTLZ_READU16(p) *((const flzuint16*)(p)) 
#else
#define FASTLZ_READU16(p) ((p)[0] | (p)[1]<<8)
#endif
 
#define HASH_LOG  10
#define HASH_SIZE (1<< HASH_LOG)
#define HASH_MASK  (HASH_SIZE-1)
#define HASH_FUNCTION(v,p) { v = FASTLZ_READU16(p); v ^= FASTLZ_READU16(p+1)^(v>>(16-HASH_LOG));v &= HASH_MASK; }
 
 
 
 
FASTLZ_INLINE int fastlz1_compress(const void* input, int length, void* output)
{
 
  const flzuint8* ip = (const flzuint8*) input;
  const flzuint8* ip_bound = ip + length - 2;
  const flzuint8* ip_limit = ip + length - 12;
  flzuint8* op = (flzuint8*) output;
 
  const flzuint8* htab[HASH_SIZE];
 
  const flzuint8** hslot;
  flzuint32 hval;
 
  flzuint32 copy;
 
  /* sanity check */
  if(FASTLZ_UNEXPECT_CONDITIONAL(length < 4))
  {
    if(length)
    {
      /* create literal copy only */
      *op++ = length-1;
      ip_bound++;
      while(ip <= ip_bound)
        *op++ = *ip++;
      return length+1;
    }
    else
      return 0;
  }
 
  /* initializes hash table */
for (hslot = htab; hslot < htab + HASH_SIZE; hslot++)
    *hslot = ip;
 
 
  /* we start with literal copy */
  copy = 2;
  *op++ = MAX_COPY-1;
  *op++ = *ip++;
  *op++ = *ip++;
 
  /* main loop */
  while(FASTLZ_EXPECT_CONDITIONAL(ip < ip_limit))
  {
    const flzuint8* ref;
    flzuint32 distance;
 
    /* minimum match length */
    flzuint32 len = 3;
 
    /* comparison starting-point */
    const flzuint8* anchor = ip;
 
 
 
    /* find potential match */
    HASH_FUNCTION(hval,ip);
    hslot = htab + hval;
    ref = htab[hval];
 
    /* calculate distance to the match */
    distance = anchor - ref;
 
    /* update hash table */
    *hslot = anchor;
 
    /* is this a match? check the first 3 bytes */
    if(distance==0 || 
 
    (distance >= MAX_DISTANCE) ||
 
    *ref++ != *ip++ || *ref++!=*ip++ || *ref++!=*ip++)
      goto literal;
 
 
    /* last matched byte */
    ip = anchor + len;
 
    /* distance is biased */
    distance--;
 
    if(!distance)
    {
      /* zero distance means a run */
      flzuint8 x = ip[-1];
      while(ip < ip_bound)
        if(*ref++ != x) break; else ip++;
    }
    else
    for(;;)
    {
      /* safe because the outer check against ip limit */
      if(*ref++ != *ip++) break;
      if(*ref++ != *ip++) break;
      if(*ref++ != *ip++) break;
      if(*ref++ != *ip++) break;
      if(*ref++ != *ip++) break;
      if(*ref++ != *ip++) break;
      if(*ref++ != *ip++) break;
      if(*ref++ != *ip++) break;
      while(ip < ip_bound)
        if(*ref++ != *ip++) break;
      break;
    }
 
    /* if we have copied something, adjust the copy count */
    if(copy)
      /* copy is biased, '0' means 1 byte copy */
      *(op-copy-1) = copy-1;
    else
      /* back, to overwrite the copy count */
      op--;
 
    /* reset literal counter */
    copy = 0;
 
    /* length is biased, '1' means a match of 3 bytes */
    ip -= 3;
    len = ip - anchor;
 
 
    if(FASTLZ_UNEXPECT_CONDITIONAL(len > MAX_LEN-2))
      while(len > MAX_LEN-2)
      {
        *op++ = (7 << 5) + (distance >> 8);
        *op++ = MAX_LEN - 2 - 7 -2; 
        *op++ = (distance & 255);
        len -= MAX_LEN-2;
      }
 
    if(len < 7)
    {
      *op++ = (len << 5) + (distance >> 8);
      *op++ = (distance & 255);
    }
    else
    {
      *op++ = (7 << 5) + (distance >> 8);
      *op++ = len - 7;
      *op++ = (distance & 255);
    }
 
 
    /* update the hash at match boundary */
    HASH_FUNCTION(hval,ip);
    htab[hval] = ip++;
    HASH_FUNCTION(hval,ip);
    htab[hval] = ip++;
 
    /* assuming literal copy */
    *op++ = MAX_COPY-1;
 
    continue;
 
    literal:
      *op++ = *anchor++;
      ip = anchor;
      copy++;
      if(FASTLZ_UNEXPECT_CONDITIONAL(copy == MAX_COPY))
      {
        copy = 0;
        *op++ = MAX_COPY-1;
      }
  }
 
  /* left-over as literal copy */
  ip_bound++;
  while(ip <= ip_bound)
  {
    *op++ = *ip++;
    copy++;
    if(copy == MAX_COPY)
    {
      copy = 0;
      *op++ = MAX_COPY-1;
    }
  }
 
  /* if we have copied something, adjust the copy length */
  if(copy)
    *(op-copy-1) = copy-1;
  else
    op--;
 
 
 
  return op - (flzuint8*)output;
}
 
static FASTLZ_INLINE int fastlz1_decompress(const void* input, int length, void* output, int maxout)
{
  const flzuint8* ip = (const flzuint8*) input;
  const flzuint8* ip_limit  = ip + length;
  flzuint8* op = (flzuint8*) output;
  flzuint8* op_limit = op + maxout;
  flzuint32 ctrl = (*ip++) & 31;
  int loop = 1;
 
  do
  {
    const flzuint8* ref = op;
    flzuint32 len = ctrl >> 5;
    flzuint32 ofs = (ctrl & 31) << 8;
 
    if(ctrl >= 32)
    {
 
      len--;
      ref -= ofs;
      if (len == 7-1)
 
        len += *ip++;
      ref -= *ip++;
 
      
 
      if (FASTLZ_UNEXPECT_CONDITIONAL(op + len + 3 > op_limit))
        return 0;
 
      if (FASTLZ_UNEXPECT_CONDITIONAL(ref-1 < (flzuint8 *)output))
        return 0;
 
 
      if(FASTLZ_EXPECT_CONDITIONAL(ip < ip_limit))
        ctrl = *ip++;
      else
        loop = 0;
 
      if(ref == op)
      {
        /* optimize copy for a run */
        flzuint8 b = ref[-1];
        *op++ = b;
        *op++ = b;
        *op++ = b;
        for(; len; --len)
          *op++ = b;
      }
      else
      {
#if !defined(FASTLZ_STRICT_ALIGN)
        const flzuint16* p;
        flzuint16* q;
#endif
        /* copy from reference */
        ref--;
        *op++ = *ref++;
        *op++ = *ref++;
        *op++ = *ref++;
 
#if !defined(FASTLZ_STRICT_ALIGN)
        /* copy a byte, so that now it's word aligned */
        if(len & 1)
        {
          *op++ = *ref++;
          len--;
        }
 
        /* copy 16-bit at once */
        q = (flzuint16*) op;
        op += len;
        p = (const flzuint16*) ref;
        for(len>>=1; len > 4; len-=4)
        {
          *q++ = *p++;
          *q++ = *p++;
          *q++ = *p++;
          *q++ = *p++;
        }
        for(; len; --len)
          *q++ = *p++;
#else
        for(; len; --len)
          *op++ = *ref++;
#endif
      }
    }
    else
    {
      ctrl++;
 
      if (FASTLZ_UNEXPECT_CONDITIONAL(op + ctrl > op_limit))
        return 0;
      if (FASTLZ_UNEXPECT_CONDITIONAL(ip + ctrl > ip_limit))
        return 0;
 
 
      *op++ = *ip++; 
      for(--ctrl; ctrl; ctrl--)
        *op++ = *ip++;
 
      loop = FASTLZ_EXPECT_CONDITIONAL(ip < ip_limit);
      if(loop)
        ctrl = *ip++;
    }
  }
  while(FASTLZ_EXPECT_CONDITIONAL(loop));
 
  return op - (flzuint8*)output;
}
 
 
int fastlz_compress(const void* input, int length, void* output)
{
    return fastlz1_compress(input, length, output);
}
 
int fastlz_decompress(const void* input, int length, void* output, int maxout)
{
    return fastlz1_decompress(input, length, output, maxout);
}
 
int fastlz_compress_level(int level, const void* input, int length, void* output)
{
  return 0;
}
 
#else /* !defined(FASTLZ_COMPRESSOR) && !defined(FASTLZ_DECOMPRESSOR) */
 
#endif /* !defined(FASTLZ_COMPRESSOR) && !defined(FASTLZ_DECOMPRESSOR) */
/*????*/

void console_getchar(uint8_t ch)
{

}

void serial_console_init( void )
{
    console_port = serial.open("serial1");
    
    if( console_port == NULL)
    {
        //beep.write(BEEP_ALARM);
        return ;
    }
    serial.init(console_port  , 921600 , console_getchar);
}


void timer_isr( void )
{

}

uint8_t buf[4096] @(0x10000000);
//https://blog.csdn.net/weixin_42381351/article/details/103492766
void FLASH_BufferRead_RANG( uint32_t ReadAddr,uint8_t* pBuffer, uint16_t NumByteToRead)
{
    uint32_t i,p1Addr,p2Addr,p1len,p2len,p1lenno,p2lenno;
    uint8_t  status=0;
       
    if(NumByteToRead>4096){printf("--------NOW NOT---------\r\n");return;}
    for(i=0;i<2049;i++)//???????? ??????2049??PAGE
    {
        p1Addr = i*4096;
        p2Addr = p1Addr + 4096;
        if(ReadAddr==p1Addr){printf("--------RIGHT GOOG A PAGE---------\r\n");status=1;break;}
        if( (ReadAddr<p2Addr)&& (ReadAddr>p1Addr)){/*printf("-------NEED TWO PAGE-----\r\n");*/status=2;break;}
    }
    if(status==0){printf("---------FUCK---------\r\n");return;}
 
    if(status==1)
    {flash.read(ReadAddr,pBuffer,NumByteToRead);return;}
    if(status==2)
    {
        p1lenno = ReadAddr - p1Addr;
        p1len = 4096 - p1lenno;
        if(NumByteToRead  > p1len)
        {
            p2len = NumByteToRead - p1len;
            p2lenno = 4096 - p2len;
 
            flash.read(p1Addr,buf,4096);
            memcpy(pBuffer,       &buf[p1lenno],  p1len);
 
            flash.read(p2Addr,buf,4096);
            memcpy(&pBuffer[p1len],buf,           p2len);
 
            //printf("-----TWO READ OK[%d][%d][%d][%d]----\r\n",p1lenno,p1len,p2len,p2lenno);
        }
        else
        {
            flash.read(p1Addr,buf,4096);
            memcpy(pBuffer,       &buf[p1lenno],  NumByteToRead);
            //printf("------ONE PAG OK-----\r\n");
        }
    }
}


uint16_t outlen[50]={0};
uint8_t  totalcnt=0;
uint32_t hexlen =0;
uint16_t bincrc16 ;
uint8_t write[4096]    @(0x10001000);
uint8_t read[4096]     @(0x10002000);

void zip2hex(void)
{
    DEBUG_LOG(INFO_DEBUG, ("KOSON ????????????\r\n"));
    
    uint16_t lenmark[50],*plenmark=NULL;
    uint8_t  i=0;
    uint32_t addrpagefrom=0,addrpageto=0;

    memset(read,0,4096);
    addrpagefrom = OTA_ZIP_START_ADDR+i*4096;  
    flash.read(addrpagefrom,read,50*2);   
    plenmark = (uint16_t *)&read;
    bincrc16 = plenmark[0];
    uint16_t zipnum   = plenmark[1];
    DEBUG_LOG(INFO_DEBUG, ("KOSON totalcnt zip=%d\r\n",zipnum));
    for(i=0;i<zipnum;i++)
    {  
        lenmark[i] = plenmark[i+2]; 
        DEBUG_LOG(INFO_DEBUG, ("KOSON lenmark[%d]=%d\r\n",i,lenmark[i]));
    }
    totalcnt = zipnum;

    //????????????
    addrpagefrom += (2* (1+1+totalcnt)); /*????U16??CRC ????U16?????? N??U16????????????*/
    for(i=0;i<totalcnt;i++)
    {
        memset(read,0,4096);//ZIP
        memset(write,0,4096);//HEX
        FLASH_BufferRead_RANG(addrpagefrom,read,lenmark[i]);
        outlen[i] = fastlz_decompress(read, lenmark[i],write, 4096); 
        DEBUG_LOG(INFO_DEBUG, ("KOSON outlen[%d]=%d\r\n",i,outlen[i]));
        addrpagefrom += lenmark[i]; 
        addrpageto   = OTA_HEX_START_ADDR+i*4096;
        flash.earse(addrpageto);/*????*/
        flash.write(addrpageto, write,  4096 );
        hexlen += outlen[i];
    }

    DEBUG_LOG(INFO_DEBUG, ("KOSON hexlen=%d\r\n",hexlen));
}


unsigned short wCRCin = 0x0000;
unsigned short CRC16_CCITT_ONE(unsigned char *puchMsg, unsigned int usDataLen)  
{  
  
  unsigned short wCPoly = 0x1021;  
  unsigned char wChar = 0;  
  int i;
  while (usDataLen--)     
  {  
        wChar = *(puchMsg++);  
 
        wCRCin ^= (wChar << 8);  
        for(i = 0;i < 8;i++)  
        {  
          if(wCRCin & 0x8000)  
            wCRCin = (wCRCin << 1) ^ wCPoly;  
          else  
            wCRCin = wCRCin << 1;  
        }  
  }  
  
  return (wCRCin) ;  
}


uint16_t get_bin_crc16(void)
{

    uint32_t addrpagefrom=0;
 
    for(uint8_t i=0;i<totalcnt;i++)
    {
        addrpagefrom = OTA_HEX_START_ADDR+i*4096;
        memset(read,0,4096);
        flash.read(addrpagefrom,read,outlen[i]);
        CRC16_CCITT_ONE((uint8_t*)&read,outlen[i]);
        DEBUG_LOG(LOG_DEBUG, ("KOSON addrpagefrom=0X%08X , outlen = %d\n" , addrpagefrom , outlen[i]));
    }
    
    return wCRCin;

}

uint8_t ota_ver_file( void )
{
    zip2hex();
    
    wCRCin=0;
    if( bincrc16 == get_bin_crc16())
    {
        log(WARN,"??????????????bincrc16=0X%04X , get_bin_crc16 = 0X%04X\n" , bincrc16 , wCRCin);
        return TRUE;
    }

    log(WARN,"??????????????bincrc16=0X%04X , get_bin_crc16 = 0X%04X\n" , bincrc16 , wCRCin);

    
    return FALSE;

}

uint8_t sys_cfg_read(SystemConfigType *data)
{
    return (chip_flash_read( DSYS_CFG_ADDR , (uint8_t *)data , sizeof(SystemConfigType) ));
}
uint8_t sys_cfg_write(SystemConfigType *data)
{
  
    return ( chip_flash_write( DSYS_CFG_ADDR , (uint8_t *)data , sizeof(SystemConfigType) ));
}

void sysCfg_save( void )
{

    cfg.crc16 = 0;
    
    memset(fb , 0xFF , sizeof(fb));
    memcpy(fb , &cfg , sizeof(SystemConfigType));
    
    cfg.crc16 =  crc16_ccitt(fb , sizeof(fb));
    
    sys_cfg_write(&cfg);
}

uint8_t read_sys_cfg( void )
{
    uint16_t crc , calcCrc;
    uint8_t cnt = 20;
    
    do
    {
        sys_cfg_read(&cfg);
        if((cfg.crc16 == 0xFFFF) && (cfg.mark == 0xFFFFFFFF))
        {
            log(WARN,"[BOOT]????sys_cfg_read????FF ????????OTA????????\n");
            
            return FALSE;
        }
        crc = cfg.crc16;
        cfg.crc16 = 0;
        memset(fb , 0xFF , sizeof(fb));
        memcpy(fb , &cfg , sizeof(SystemConfigType));
        
        calcCrc =  crc16_ccitt(fb , sizeof(fb));

        log(ERR,"[BOOT]????????????????,??????CRC16 = %x, ????????cfg.crc16 = %x \n" , calcCrc , crc);
  
    }while((calcCrc != crc)&& (--cnt) );

    if( cnt ==0 )
    {
        log(WARN,"[BOOT]?????????????? ,????????????????????  return FALSE\n");

        return FALSE;
    }

    return TRUE;
}

/**
  * @brief  Unlocks Flash for write access
  * @param  None
  * @retval None
  */
static void FLASH_Init(void)
{
  /* Unlock the Program memory */
  HAL_FLASH_Unlock();

  /* Clear all FLASH flags */
  __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_PGSERR | FLASH_FLAG_WRPERR | FLASH_FLAG_OPTVERR);
  /* Unlock the Program memory */
  HAL_FLASH_Lock();
}

/**
  * @brief  Gets the bank of a given address
  * @param  Addr: Address of the FLASH Memory
  * @retval The bank of a given address
  */
static uint32_t GetBank(uint32_t Addr)
{
    uint32_t bank = 0;

    if (READ_BIT(SYSCFG->MEMRMP, SYSCFG_MEMRMP_FB_MODE) == 0)
    {
        /* No Bank swap */
        if (Addr < (FLASH_BASE + FLASH_BANK_SIZE))
        {
            bank = FLASH_BANK_1;
        }
        else
        {
            bank = FLASH_BANK_2;
        }
    }
    else
    {
        /* Bank swap */
        if (Addr < (FLASH_BASE + FLASH_BANK_SIZE))
        {
            bank = FLASH_BANK_2;
        }
        else
        {
            bank = FLASH_BANK_1;
        }
    }

    return bank;
}

/**
  * @brief  Gets the page of a given address
  * @param  Addr: Address of the FLASH Memory
  * @retval The page of a given address
  */
static uint32_t GetPage(uint32_t Addr)
{
  uint32_t page = 0;
  
  if (Addr < (FLASH_BASE + FLASH_BANK_SIZE))
  {
    /* Bank 1 */
    page = (Addr - FLASH_BASE) / FLASH_PAGE_SIZE;
  }
  else
  {
    /* Bank 2 */
    page = (Addr - (FLASH_BASE + FLASH_BANK_SIZE)) / FLASH_PAGE_SIZE;
  }
  
  return page;
}

/**
  * @brief  This function does an erase of all user flash area
  * @param  start: start of user flash area
  * @retval FLASHIF_OK : user flash area successfully erased
  *         FLASHIF_ERASEKO : error occurred
  */
static uint32_t FLASH_Erase( void )
{
  uint32_t FirstPage = 0, BankNumber = 0 ,PAGEError = 0;;
  FLASH_EraseInitTypeDef EraseInitStruct;
  
  /* Unlock the Flash to enable the flash control register access *************/
  HAL_FLASH_Unlock();

  /* Erase the user Flash area
    (area defined by FLASH_USER_START_ADDR and FLASH_USER_END_ADDR) ***********/

  /* Clear OPTVERR bit set on virgin samples */
  __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_OPTVERR); 
  /* Get the 1st page to erase */
  FirstPage = GetPage(APPLICATION_ADDRESS);

  /* Get the bank */
  BankNumber = GetBank(APPLICATION_ADDRESS);
  
  
    /* Fill EraseInit structure*/
    EraseInitStruct.TypeErase   = FLASH_TYPEERASE_PAGES;
    EraseInitStruct.Banks       = BankNumber;
    EraseInitStruct.Page        = FirstPage;
    EraseInitStruct.NbPages     = 128-FirstPage;

    /* Note: If an erase operation in Flash memory also concerns data in the data or instruction cache,
     you have to make sure that these data are rewritten before they are accessed during code
     execution. If this cannot be done safely, it is recommended to flush the caches by setting the
     DCRST and ICRST bits in the FLASH_CR register. */
    if (HAL_FLASHEx_Erase(&EraseInitStruct, &PAGEError) != HAL_OK)
    {
     HAL_FLASH_Lock();
     return FLASHIF_ERASEKO;
    }

    HAL_IWDG_Refresh(&hiwdg);
    /* Fill EraseInit structure*/
    EraseInitStruct.TypeErase   = FLASH_TYPEERASE_PAGES;
    EraseInitStruct.Banks       = GetBank(0x08040000);
    EraseInitStruct.Page        = GetPage(0x08040000);
    EraseInitStruct.NbPages     = GetPage(USER_FLASH_END_ADDRESS);

    /* Note: If an erase operation in Flash memory also concerns data in the data or instruction cache,
     you have to make sure that these data are rewritten before they are accessed during code
     execution. If this cannot be done safely, it is recommended to flush the caches by setting the
     DCRST and ICRST bits in the FLASH_CR register. */
    if (HAL_FLASHEx_Erase(&EraseInitStruct, &PAGEError) != HAL_OK)
    {
     HAL_FLASH_Lock();
     return FLASHIF_ERASEKO;
    }
      

  
  
  /* Lock the Flash to disable the flash control register access (recommended
     to protect the FLASH memory against possible unwanted operation) *********/
  HAL_FLASH_Lock();
  
  return FLASHIF_OK;
}

static uint32_t FLASH_Write(uint32_t destination, uint32_t *p_source, uint32_t length)
{
    uint32_t status = FLASHIF_OK;
    uint32_t i = 0;

    /* Unlock the Flash to enable the flash control register access *************/
    HAL_FLASH_Unlock();

    /* DataLength must be a multiple of 64 bit */
    for (i = 0; (i < length/2) && (destination <= (USER_FLASH_END_ADDRESS-8)); i++)
    {
        /* Device voltage range supposed to be [2.7V to 3.6V], the operation will
        be done by word */ 
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, destination, *((uint64_t *)(p_source+2*i))) == HAL_OK)      
        {
            /* Check the written value */
            if (*(uint64_t*)destination != *(uint64_t *)(p_source+2*i))
            {
                /* Flash content doesn't match SRAM content */
                status = FLASHIF_WRITINGCTRL_ERROR;
                break;
            }
            /* Increment FLASH destination address */
            destination += 8;
        }
        else
        {
            /* Error occurred while writing data in Flash memory */
            status = FLASHIF_WRITING_ERROR;
            break;
        }
    }

    /* Lock the Flash to disable the flash control register access (recommended
    to protect the FLASH memory against possible unwanted operation) *********/
    HAL_FLASH_Lock();

    return status;
}

int8_t bootload_download_to_flash( void )
{
    uint32_t baseAddr = APPLICATION_ADDRESS;
    uint32_t readAddr = OTA_HEX_START_ADDR;//????????
    uint32_t ramsource , len =0;
    cfg.otaVar.fileSize = hexlen;//???????? ????ZIP??APP???? ??????
    uint8_t buff[1024]; 

    FLASH_Erase();
    
    for( len = 0;  len < cfg.otaVar.fileSize; len += 1024 )
    {
        flash.read(readAddr+len , buff , 1024);
       // if(len == 0)printf("%08x",(uint32_t*)&buff[0]);//??????????
        ramsource = (uint32_t)&buff;
        
        if( FLASH_Write(baseAddr+len ,  (uint32_t*) ramsource , 256) != FLASHIF_OK)
        {
            log(ERR,"???????????? , ????????\n");
            NVIC_SystemReset();
        }
        printf("...");
        HAL_IWDG_Refresh(&hiwdg);
    }
    
    log(INFO,"????????");
    
    
    return TRUE;
}

void clear_ota_mark( void )
{
    cfg.otaVar.otaUpgMark = 0;
    cfg.otaVar.crc32 = 0;
    cfg.otaVar.fileSize =0;
    cfg.otaVar.ver =0;
    sysCfg_save();
}

uint8_t bootloader_iap( void )
{
    if( read_sys_cfg() == FALSE)
    {
        return FALSE;
    }
    log(INFO,"cfg.otaVar.otaUpgMark=%X UPG_MARK=%X\n",cfg.otaVar.otaUpgMark,UPG_MARK);
    if( cfg.otaVar.otaUpgMark == UPG_MARK)
    {
        if( cfg.otaVar.fileSize == 0)
        {
            log(ERR,"??????????0 ?? ??????????????????????????????????????????\n");
            clear_ota_mark();
            return FALSE;
        }
    
        log(INFO,"????????????\n");
        log(DEBUG,"*****************************************************\n");
        log(DEBUG,"???????? = %d , ???? crc32 = %u ,?????? = %d\n" , cfg.otaVar.fileSize , cfg.otaVar.crc32 , cfg.otaVar.ver);
        log(DEBUG,"*****************************************************\n");
        log(INFO,"??????????????????\n");
        if( ota_ver_file() == FALSE)
        {
            log(INFO,"??????????????????Flash\n");
            clear_ota_mark();
            return FALSE;
        }
        log(INFO,"??????????????????????????????????\n");
        if( bootload_download_to_flash() == TRUE)
        {
            cfg.parm.soft_version=cfg.otaVar.ver;
            clear_ota_mark();
            log(INFO,"????????\n");
            return TRUE;
        }
        
    }
    return FALSE;
}

void jump_application( void )
{
    if (((*(__IO uint32_t*)APPLICATION_ADDRESS) & 0x1FFE0000 ) == 0x10000000) //stack use ram1
    {
      log(DEBUG,"[BOOT]??????????????\n\n\n");
      /* Jump to user application */
      __set_PRIMASK(1); //must be close en all
      JumpAddress = *(__IO uint32_t*) (APPLICATION_ADDRESS + 4);
      JumpToApplication = (pFunction) JumpAddress;
      /* Initialize user application's Stack Pointer */
      __set_MSP(*(__IO uint32_t*) APPLICATION_ADDRESS);
      JumpToApplication();
    } 
    else  
    {  
        log(DEBUG,"[BOOT]????????????\n");  
    } 
}

/* USER CODE END 0 */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration----------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */ 

  MX_GPIO_Init();
  MX_RTC_Init();
  MX_SPI1_Init();
  MX_USART1_UART_Init();
  MX_IWDG_Init();
  //MX_TIM7_Init();
  /* Initialize interrupts */
  //MX_NVIC_Init();
  FLASH_Init();
  /* USER CODE BEGIN 2 */
  serial_console_init();
  spi_flash_init();
  log(DEBUG,"\n");
  log(DEBUG,"********************* 2021 Copyright boot********************* \n");
  log(DEBUG,"????????????????????: %s %s .\r\n" ,__DATE__,__TIME__);
  log(DEBUG,"??????????????????????????\n");
  bootloader_iap();
  
  jump_application();
  /* USER CODE END 2 */


  /* We should never get here as control is now taken by the scheduler */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
  /* USER CODE END WHILE */
    log(DEBUG,"boot i am ok!!!\n");
    HAL_Delay(1000);
  /* USER CODE BEGIN 3 */

  }
  /* USER CODE END 3 */

}

/** System Clock Configuration
*/
void SystemClock_Config(void)
{

  RCC_OscInitTypeDef RCC_OscInitStruct;
  RCC_ClkInitTypeDef RCC_ClkInitStruct;
  RCC_PeriphCLKInitTypeDef PeriphClkInit;

    /**Configure LSE Drive Capability 
    */
  __HAL_RCC_LSEDRIVE_CONFIG(RCC_LSEDRIVE_LOW);

    /**Initializes the CPU, AHB and APB busses clocks 
    */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSI|RCC_OSCILLATORTYPE_HSE
                              |RCC_OSCILLATORTYPE_LSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.LSEState = RCC_LSE_ON;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 1;
  RCC_OscInitStruct.PLL.PLLN = 20;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV7;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

    /**Initializes the CPU, AHB and APB busses clocks 
    */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_RTC|RCC_PERIPHCLK_USART1
                              |RCC_PERIPHCLK_USART2|RCC_PERIPHCLK_USART3
                              |RCC_PERIPHCLK_UART4|RCC_PERIPHCLK_UART5
                              |RCC_PERIPHCLK_ADC;
  PeriphClkInit.Usart1ClockSelection = RCC_USART1CLKSOURCE_PCLK2;
  PeriphClkInit.Usart2ClockSelection = RCC_USART2CLKSOURCE_PCLK1;
  PeriphClkInit.Usart3ClockSelection = RCC_USART3CLKSOURCE_PCLK1;
  PeriphClkInit.Uart4ClockSelection = RCC_UART4CLKSOURCE_PCLK1;
  PeriphClkInit.Uart5ClockSelection = RCC_UART5CLKSOURCE_PCLK1;
  PeriphClkInit.AdcClockSelection = RCC_ADCCLKSOURCE_PLLSAI1;
  PeriphClkInit.RTCClockSelection = RCC_RTCCLKSOURCE_LSE;
  PeriphClkInit.PLLSAI1.PLLSAI1Source = RCC_PLLSOURCE_HSE;
  PeriphClkInit.PLLSAI1.PLLSAI1M = 1;
  PeriphClkInit.PLLSAI1.PLLSAI1N = 16;
  PeriphClkInit.PLLSAI1.PLLSAI1P = RCC_PLLP_DIV7;
  PeriphClkInit.PLLSAI1.PLLSAI1Q = RCC_PLLQ_DIV2;
  PeriphClkInit.PLLSAI1.PLLSAI1R = RCC_PLLR_DIV2;
  PeriphClkInit.PLLSAI1.PLLSAI1ClockOut = RCC_PLLSAI1_ADC1CLK;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

    /**Configure the main internal regulator output voltage 
    */
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

    /**Configure the Systick interrupt time 
    */
  HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq()/1000);

    /**Configure the Systick 
    */
  HAL_SYSTICK_CLKSourceConfig(SYSTICK_CLKSOURCE_HCLK);

  /* SysTick_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(SysTick_IRQn, 15, 0);
}

/** NVIC Configuration
*/
void MX_NVIC_Init(void)
{
  /* EXTI9_5_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(EXTI9_5_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);
  /* USART2_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(USART2_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(USART2_IRQn);
  /* USART3_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(USART3_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(USART3_IRQn);
  /* EXTI15_10_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
  /* RTC_Alarm_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(RTC_Alarm_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(RTC_Alarm_IRQn);
  /* USART1_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(USART1_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(USART1_IRQn);
}

/* USER CODE BEGIN 4 */

int fputc(int ch, FILE *f)
{
    if(ch == '\n')
    {
        serial.putc(console_port,'\r');
    }
    
    serial.putc(console_port, ch);
    
    return ch;
}

uint16_t osGetCPUUsage(void)
{
    return 0;
}


/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @param  None
  * @retval None
  */
void _Error_Handler(char * file, int line)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  while(1) 
  {
  }
  /* USER CODE END Error_Handler_Debug */ 
}

#ifdef USE_FULL_ASSERT

/**
   * @brief Reports the name of the source file and the source line number
   * where the assert_param error has occurred.
   * @param file: pointer to the source file name
   * @param line: assert_param error line source number
   * @retval None
   */
void assert_failed(uint8_t* file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
    ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */

}

#endif

/**
  * @}
  */ 

/**
  * @}
*/ 

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

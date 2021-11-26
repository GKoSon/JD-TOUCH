#include <stdio.h>
#include <string.h>
#include <stdlib.h>//free
 
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
 
 

int main(int argc,char **argv)
{
  #define INLEN  4096
	int lSize=0,i=0;
	FILE *fptarget = NULL;

	fptarget = fopen("1.bin", "rb");
	fseek(fptarget,0,SEEK_END);//光标到尾巴
  lSize = ftell(fptarget);//光标的位置就是长度了 获得文本长度


  uint8_t readcnt     = (lSize/INLEN)?(lSize/INLEN+1):(lSize/INLEN);
	uint16_t lastpaklen = lSize - ((readcnt-1)* INLEN);
	int outlen=0;
	uint16_t lenmark[50]={0}; 
	uint8_t indata[INLEN];
	uint8_t outdata[INLEN+512];
	printf("lSize %d readcnt %d lastpaklen %d\r\n",lSize,readcnt,lastpaklen);
	FILE *fpLZ  = fopen("1.bin", "rb");
	FILE *fpnew = fopen("1_zip.bin", "wb");
	
  for(i=0;i<readcnt-1;i++)
  {
		outlen = 0;
		memset(indata,0,sizeof(indata));
		memset(outdata,0,sizeof(outdata));
 
		fread (indata, sizeof(char), INLEN, fpLZ);
		outlen = fastlz_compress(indata,INLEN,outdata);
		printf("inputlen=%d -- outlen=%d [%d]\r\n",INLEN,outlen,outlen*100/INLEN);
		lenmark[i]=outlen;
    fwrite(outdata, sizeof(char), outlen, fpnew);
	}
	printf("lastone\r\n");
	i = readcnt-1;
	outlen = 0;
	memset(indata,0,sizeof(indata));
	memset(outdata,0,sizeof(outdata));
	fread (indata, sizeof(char), lastpaklen, fpLZ);
	outlen = fastlz_compress(indata,lastpaklen,outdata);
  printf("inputlen=%d -- outlen=%d [%d]\r\n",lastpaklen,outlen,outlen*100/lastpaklen);
	lenmark[i]=outlen;
	fwrite(outdata, sizeof(char), outlen, fpnew);
	
//fwrite(lenmark, sizeof(char), 50*2, fpnew);//此时把压缩后的bin文件在最后追加这个数组！我们不放在最后放在最前
	

	outlen=0;//展示一下压缩效果 打印出来
  for(i=0;i<50;i++)outlen+=lenmark[i];
  printf("ALL olen=%d newlen=%d [%d]",lSize,outlen,outlen*100/lSize);   
  fclose(fpLZ);
	//free(fpLZ);
	fclose(fpnew);
	//free(fpnew);
 
	
	uint8_t	data2buf[200*1024];
	FILE *fpolddel  =  fopen("1_zip.bin",  "rb");
	FILE *fpnewout  =  fopen("outzip.bin", "wb");
	fwrite(lenmark, sizeof(char), 50*2, fpnewout);//此时把压缩后的bin文件在头部写入这个数组！
	memset(data2buf,0,200*1024);
	fread (data2buf, sizeof(char), outlen, fpolddel);
	fwrite(data2buf, sizeof(char), outlen, fpnewout);	//outlen 此时标识压缩后文件的len
	fclose(fpolddel);
	//free(fpolddel);
	fclose(fpnewout);
	//free(fpnewout);
	//remove("zip.bin");//del 临时文件




}
 /*g++ .c 压缩文件*/
 
 
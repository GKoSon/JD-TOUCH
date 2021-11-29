#include <stdio.h>
#include <string.h>
#include <stdlib.h>//free函数

/*开源*/ 
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
/*开源*/
	




#define DEBUG_LOG(type, message)                                           \
do                                                                            \
{                                                                             \
    if (type)                                                                 \
        printf message;                                                   \
}                                                                             \
while (0)
#define ERR_DEBUG                                                      1
#define INFO_DEBUG                                                     1
#define LOG_DEBUG                                                      0

//完成'4'-->4  ‘A’-->10
unsigned char str2byte(unsigned char dData)
{
	unsigned char rst=0;
	if((dData >= '0')&& (dData <= '9'))
				rst = dData-'0';
	else if((dData >= 'A')&&(dData <= 'F'))
				rst =  dData+10-'A';
	else if((dData >= 'a')&&(dData <= 'f'))
				rst =  dData+10-'a';
	else {printf("ERR str2byte\r\n");rst = 0;}//不是ASCII的char就返回0 有点儿欠妥！
			
	return rst;
}
//压缩一半
//完成"10"--->0x10  "1234"--->0X12,0X34 
//len标识传入的strlen(strings)
//返回值是len的一半
char memcpydown(void* Bytes,void* Strings,char len)
{
	unsigned char* strings=(unsigned char *)Strings;
	unsigned char* bytes  =(unsigned char *)Bytes;
	unsigned char i = 0,j=0,lowbits=0,highbits=0;
	if(len%2){printf("ERR memcpydown\r\n");return 0;}//禁止奇数
	for (i = 0; i < len; i+=2)
	{           
		highbits = str2byte(strings[i]  );
		lowbits  = str2byte(strings[i+1]);
		bytes[j++] =( highbits << 4)|lowbits;
	}
	return j;
}

//#define INPUT_HEX_NAME "1.hex"
//#define OUTPUT_BIN_NAME "1.bin"
const char * INPUT_HEX_NAME = "1.hex";
const char * OUTPUT_BIN_NAME ="1.bin";
const char * OUTPUT_ZIP_NAME ="zip.bin";
const char * OUTPUT_HEADZIP_NAME ="headzip.bin";
void hex2bin(void)
{
	FILE *fp1,*fp2; 
	unsigned int lSize=0;
	//char *name = "1.bin";
	const char *name = OUTPUT_BIN_NAME;
	unsigned char ch,start=0;
	unsigned char buf[44]={0},i=0,len=0,addr,type;
	unsigned char bin[16]={0},j=0;
	

	//fp1 = fopen("1.hex", "r");
	fp1 = fopen(INPUT_HEX_NAME, "r");
	
	if (NULL == fp1){ printf("NULL == fp1 no find 1.hex"); return ;}

	fp2 = fopen(name, "wb");
	if (NULL == fp2){ printf("NULL == fp2 no find 1.bin please root sudo");return ;}
	

	while(1)
	{
		ch = fgetc(fp1);
		if(feof(fp1))//如果读到了文件结尾，就退出 while循环
			break;
     
	    if(start == 0)
	    {
	    	if(ch==':'){start = 1;i=0;len=0;memset(buf,0,44);memset(bin,0,16);}
	    }	
		else 
		{
			buf[i++]=ch;
			if(i==2)//10
			{
			    len  = (str2byte(buf[0]))*16 + (str2byte(buf[1]));
				//源码这有问题比如上面0E就处理成21了 需要上面的函数 len  = (buf[0]-'0')*16 + (buf[1]-'0');
				//if(len!=16 && len!=8 && len!=0){start = 0;continue;}//要求长度是16 最后有个是8 其他的赶紧走
				//if(len ==2 || len==4 ){start = 0;continue;}//要求长度是16 最后有个是8 其他的赶紧走
				if(buf[0]=='1'&&buf[1]=='0')
				{}
				else 
				{
					//printf("[%c-%c]\r\n",buf[0],buf[1]);
				}
			}
			if(i==2+4)//A640
			    addr = buf[2]+buf[3]+buf[4]+buf[5];
			if(i==2+4+2)//00
			{
				type = (buf[6]-'0')*16 + (buf[7]-'0');
 
				if(type!=0)//要求类别是00的才可以放入bin文件 别的赶紧走
				{ 
					//if(type==4)printf("不支持线下地址\r\n");
					//else if(type==2)printf("不支持扩展段\r\n");	
					//else if(type==1)printf("文件即将结束\r\n");	
					//else printf("type=%d\r\n",type);	
					start = 0;
				}
			}
	        if(i == 2+4+2+len*2)
	        {//0208116A0208B9690208E96B84190C1E
 	        	memcpydown(bin,&buf[8],len*2);
	        	for(j=0;j<len;j++)
					fputc(bin[j] , fp2);
				start = 0;
				i=0;
	        }

		}
	}
	
	fclose(fp1);
	fclose(fp2);
    //free(fp1);
	//free(fp2)
	DEBUG_LOG(INFO_DEBUG, ("******1.hex->1.bin*******\r\n"));
}

unsigned short CRC16_CCITT(unsigned char *puchMsg, unsigned int usDataLen)  
{  
  unsigned short wCRCin = 0x0000;  
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


uint16_t all_bin_num = 0;
uint16_t every_bin_len[50]={0};
uint16_t every_zip_len[50]={0};
char every_bin_name[50][20]={0};
char every_zip_name[50][20]={0};
uint32_t allbinlen = 0;
int get_filelen(char *filename)
{
	FILE *fptarget; 
	int  lSize=0;
	fptarget = fopen(filename, "rb");
	fseek(fptarget,0,SEEK_END);//光标到尾巴
    lSize = ftell(fptarget);//光标的位置就是长度了
	fclose(fptarget);

	return lSize;
}
void bin2Nbin(void)
{
    #define SIZEONE (4096)
	FILE *fptarget; 
	unsigned char ch,cnt,i;
	int  lSize=0,lastlen=0;
	char name[20]={0};
	char buffer[SIZEONE]={0};
	fptarget = fopen(OUTPUT_BIN_NAME, "rb");
	//https://blog.csdn.net/a6472953/article/details/7190112
	fseek(fptarget,0,SEEK_END);//光标到尾巴
    lSize = ftell(fptarget);//光标的位置就是长度了
	fclose(fptarget);
	cnt =  lSize%SIZEONE ? (lSize/SIZEONE +1) :lSize/SIZEONE ;
allbinlen = lSize;
	lastlen = lSize - (SIZEONE*(cnt-1));
  DEBUG_LOG(LOG_DEBUG, ("target %s len =%d to be %d is numbered %d[last one is not %d is %d]\n",OUTPUT_BIN_NAME,lSize,SIZEONE,cnt,SIZEONE,lastlen));
	fptarget = fopen(OUTPUT_BIN_NAME, "rb");
	FILE *fp; 
	for( i=0;i<cnt-1;i++)
	{	
		sprintf(name,"NO-%02d.bin",i);
		fp = fopen(name, "wb");
		memset(buffer,0,SIZEONE);
	    fread (buffer, sizeof(char), SIZEONE, fptarget);
		fwrite(buffer, sizeof(char), SIZEONE, fp);
		fclose(fp);
CRC16_CCITT_ONE((uint8_t*)&buffer,SIZEONE);
		//free(fp);//这句话非常重要 否则就第一个bin文件是好的
	}

	sprintf(name,"NO-%02d.bin",cnt-1);
	fp = fopen(name, "wb");
	memset(buffer,0,SIZEONE);
	fread (buffer, sizeof(char), lastlen, fptarget);
	fwrite(buffer, sizeof(char), lastlen, fp);
	fclose(fp);
CRC16_CCITT_ONE((uint8_t*)&buffer,lastlen);
	//free(fp);//这句话非常重要 否则就第一个bin文件是好的
	

    DEBUG_LOG(INFO_DEBUG, ("******1.bin->N.bin*******\r\n"));
	for( i=0;i<cnt;i++)
	{	
        memset(name,0,sizeof(name));
		sprintf(name,"NO-%02d.bin",i);
    	lSize = get_filelen(name);
        DEBUG_LOG(LOG_DEBUG, ("file %s len =%d ",name,lSize));
        every_bin_len[i]=lSize;//////// 全局
		memcpy (every_bin_name[i],name,strlen(name));//////// 全局
		sprintf(every_zip_name[i],"%s%s",name,".zip");
		DEBUG_LOG(LOG_DEBUG, ("[BINfile %s ZIPfile =%s]\n",every_bin_name[i],every_zip_name[i]));

	}
	all_bin_num=cnt;//////// 全局

    DEBUG_LOG(INFO_DEBUG, ("******bin2Nbin*******\r\n"));
	return;
}
/*给出一个bin文件名字 + 该文件的长度我制作一个zip 返回该ZIP的长度*/
/*前面已经保证了bin文件长度是4096 除了最后一个*/
int bin2zip(char *binname,int inlen,char *zipname)
{
	#define INLEN  4096

	uint8_t indata[INLEN]={0};
	uint8_t outdata[INLEN+512]={0};

	FILE *fpBIN  = fopen(binname, "rb");
	FILE *fpZIP  = fopen(zipname, "wb");
 
	fread (indata, sizeof(char), inlen, fpBIN);
	int outlen = fastlz_compress(indata,inlen,outdata);
	DEBUG_LOG(LOG_DEBUG, ("inlen=%d -- outlen=%d [%d]\r\n",inlen,outlen,outlen*100/inlen));
    fwrite(outdata, sizeof(char), outlen, fpZIP);

    fclose(fpZIP);
	fclose(fpBIN);
    DEBUG_LOG(INFO_DEBUG, ("******bin2zip*******\r\n"));
	return outlen;
}

void Nbin2Nzip(void)
{
	int i = 0;
	for(i = 0 ; i < all_bin_num;i++)
       every_zip_len[i] = bin2zip(every_bin_name[i],every_bin_len[i],every_zip_name[i]);
    DEBUG_LOG(INFO_DEBUG, ("******Nbin2Nzip*******\r\n"));
}

void Nzip2zip(void)
{
	uint8_t	data2buf[50*4096];//因为最大是50个4K的文件 
	FILE *fpin   =  NULL;
	FILE *fpout  =  fopen(OUTPUT_ZIP_NAME, "wb");
	int i =0;
	for(i=0;i<all_bin_num;i++)
	{
		memset(data2buf,0,sizeof(data2buf));
		fpin = fopen(every_zip_name[i],  "rb");
		fread (data2buf, sizeof(char), every_zip_len[i], fpin);
		fwrite(data2buf, sizeof(char), every_zip_len[i], fpout);	
		fclose(fpin);
	}
    fclose(fpout);
}


void Zip_Head_Handle(void)
{
	uint8_t	i,data2buf[200*1024];
	FILE *fpolddel  =  fopen(OUTPUT_ZIP_NAME,        "rb");
	FILE *fpnewout  =  fopen(OUTPUT_HEADZIP_NAME,    "wb");
	//fwrite(every_zip_len, sizeof(char), 50*2, fpnewout);//此时把压缩后的bin文件在头部写入这个数组！
	fwrite(&wCRCin,       sizeof(uint16_t), 1,           fpnewout);
	fwrite(&all_bin_num,  sizeof(uint16_t), 1,           fpnewout);

	fwrite(every_zip_len, sizeof(uint16_t), all_bin_num, fpnewout);//此时把压缩后的bin文件在头部写入这个数组！

	memset(data2buf,0,200*1024);
	int	allziplen=0;
    for(i=0; i<all_bin_num; i++) allziplen+=every_zip_len[i];
  
  DEBUG_LOG(INFO_DEBUG, ("allziplen=%d allbinlen=%d [%d]\r\n",allziplen,allbinlen,(allziplen*100)/allbinlen));
	fread (data2buf, sizeof(char), allziplen, fpolddel);
	fwrite(data2buf, sizeof(char), allziplen, fpnewout);	
	fclose(fpolddel);
    fclose(fpnewout);
}

void Show_Head(void)
{
    uint8_t	i;
    printf("Show_Head\r\n");
    DEBUG_LOG(INFO_DEBUG, ("******Show_Head*******\r\n"));
    FILE *fpnewout  =  fopen(OUTPUT_HEADZIP_NAME,    "rb");
	uint16_t	buf[50];
	fread (buf, sizeof(uint16_t), 50, fpnewout);

    DEBUG_LOG(INFO_DEBUG, ("1--CRC16    =0X%04X\r\n",buf[0]));
    DEBUG_LOG(INFO_DEBUG, ("2--numofzips=%d\r\n",buf[1]));
    for(i=0;i<buf[1];i++)
      DEBUG_LOG(INFO_DEBUG, ("【%d】lenofzip=0X%04X \r\n",i,buf[i]));

	fclose(fpnewout);
}
/*验证全局CRC16 和分布CRC16 是否相等 JAMES*/
void Test_crc16(void)
{
	int  lSize=0;
	FILE *fp = fopen(OUTPUT_BIN_NAME, "rb");
	fseek(fp,0,SEEK_END);//光标到尾巴
    lSize = ftell(fp);//光标的位置就是长度了
	fclose(fp);

	uint8_t	i,data2buf[50*4096];
	fp  =  fopen(OUTPUT_BIN_NAME,     "rb");
	memset(data2buf,0,200*1024);
	fread (data2buf, sizeof(char), lSize, fp);
    printf("JAMES ALL FILE[%d]crc16=%04X\r\n",	lSize,CRC16_CCITT(data2buf,lSize));	
	fclose(fp);

	printf("JAMES ALL FILE crc16=%04X\r\n",	wCRCin);
}
int main(int argc,char **argv)
{
	/*完成1.hex-->1.bin*/
    hex2bin();
    /*完成1.bin-->N个bin 前面都是4K最后一个不确定*/
    bin2Nbin();
	/*完成N个bin 每个都压缩一次【一个多少个bin？每个bin多大？前面函数做的全局变量】*/
	Nbin2Nzip();
	/*完成N个ZIP 合并为1个zip【多少个zip？前面bin数目一样的 全局变量】*/
	Nzip2zip();
	/*在ZIP头部写入特征表*/
	Zip_Head_Handle();
	Test_crc16();
	Show_Head();
	printf("******GKOSON FINISHED*******\r\n");
	//getchar();
	char i;
	for(i=0;i<all_bin_num;i++)
    {
		remove(every_zip_name[i]);
		remove(every_bin_name[i]);
	}
	return 1;
}

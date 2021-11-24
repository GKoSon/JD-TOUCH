#include <stdio.h>
#include <string.h>
#include <stdlib.h>//free函数
 
 
/*所谓MD5就是给进去文件字符串啥的 给你一个摘要 结果是16个HEX或者32个char*/
 
/*MD5部分--------------------H*/
typedef struct
{
    unsigned int count[2];
    unsigned int state[4];
    unsigned char buffer[64];   
}MD5_CTX;
 
                         
#define F(x,y,z) ((x & y) | (~x & z))
#define G(x,y,z) ((x & z) | (y & ~z))
#define H(x,y,z) (x^y^z)
#define I(x,y,z) (y ^ (x | ~z))
#define ROTATE_LEFT(x,n) ((x << n) | (x >> (32-n)))
#define FF(a,b,c,d,x,s,ac) \
          { \
          a += F(b,c,d) + x + ac; \
          a = ROTATE_LEFT(a,s); \
          a += b; \
          }
#define GG(a,b,c,d,x,s,ac) \
          { \
          a += G(b,c,d) + x + ac; \
          a = ROTATE_LEFT(a,s); \
          a += b; \
          }
#define HH(a,b,c,d,x,s,ac) \
          { \
          a += H(b,c,d) + x + ac; \
          a = ROTATE_LEFT(a,s); \
          a += b; \
          }
#define II(a,b,c,d,x,s,ac) \
          { \
          a += I(b,c,d) + x + ac; \
          a = ROTATE_LEFT(a,s); \
          a += b; \
          }                                            
void MD5Init(MD5_CTX *context);
void MD5Update(MD5_CTX *context,unsigned char *input,unsigned int inputlen);
void MD5Final(MD5_CTX *context,unsigned char digest[16]);
void MD5Transform(unsigned int state[4],unsigned char block[64]);
void MD5Encode(unsigned char *output,unsigned int *input,unsigned int len);
void MD5Decode(unsigned int *output,unsigned char *input,unsigned int len);
 
 
void test_MD51(void);
void test_MD52(void);			
void GMD5_str_IIO( unsigned char *input, unsigned char inputlen, unsigned char output[33] );
 
 
/*MD5部分--------------------C*/
unsigned char PADDING[]={0x80,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                         0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                         0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                         0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
                         
void MD5Init(MD5_CTX *context)
{
     context->count[0] = 0;
     context->count[1] = 0;
     context->state[0] = 0x67452301;
     context->state[1] = 0xEFCDAB89;
     context->state[2] = 0x98BADCFE;
     context->state[3] = 0x10325476;
}
void MD5Update(MD5_CTX *context,unsigned char *input,unsigned int inputlen)
{
    unsigned int i = 0,index = 0,partlen = 0;
    index = (context->count[0] >> 3) & 0x3F;
    partlen = 64 - index;
    context->count[0] += inputlen << 3;
    if(context->count[0] < (inputlen << 3))
       context->count[1]++;
    context->count[1] += inputlen >> 29;
    
    if(inputlen >= partlen)
    {
       memcpy(&context->buffer[index],input,partlen);
       MD5Transform(context->state,context->buffer);
       for(i = partlen;i+64 <= inputlen;i+=64)
           MD5Transform(context->state,&input[i]);
       index = 0;        
    }  
    else
    {
        i = 0;
    }
    memcpy(&context->buffer[index],&input[i],inputlen-i);
}
void MD5Final(MD5_CTX *context,unsigned char digest[16])
{
    unsigned int index = 0,padlen = 0;
    unsigned char bits[8];
    index = (context->count[0] >> 3) & 0x3F;
    padlen = (index < 56)?(56-index):(120-index);
    MD5Encode(bits,context->count,8);
    MD5Update(context,PADDING,padlen);
    MD5Update(context,bits,8);
    MD5Encode(digest,context->state,16);
}
void MD5Encode(unsigned char *output,unsigned int *input,unsigned int len)
{
    unsigned int i = 0,j = 0;
    while(j < len)
    {
         output[j] = input[i] & 0xFF;  
         output[j+1] = (input[i] >> 8) & 0xFF;
         output[j+2] = (input[i] >> 16) & 0xFF;
         output[j+3] = (input[i] >> 24) & 0xFF;
         i++;
         j+=4;
    }
}
void MD5Decode(unsigned int *output,unsigned char *input,unsigned int len)
{
     unsigned int i = 0,j = 0;
     while(j < len)
     {
           output[i] = (input[j]) |
                       (input[j+1] << 8) |
                       (input[j+2] << 16) |
                       (input[j+3] << 24);
           i++;
           j+=4; 
     }
}
void MD5Transform(unsigned int state[4],unsigned char block[64])
{
	unsigned int a = state[0];
	unsigned int b = state[1];
	unsigned int c = state[2];
	unsigned int d = state[3];
	unsigned int x[64];
	MD5Decode(x,block,64);
	FF(a, b, c, d, x[ 0], 7, 0xd76aa478); /* 1 */
	FF(d, a, b, c, x[ 1], 12, 0xe8c7b756); /* 2 */
	FF(c, d, a, b, x[ 2], 17, 0x242070db); /* 3 */
	FF(b, c, d, a, x[ 3], 22, 0xc1bdceee); /* 4 */
	FF(a, b, c, d, x[ 4], 7, 0xf57c0faf); /* 5 */
	FF(d, a, b, c, x[ 5], 12, 0x4787c62a); /* 6 */
	FF(c, d, a, b, x[ 6], 17, 0xa8304613); /* 7 */
	FF(b, c, d, a, x[ 7], 22, 0xfd469501); /* 8 */
	FF(a, b, c, d, x[ 8], 7, 0x698098d8); /* 9 */
	FF(d, a, b, c, x[ 9], 12, 0x8b44f7af); /* 10 */
	FF(c, d, a, b, x[10], 17, 0xffff5bb1); /* 11 */
	FF(b, c, d, a, x[11], 22, 0x895cd7be); /* 12 */
	FF(a, b, c, d, x[12], 7, 0x6b901122); /* 13 */
	FF(d, a, b, c, x[13], 12, 0xfd987193); /* 14 */
	FF(c, d, a, b, x[14], 17, 0xa679438e); /* 15 */
	FF(b, c, d, a, x[15], 22, 0x49b40821); /* 16 */
 
	/* Round 2 */
	GG(a, b, c, d, x[ 1], 5, 0xf61e2562); /* 17 */
	GG(d, a, b, c, x[ 6], 9, 0xc040b340); /* 18 */
	GG(c, d, a, b, x[11], 14, 0x265e5a51); /* 19 */
	GG(b, c, d, a, x[ 0], 20, 0xe9b6c7aa); /* 20 */
	GG(a, b, c, d, x[ 5], 5, 0xd62f105d); /* 21 */
	GG(d, a, b, c, x[10], 9,  0x2441453); /* 22 */
	GG(c, d, a, b, x[15], 14, 0xd8a1e681); /* 23 */
	GG(b, c, d, a, x[ 4], 20, 0xe7d3fbc8); /* 24 */
	GG(a, b, c, d, x[ 9], 5, 0x21e1cde6); /* 25 */
	GG(d, a, b, c, x[14], 9, 0xc33707d6); /* 26 */
	GG(c, d, a, b, x[ 3], 14, 0xf4d50d87); /* 27 */
	GG(b, c, d, a, x[ 8], 20, 0x455a14ed); /* 28 */
	GG(a, b, c, d, x[13], 5, 0xa9e3e905); /* 29 */
	GG(d, a, b, c, x[ 2], 9, 0xfcefa3f8); /* 30 */
	GG(c, d, a, b, x[ 7], 14, 0x676f02d9); /* 31 */
	GG(b, c, d, a, x[12], 20, 0x8d2a4c8a); /* 32 */
 
	/* Round 3 */
	HH(a, b, c, d, x[ 5], 4, 0xfffa3942); /* 33 */
	HH(d, a, b, c, x[ 8], 11, 0x8771f681); /* 34 */
	HH(c, d, a, b, x[11], 16, 0x6d9d6122); /* 35 */
	HH(b, c, d, a, x[14], 23, 0xfde5380c); /* 36 */
	HH(a, b, c, d, x[ 1], 4, 0xa4beea44); /* 37 */
	HH(d, a, b, c, x[ 4], 11, 0x4bdecfa9); /* 38 */
	HH(c, d, a, b, x[ 7], 16, 0xf6bb4b60); /* 39 */
	HH(b, c, d, a, x[10], 23, 0xbebfbc70); /* 40 */
	HH(a, b, c, d, x[13], 4, 0x289b7ec6); /* 41 */
	HH(d, a, b, c, x[ 0], 11, 0xeaa127fa); /* 42 */
	HH(c, d, a, b, x[ 3], 16, 0xd4ef3085); /* 43 */
	HH(b, c, d, a, x[ 6], 23,  0x4881d05); /* 44 */
	HH(a, b, c, d, x[ 9], 4, 0xd9d4d039); /* 45 */
	HH(d, a, b, c, x[12], 11, 0xe6db99e5); /* 46 */
	HH(c, d, a, b, x[15], 16, 0x1fa27cf8); /* 47 */
	HH(b, c, d, a, x[ 2], 23, 0xc4ac5665); /* 48 */
 
	/* Round 4 */
	II(a, b, c, d, x[ 0], 6, 0xf4292244); /* 49 */
	II(d, a, b, c, x[ 7], 10, 0x432aff97); /* 50 */
	II(c, d, a, b, x[14], 15, 0xab9423a7); /* 51 */
	II(b, c, d, a, x[ 5], 21, 0xfc93a039); /* 52 */
	II(a, b, c, d, x[12], 6, 0x655b59c3); /* 53 */
	II(d, a, b, c, x[ 3], 10, 0x8f0ccc92); /* 54 */
	II(c, d, a, b, x[10], 15, 0xffeff47d); /* 55 */
	II(b, c, d, a, x[ 1], 21, 0x85845dd1); /* 56 */
	II(a, b, c, d, x[ 8], 6, 0x6fa87e4f); /* 57 */
	II(d, a, b, c, x[15], 10, 0xfe2ce6e0); /* 58 */
	II(c, d, a, b, x[ 6], 15, 0xa3014314); /* 59 */
	II(b, c, d, a, x[13], 21, 0x4e0811a1); /* 60 */
	II(a, b, c, d, x[ 4], 6, 0xf7537e82); /* 61 */
	II(d, a, b, c, x[11], 10, 0xbd3af235); /* 62 */
	II(c, d, a, b, x[ 2], 15, 0x2ad7d2bb); /* 63 */
	II(b, c, d, a, x[ 9], 21, 0xeb86d391); /* 64 */
	state[0] += a;
	state[1] += b;
	state[2] += c;
	state[3] += d;
}
 
 
 
 
//0XAB-->"AB" 长度会扩大一倍！注意：0没有结束符
static void G_1byteTo2str(unsigned char* strings,unsigned char* bytes,unsigned char len)
{
    unsigned char const StrRefer[]="0123456789abcdef";//"0123456789ABCDEF";
    #define GET_MSB_STR(x) (StrRefer[((x>>4)&0x0f)])
    #define GET_LSB_STR(x) (StrRefer[(x&0x0f)])
    for(char i=0,j=0;i<len;i++,j+=2)
    {
        strings[j]  =GET_MSB_STR(bytes[i]);
        strings[j+1]=GET_LSB_STR(bytes[i]);
    }
 
}
 
 
 
 
void test_MD51(void)
{
	int i;
	unsigned char encrypt[] ="admin";//21232f297a57a5a743894a0e4a801fc3
	unsigned char decrypt[16];
    unsigned char decrypt2[32];	
	MD5_CTX md5;
	MD5Init(&md5);         		
	MD5Update(&md5,encrypt,strlen((char *)encrypt));
	MD5Final(&md5,decrypt);        
	printf("加密前:%s\r\n加密后:",encrypt);
	for(i=0;i<16;i++)
	{
		printf("%02X",decrypt[i]);
	}
	printf("\r\n");
	
	G_1byteTo2str(decrypt2,decrypt,16);
	for(i=0;i<32;i++)
	{
		printf("%02X-",decrypt2[i]);
	}
		printf("\r\n");
}
//MD5出来都是16个0XFF 现在这个是基础可以连续 就是最后变一下 就是MD5FILE
void test_MD52(void)
{
		int i;
		unsigned char encrypt[] ="adminadminadmin123";//18个
		unsigned char decrypt[16];
		MD5_CTX md5;
		MD5Init(&md5);         		
		MD5Update(&md5,encrypt,strlen((char *)encrypt));
		MD5Final(&md5,decrypt);        
		printf("方法1加密前:%s\r\n加密后:",encrypt);//86A74E7F4570EC093FC62E3D99B233BB
		for(i=0;i<16;i++)
		{
		printf("%02X",decrypt[i]);
		}
		printf("\r\n+++++++++++++++++\r\n");
		memset(decrypt,0,16); 
		MD5Init(&md5);    
		for(char i=0;i<3;i++)
		MD5Update(&md5,&encrypt[6*i],6);
		MD5Final(&md5,decrypt);  
		printf("方法2加密前:%s\r\n加密后:",encrypt);
		for(i=0;i<16;i++)
		{
		printf("%02X",decrypt[i]);
		}
		printf("\r\n##################\r\n");
}
 
void GMD5_str_IIO( unsigned char *input, unsigned char inputlen, unsigned char output[33] )
{
	unsigned char decrypt[16];
	MD5_CTX md5;
	MD5Init(&md5);         		
	MD5Update(&md5,input,inputlen);
	MD5Final(&md5,decrypt);        
	G_1byteTo2str(output,decrypt,16);
//		for(int i=0;i<32;i++)
//	{
//		printf("%02X-",output[i]);
//	}
//		printf("\r\n");
}
 
 
 
//完成'4'-->4  ‘A’-->10
unsigned char G_strTobyte(unsigned char dData)
{
	unsigned char rst=0;
	if((dData >= '0')&& (dData <= '9'))
				rst = dData-'0';
	else if((dData >= 'A')&&(dData <= 'F'))
				rst =  dData+10-'A';
	else if((dData >= 'a')&&(dData <= 'f'))
				rst =  dData+10-'a';
	else {printf("ERR G_strTobyte\r\n");rst = 0;}//不是ASCII的char就返回0 有点儿欠妥！
			
	return rst;
}
//压缩一半
//完成"10"--->0x10  "1234"--->0X12,0X34 
//len标识传入的strlen(strings)
//返回值是len的一半
char G_strsTobytes(void* Strings,void* Bytes,char len)
{
	unsigned char* strings=(unsigned char *)Strings;
	unsigned char* bytes  =(unsigned char *)Bytes;
	unsigned char i = 0,j=0,lowbits=0,highbits=0;
	if(len%2){printf("ERR G_strsTobytes\r\n");return 0;}//禁止奇数
	for (i = 0; i < len; i+=2)
	{           
		highbits = G_strTobyte(strings[i]  );
		lowbits  = G_strTobyte(strings[i+1]);
		bytes[j++] =( highbits << 4)|lowbits;
	}
	return j;
}

int main(int argc,char **argv)
{
	FILE *fp1,*fp2; 
	unsigned int lSize=0;
	char *name = NULL;
	unsigned char ch,start=0;
	unsigned char buf[44]={0},i=0,len=0,addr,type;
	unsigned char bin[16]={0},j=0;
	if(argc==1)
	{
		name = (char *)"1.bin";
		printf("NEWNAME %s\r\n",name);
	}
	else if(argc==2)
	{
		name = argv[1];
		printf("NEWNAME %s\r\n",name);
	}

	fp1 = fopen("1.hex", "r");
	if (NULL == fp1){ printf("NULL == fp1"); return 1;}

	fp2 = fopen(name, "wb");
	if (NULL == fp2){ printf("NULL == fp2");return 1;}
	

	while(1)
	{
		ch = fgetc(fp1);
		if(feof(fp1))		//如果读到了文件结尾，就退出 while循环
			break;
     
	    if(start == 0)
	    {
	    	if(ch==':'){start = 1;i=0;len=0;memset(buf,0,44);memset(bin,0,16);}
	    }	
		else 
		{
//  :1016500003F92802083010120878124408520418BE
//  :0E1660001003E529020848100002F8F80208FD
			buf[i++]=ch;
			if(i==2)//10
			{
			    len  = (G_strTobyte(buf[0]))*16 + (G_strTobyte(buf[1]));
				//源码这有问题比如上面0E就处理成21了 需要上面的函数 len  = (buf[0]-'0')*16 + (buf[1]-'0');
				//if(len!=16 && len!=8 && len!=0){start = 0;continue;}//要求长度是16 最后有个是8 其他的赶紧走
				//if(len ==2 || len==4 ){start = 0;continue;}//要求长度是16 最后有个是8 其他的赶紧走
				if(buf[0]=='1'&&buf[1]=='0')
				{}
				else 
				{printf("[%c-%c]\r\n",buf[0],buf[1]);}
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
 	        	G_strsTobytes(&buf[8],bin,len*2);
	        	for(j=0;j<len;j++)
					fputc(bin[j] , fp2);
				start = 0;
				i=0;
	        }

		}
	}
	
	fclose(fp1);
	fclose(fp2);
//	free(fp1);
//	free(fp2);
	printf("******GKOSON FINISHED*******\r\n");
 
/*
自己做 追加BIN
1--加入长度
2--加入MD5
3--加入CRC【放弃】
4--加密【放弃】
一个脚本完成！放在最后！
*/

 	//1----完成文件长度的GET
	FILE *fp = fopen(name, "rb");
    if (NULL == fp){ printf("NULL == fp");return 1;}
	fseek(fp,0,SEEK_END);//光标到尾巴
	lSize = ftell(fp);//光标的位置就是长度了
	fclose(fp);
//	free(fp);
    printf("bin len =%d = %dK\n",lSize,lSize/1024);

 	//2----完成文件MD5的GET
    FILE *fpMD5 = fopen(name, "rb");
    int filecnt,infilelen,addrpage;
    unsigned char infile[4096];//每次读到文件的buf//前面都是4096最后一页不是哦
    unsigned char  MD5rst[16];
    MD5_CTX md5;
    MD5Init(&md5);     
 
    filecnt = (lSize/4096)?(lSize/4096+1):(lSize/4096);
 
    for(i=0;i<filecnt-1;i++)
    {
        addrpage=0 + (i*4096);
		fseek(fpMD5,addrpage,SEEK_SET);//光标到开头
		
        infilelen=4096;
		
		fread (infile, sizeof(char), infilelen, fpMD5);
        MD5Update(&md5,infile,infilelen);
 
    }
    i=filecnt-1;
    addrpage=0 + (i*4096);
	fseek(fpMD5,addrpage,SEEK_SET);//光标到开头
	
    infilelen=(lSize)-((i)*4096);    printf("infilelen=%d\r\n",infilelen);
	
    fread (infile, sizeof(char), infilelen, fpMD5);
    MD5Update(&md5,infile,infilelen);
    MD5Final(&md5,MD5rst);        
    printf("MD5rst:\r\n");    for(i=0;i<16;i++)    printf("%02X",MD5rst[i]);    printf("\r\n");
   
   
   
   
    fclose(fpMD5);
//	free(fpMD5);
 
 
 	//3----执行追加：	
	FILE *fpbin = fopen(name,  "ab+");//写
	//打开 默认在文件头 需要到尾巴wb--》wb+
	//fseek(fpbin,0,SEEK_END);//光标到尾巴
	//fseek(fpbin,lSize,SEEK_SET);//光标到尾巴
	unsigned char uintbuf[4];
	memcpy(uintbuf,&lSize,4);		
	//fwrite(uintbuf, sizeof(char), 4, fpbin);
	//fwrite(MD5rst, sizeof(char), 16, fpbin);
    for(j=0;j<4;j++)	fputc(uintbuf[j] , fpbin);
    for(j=0;j<16;j++)	fputc(MD5rst[j] ,  fpbin);
	fclose(fpbin);
//	free(fpbin);
 
    printf("追加完毕\r\n");
	//scanf("%d\n",&ch);//人为堵塞一下
	getchar();//人为堵塞一下
	return 1;
}

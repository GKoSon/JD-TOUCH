#include <stdio.h>
#include <string.h>
#include <stdlib.h>//free函数


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

void hex2bin(void)
{
	FILE *fp1,*fp2; 
	unsigned int lSize=0;
	char *name = "1.bin";
	unsigned char ch,start=0;
	unsigned char buf[44]={0},i=0,len=0,addr,type;
	unsigned char bin[16]={0},j=0;
	

	fp1 = fopen("1.hex", "r");
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
	printf("******1.hex->1.bin*******\r\n");
}



void bin2Nbin(void)
{
    #define SIZEONE (1024*4)
	FILE *fptarget; 
	unsigned char ch,cnt,i;
	int  lSize=0,lastlen=0;
	char name[40]={0};
	char buffer[SIZEONE]={0};
	fptarget = fopen("1.bin", "rb");
	//https://blog.csdn.net/a6472953/article/details/7190112
	fseek(fptarget,0,SEEK_END);//光标到尾巴
    lSize = ftell(fptarget);//光标的位置就是长度了
	fclose(fptarget);
	cnt =  lSize%SIZEONE ? (lSize/SIZEONE +1) :lSize/SIZEONE ;
	lastlen = lSize - (SIZEONE*(cnt-1));
    printf("target 1.bin len =%d to be %d is numbered %d[last one is not %d is %d]\n",lSize,SIZEONE,cnt,SIZEONE,lastlen);
	
	fptarget = fopen("1.bin", "rb");
	FILE *fp; 
	for( i=0;i<cnt-1;i++)
	{	
		sprintf(name,"NO-%02d.bin",i);
		fp = fopen(name, "wb");
		memset(buffer,0,SIZEONE);
	    fread (buffer, sizeof(char), SIZEONE, fptarget);
		fwrite(buffer, sizeof(char), SIZEONE, fp);
		fclose(fp);
		//free(fp);//这句话非常重要 否则就第一个bin文件是好的
	}

	sprintf(name,"NO-%02d.bin",cnt-1);
	fp = fopen(name, "wb");
	memset(buffer,0,SIZEONE);
	fread (buffer, sizeof(char), lastlen, fptarget);
	fwrite(buffer, sizeof(char), lastlen, fp);
	fclose(fp);
	//free(fp);//这句话非常重要 否则就第一个bin文件是好的
	
	printf("******1.bin->N.bin*******\n");
	for( i=0;i<cnt;i++)
	{	

		sprintf(name,"NO-%02d.bin",i);
		fp = fopen(name, "rb");
		fseek(fp,0,SEEK_END);//光标到尾巴
    	lSize = ftell(fp);//光标的位置就是长度了
		fclose(fp);
		//free(fp);
        printf("NO-%02d.bin len =%d  is  %d\n",i,lSize);
	}
	return;
}
int main(int argc,char **argv)
{
	/*完成1.hex-->1.bin*/
    hex2bin();
    /*完成1.bin-->N个bin 前面都是4K最后一个不确定*/
    bin2Nbin();
	/*完成N个bin 每个都压缩一次*/
	
	printf("******GKOSON FINISHED*******\r\n");
	getchar();

	return 1;
}

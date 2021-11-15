#include <stdio.h>
#include <string.h>
#include <stdlib.h>//free


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
					//if(type==2)printf("不支持扩展段\r\n");	
					//if(type==1)printf("文件即将结束\r\n");	
					printf("type=%d\r\n",type);	
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
	free(fp1);
	free(fp2);
	printf("******GKOSON FINISHED*******");
	getchar();
	return 1;
}

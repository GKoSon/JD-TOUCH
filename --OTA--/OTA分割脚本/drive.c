#include <stdio.h>
#include <string.h>//memset
#include <stdlib.h>//free

#define SIZEONE 1024
int main(int argc,char **argv)
{
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
    printf("target len =%d to be %d is numbered %d\n",lSize,SIZEONE,cnt);
	
	fptarget = fopen("1.bin", "rb");
	for( i=0;i<cnt-1;i++)
	{	
		FILE *fp; 
		sprintf(name,"NO-%d.bin",i);
		fp = fopen(name, "wb");
		memset(buffer,0,SIZEONE);
	    fread (buffer, sizeof(char), SIZEONE, fptarget);
		fwrite(buffer, sizeof(char), SIZEONE, fp);
		fclose(fp);
		free(fp);//这句话非常重要 否则就第一个bin文件是好的
	}
	FILE *fp; 
	sprintf(name,"NO-%02d.bin",cnt-1);
	fp = fopen(name, "wb");
	memset(buffer,0,SIZEONE);
	fread (buffer, sizeof(char), lastlen, fptarget);
	fwrite(buffer, sizeof(char), lastlen, fp);
	fclose(fp);
	free(fp);//这句话非常重要 否则就第一个bin文件是好的
	
	printf("******GKOSON FINISHED*******");
	for( i=0;i<cnt;i++)
	{	
		FILE *fp; 
		sprintf(name,"NO-%d.bin",i);
		fp = fopen(name, "rb");
		fseek(fp,0,SEEK_END);//光标到尾巴
    	lSize = ftell(fp);//光标的位置就是长度了
		fclose(fp);
		free(fp);
        printf("NO-%02d.bin len =%d  is  %d\n",i,lSize);
	}
	scanf("%d\n",&ch);//人为堵塞一下
	return 1;
}
#include "unit.h"
#include "iwdg.h"
#include "config.h"
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

uint32_t    sysDebugFlag = 0xFE;


char diyota =0;
/*时间*/
/*控制*/
/*黑白名单*/
/*通讯组*/
/*升级*/ 
char topicPath0[]               = {"/star_line/server/timeCalibration"};
char topicPath1[GMAX_LEN_TOPIC] = {"/star_line/server/deviceControl/"};
char topicPath2[GMAX_LEN_TOPIC] = {"/star_line/server/syncFilterItem/"};
char topicPath3[GMAX_LEN_TOPIC] = {"/star_line/server/syncGroup/"};
char topicPath4[GMAX_LEN_TOPIC] = {"/star_line/server/otaDown/"};

/*一个十六进制数 比如0X11 那就是0001 0001 按照下面计算 返回是 11 也就是0X11变成11*/
uint8_t bcd_to_bin(uint8_t bcd)
{
    return (bcd>>4)*10 + (bcd&0x0F);
}


/*  tcp://139.9.66.72:18 */
char * ip_port_handle(char *  sor)
{
    char *p=0;
    typedef struct
    {
        uint8_t ip[20];
        uint16_t port;
    }serverAddrType;
    static serverAddrType ip_port;

    printf("ip_port_handle input[%s]\r\n",sor); 
    
    if(p = strstr ((const char*)sor,"//"))   p+=2;
    else  p = (char *)sor;

    
    sscanf((const char*)p,"%[^:]", ip_port.ip);
    
    p = strstr ((const char*)p,":");
    ++p;
    ip_port.port = atoi(p);

    
    return (char*)&ip_port;
}
         

//比较AB相等 1是的 0 不是 AB的差距是不是再rang内部 1是 0不是
uint8_t Gequal(uint32_t A,uint32_t B,uint8_t range)
{
    if(A>B)
    {
        if((A-B)>range)            return 0;
        else return 1;
    }
    else
    {
        if((B-A)>range)            return 0;
        else return 1;
    }

}

//0XAB-->"AB" 长度会扩大一倍！注意：0没有结束符
void memcpy_up(void* Strings,void* Bytes,unsigned char len)
{
   unsigned char* strings = (unsigned char*)Strings;
   unsigned char* bytes   = (unsigned char*)Bytes;
    unsigned char const StrRefer[]="0123456789abcdef";//"0123456789ABCDEF";
    #define GET_MSB_STR(x) (StrRefer[((x>>4)&0x0f)])
    #define GET_LSB_STR(x) (StrRefer[(x&0x0f)])
    for(char i=0,j=0;i<len;i++,j+=2)
    {
        strings[j]  =GET_MSB_STR(bytes[i]);
        strings[j+1]=GET_LSB_STR(bytes[i]);
    }

}


unsigned short CRC16_CCITT(unsigned char *puchMsg, unsigned int usDataLen)  
{  
  unsigned short wCRCin = 0x0000;  
  unsigned short wCPoly = 0x1021;  
  unsigned char wChar = 0;  
    
  while (usDataLen--)     
  {  
        wChar = *(puchMsg++);  
 
        wCRCin ^= (wChar << 8);  
        for(int i = 0;i < 8;i++)  
        {  
          if(wCRCin & 0x8000)  
            wCRCin = (wCRCin << 1) ^ wCPoly;  
          else  
            wCRCin = wCRCin << 1;  
        }  
  }  
  
  return (wCRCin) ;  
}

uint8_t mycrc8(uint8_t *ps1,uint8_t uLen)
{
  uint8_t i_crc=0,i;
  
  for(i=0;i<uLen;i++)
  {
        i_crc ^=*(ps1++);
  }
  printf("mycrc8=%02X\r\n",i_crc);
  return i_crc;
}

unsigned char aiot_strcmp( unsigned char *pst , unsigned char *str , unsigned char len)
{

     if((len == 0)||(pst == NULL)||(str == NULL))
    {
        return FALSE;
    }
        
    while( len-- )
    {
        if(*str++ != *pst++ )
            return FALSE;
    }
    return TRUE;
}


void printf_time( void )
{
    rtcTimeType tim;
    
    rtc.read_time(&tim);
    
    printf("[%02d-%02d-%02d %02d:%02d:%02d]" , tim.year,tim.mon,tim.day,tim.hour , tim.min , tim.sec);
      
}



void log_err( char* fmt, ...)
{
    va_list args;
    int fmt_result;
    char log_buf[512];

    memset(log_buf , 0x00 , 512);
    
    va_start(args, fmt);
    
    fmt_result = vsprintf(log_buf , fmt, args);
    if (( fmt_result > 0 ) && ( fmt_result < 512))
    {
#ifndef BOOTLOADER
        err_log(DEBUG_MESSAGE , (uint8_t *)log_buf , fmt_result);
#endif
        printf("\033[0;31m");
        printf("[#]");
        printf("[%d%]" , osGetCPUUsage());
        printf_time();\
        printf("%.*s" , fmt_result , log_buf);
        printf("\033[0;39m");
    }
    
    va_end(args);
         
}



void log_arry(uint32_t level ,unsigned char *pst , unsigned char *arry , unsigned int leng)
{
    if(LOG_BIT_ON(sysDebugFlag,level))
    {
        printf("%s :  [" , pst);
        for(unsigned int i = 0 ; i < leng ; i++)
        {
            printf("%02X " , arry[i]);
        }
        printf("]\r\n");
        printf("\033[0;39m");
    }
}

void log_arry10(uint32_t level ,unsigned char *pst , unsigned char *arry , unsigned int leng)
{
    if(LOG_BIT_ON(sysDebugFlag,level))
    {
        printf("%s :  [" , pst);
        for(unsigned int i = 0 ; i < leng ; i++)
        {
            printf("%d " , arry[i]);
        }
        printf("]\r\n");
        printf("\033[0;39m");
    }
}



uint8_t  hex_to_char(uint8_t ucData)
{
    if(ucData < 10){
        return ucData+'0';
    }
    else{
        return ucData-10+'A';
    }
}



unsigned char str_to_int(unsigned char dData)
{      
        unsigned char rst=0;
	if((dData >= '0')&& (dData <= '9'))
          rst = dData-'0';
	else if((dData >= 'A')&&(dData <= 'F'))
          rst =  dData+10-'A';
	else if((dData >= 'a')&&(dData <= 'f'))
          rst =  dData+10-'a';
        else rst = 0;//不是ASCII的char
        
        return rst;
}

/*"1234"---->0X12 0X34  参数3一般是strlen*/
int memcpy_down( void *Respone, void *Data , int length)
{
    unsigned char *respone=(unsigned char *)Respone;
    char *data             =(char *)Data;
    if(length%2 != 0)    return -1;

    for( int i = 0 ; i < length/2 ;i++)
    {
        *respone++ = (((str_to_int(data[i*2])<<4)&0xf0)|(str_to_int(data[i*2+1])&0x0f)); 
    }

    return 0;
    
}

uint64_t atol64( char *str)
{
    uint64_t t[16]={0}; 
    uint8_t i=0 , cnt =0 , j =0;
    uint64_t temp = 0;
    
    while( *str != '\0')
    {
        t[cnt++] = str_to_int(*str);
        str++;
        if( cnt > 16)
        {
            return 0;
        }
    }
    
    j = cnt -1;
    
    for( i = 0 ; i < cnt ; i++)
    {
        temp |= t[j--]<<(i*4);
    }
    
    return temp;
    
}

void sys_delay(uint32_t ms)
{
#define STEP 200
    if(ms < STEP)
    {
        osDelay(ms);
    }
    else
    {
        for(uint16_t i = 0 ; i <  ms/STEP ; i++)
        {
            osDelay(STEP);
            HAL_IWDG_Refresh(&hiwdg);
            task_keep_alive(TASK_ALL_BIT); 
        }
        osDelay( ms%STEP);
        HAL_IWDG_Refresh(&hiwdg);
        task_keep_alive(TASK_ALL_BIT); 
        
    }
}


void read_task_stack(char const *name , xTaskHandle taskHandle)
{
    unsigned portBASE_TYPE uxHighWaterMark; 
    
    uxHighWaterMark = uxTaskGetStackHighWaterMark(taskHandle);

    log(DEBUG,"[%s]stack surplus = %d.\r\n" ,name,uxHighWaterMark);
    
}

        

void soft_system_resert( const char *funs )
{
    log(WARN,"System has been resert form %s\n" , funs);
    NVIC_SystemReset();
}



//123->"1.2.3"
void StringVer(char *s,uint32_t u) 
{
    int Arr[3];
    Arr[0]= u/100;
    Arr[1]= (u/10) - 10*(Arr[0]);
    Arr[2]= u%10;
    sprintf(s,"%d.%d.%d",Arr[0],Arr[1],Arr[2]);
}

//"1.2.3"->123
uint32_t InterVer(char *s)
{
    int Arr[3];
    sscanf((const char*)s,"%d.%d.%d",&Arr[0],&Arr[1],&Arr[2]);
    return Arr[0]*100 + Arr[1]*10 + Arr[2];
}
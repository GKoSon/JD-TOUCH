#include "bsp_rtc.h"
#include "rtc.h"
#include "unit.h"
#include "config.h"


//1480494176
//时间戳 每年的起始值
const uint32_t gdwUnixTimeStampYear[100]=
{ 946656000, 978278400,1009814400,1041350400,1072886400,1104508800,1136044800,1167580800,1199116800,1230739200,//00-1-1 0:0:0 09-1-1 0:0:0
1262275200,1293811200,1325347200,1356969600,1388505600,1420041600,1451577600,1483200000,1514736000,1546272000,//10-1-1 0:0:0
1577808000,1609430400,1640966400,1672502400,1704038400,1735660800,1767196800,1798732800,1830268800,1861891200,//20-1-1 0:0:0
1893427200,1924963200,1956499200,1988121600,2019657600,2051193600,2082729600,2114352000,2145888000,2177424000,//30-1-1 0:0:0
2208960000,2240582400,2272118400,2303654400,2335190400,2366812800,2398348800,2429884800,2461420800,2493043200,//40-1-1 0:0:0
2524579200,2556115200,2587651200,2619273600,2650809600,2682345600,2713881600,2745504000,2777040000,2808576000,//50-1-1 0:0:0
2840112000,2871734400,2903270400,2934806400,2966342400,2997964800,3029500800,3061036800,3092572800,3124195200,//60-1-1 0:0:0
3155731200,3187267200,3218803200,3250425600,3281961600,3313497600,3345033600,3376656000,3408192000,3439728000,//70-1-1 0:0:0
3471264000,3502886400,3534422400,3565958400,3597494400,3629116800,3660652800,3692188800,3723724800,3755347200,//80-1-1 0:0:0
3786883200,3818419200,3849955200,3881577600,3913113600,3944649600,3976185600,4007808000,4039344000,4070880000//90-1-1 0:0:0
};
//100年最大时间戳
const uint32_t gdwUnixTimeMax=4102415990;//2099/12/31 23:59:50     4102415999;//2099/12/31 23:59:59
//每月的1日的 总天数
const uint16_t      gwUnixTimeStampMonth[12]={0, 31,59,90,120,151,181,212,243,273,304,334};
const static uint8_t no_leap_year[12]={6,2,2,5,0,3,5,1,4,6,2,4};//非闰年
const static uint8_t leap_year[12]={5,1,2,5,0,3,5,1,4,6,2,4};//闰年  
const static uint8_t no_leap_year_month_day[12]={31,28,31,30,31,30,31,31,30,31,30,31};//非闰年
const static uint8_t leap_year_month_day[12]={31,29,31,30,31,30,31,31,30,31,30,31};//闰年  

unsigned char is_leap_year(unsigned char year)
{    
    if((year%4==0)&&(year!=0)) 
        return 1;  
    else 
        return 0; 
}

unsigned char RTC_Get_Day(unsigned char year,unsigned char month)
{
    return is_leap_year(year)?leap_year_month_day[month-1]:no_leap_year_month_day[month-1]; 
}

unsigned char get_week(unsigned char year,unsigned char month,unsigned char day)
{
    uint8_t temp;
    
    temp=is_leap_year(year)?leap_year[month-1]:no_leap_year[month-1]; 
    
    return ((day+temp+((year/4+year)%7))%7); 
}

void time_to_stamp(uint32_t *stamp,uint8_t *dt)//由日期时间 转成 时间戳
{
    if((*(dt+1)>0)&&(*(dt+1)<3))
    {
        *stamp = (((gwUnixTimeStampMonth[*(dt+1)-1] + 
                    (*(dt+2)-1))*24 + *(dt+3)) * 60 + 
                    *(dt+4)) * 60 + *(dt+5);
    }
    else
    {
        *stamp = (((gwUnixTimeStampMonth[*(dt+1)-1] + 
                    (*(dt+2)-1) + ((is_leap_year(*(dt+0)))?1:0))*24 + 
                   *(dt+3)) * 60 + *(dt+4)) * 60 + *(dt+5);
    }
    
    *stamp +=gdwUnixTimeStampYear[*(dt+0)];
}


void stamp_to_time(uint32_t *dwStamp,uint8_t *dt)//由 时间戳 转成 日期时间
{
    uint8_t i,j;
    uint16_t wUnixTimeStampMonth[12];
    uint32_t dwD;

    if(*dwStamp>gdwUnixTimeMax)//大于 2099/12/31 23：59：50
    {
        *(dt+0)=99;*(dt+1)=12;*(dt+2)=31;
        *(dt+3)=23;*(dt+4)=59;*(dt+5)=50;
    }
    else
    {
        for(i=0;i<100;i++)
        {
            if(gdwUnixTimeStampYear[i]>*dwStamp)
                break;
        }
        if(i==0)
        {
            for(j=0;j<6;j++)*(dt+i)=0;
                return;
        }
        else
        {
            *(dt+0)=i-1;

            if(is_leap_year(*(dt+0))==1)
            {
                for(j = 0 ; j < 2; j++)
                {
                    wUnixTimeStampMonth[j]=gwUnixTimeStampMonth[j];
                }
                for(j = 2; j < 12; j++)
                {
                    wUnixTimeStampMonth[j]=gwUnixTimeStampMonth[j]+1;
                }
            }
            else
            {
                for(j = 0; j < 12; j++)
                {
                    wUnixTimeStampMonth[j]=gwUnixTimeStampMonth[j];
                }
            }
            //printf("年：%d\r\n",*(dt+0));
            dwD=*dwStamp-gdwUnixTimeStampYear[i-1];
            //printf("dwD Zs=%d\r\n",dwD);
            *(dt+5)=dwD%60;
            //printf("dwD 秒=%d\r\n",*(dt+5));
            dwD=dwD/60;
            *(dt+4)=dwD%60;
            //printf("dwD 分钟=%d\r\n",*(dt+4));
            dwD=dwD/60;
            *(dt+3)=dwD%24;
            //printf("dwD 小时=%d\r\n",*(dt+3));
            dwD=dwD/24;
            //printf("dwD 天数=%d\r\n",dwD);
            for(i=0;i<12;i++)
            {
                if(wUnixTimeStampMonth[i]>dwD)
                    break;
            }
            *(dt+1)=i;
            *(dt+2)=(dwD-wUnixTimeStampMonth[i-1])+1;
        }
    }
}


void rtc_set_time( rtcTimeType *time)
{
    hal_rtc_set(time->year,time->mon,time->day,time->hour,time->min,time->sec ,time->week);
}


uint32_t rtc_read_stamp( void )
{
    uint32_t unix_stamp = 0;
    
	uint8_t dataTime[7];

	hal_rtc_read(dataTime);
	
	time_to_stamp(&unix_stamp , dataTime);

    return unix_stamp;
}

void rtc_read_time(rtcTimeType *time)
{
    uint8_t dataTime[7];
    
    hal_rtc_read(dataTime);
    
    time->year = dataTime[0];
    time->mon = dataTime[1];
    time->day = dataTime[2];
    time->hour = dataTime[3];
    time->min = dataTime[4];
    time->sec = dataTime[5];
    time->week = dataTime[6];
   
}

uint32_t rtc_time_to_stamp(rtcTimeType *time)
{
    uint32_t unix_stamp = 0;
    uint8_t dataTime[7];
    
    dataTime[0] = time->year;
	dataTime[1] = time->mon;
	dataTime[2] = time->day;
	dataTime[3] = time->hour;
	dataTime[4] = time->min;
	dataTime[5] = time->sec;
    dataTime[5] = time->week;
    
    time_to_stamp(&unix_stamp , dataTime);

    log(DEBUG,"Input = 20%d-%02d-%02d %02d:%02d:%02d, output stamp =%ld\n" ,
                dataTime[0],dataTime[1],dataTime[2] ,dataTime[3],dataTime[4],dataTime[5] ,unix_stamp);    
    
    
    return unix_stamp;
}

void rtc_stamp_to_time(rtcTimeType *time ,uint32_t unix_stamp)
{
    uint8_t dataTime[7];


    stamp_to_time(&unix_stamp ,dataTime );
                 
    time->year = dataTime[0];
    time->mon = dataTime[1];
    time->day = dataTime[2];
    time->hour = dataTime[3];
    time->min = dataTime[4];
    time->sec = dataTime[5];

    time->week = get_week(time->year , time->mon , time->day);
    
    //log(DEBUG,"Input stamp:%ld ,out time:20%d-%02d-%02d %02d:%02d:%02d\n" ,unix_stamp,time->year,time->mon,time->day,
    //                                                  time->hour,time->min,time->sec);    
        
}


void HAL_RTC_AlarmAEventCallback(RTC_HandleTypeDef *hrtc)
{
    log(WARN,"Rtc alarm\r\n");
    
    rtcTimerEnable = TRUE;

}
void rtc_alarm_callback( void)
{
    //sysOperationType p;
    
    //portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
    
    //p.handle = SYS_RTC_ALARM_IRQ;
    //xQueueSendFromISR( xOperationQueue, ( void* )&p, &xHigherPriorityTaskWoken );  
}

void rtc_set_time_form_stamp( uint32_t unix_stamp)
{
    rtcTimeType time;
    
    rtc_stamp_to_time(&time , unix_stamp);
    
    hal_rtc_set(time.year,time.mon,time.day,time.hour,time.min,time.sec,time.week);
}


rtcType    rtc=
{
    .set_time               = rtc_set_time,
    .set_time_form_stamp    = rtc_set_time_form_stamp,
    .read_stamp             = rtc_read_stamp,
    .read_time              = rtc_read_time,
    .time_to_stamp          = rtc_time_to_stamp,
    .stamp_to_time          = rtc_stamp_to_time,
};

/*
void rtc_init ( void )
{
    stm32_rtc_init();
}

INIT_EXPORT(rtc_init , "rtc");
*/
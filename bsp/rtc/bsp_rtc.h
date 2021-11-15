#ifndef _BSP_RTC_H_
#define _BSP_RTC_H_

#include <stdint.h>


typedef struct
{
    uint8_t year;
    uint8_t mon;
    uint8_t day;
    uint8_t hour;
    uint8_t min;
    uint8_t sec;
    uint8_t week;
}rtcTimeType;

typedef struct
{
    void        (*set_time)                 (rtcTimeType *time);
    void        (*set_time_form_stamp)      (uint32_t unix_stamp);
	uint32_t    (*read_stamp)	            ( void );
	void        (*read_time)                (rtcTimeType *time);
    uint32_t    (*time_to_stamp)            (rtcTimeType *time);
    void        (*stamp_to_time)            (rtcTimeType *time ,uint32_t unix_stamp);
}rtcType;

extern void rtc_alarm_callback( void);

extern rtcType  rtc;


#endif




#ifndef _UINT_H_
#define _UINT_H_

#include "stdio.h"
#include "stdint.h"
#include "cmsis_os.h"
#include "stm32l4xx_hal.h"
#include "cpu_utils.h"
#include "bsp_rtc.h"
#include "err_log.h"
#include "iwdg.h"


#ifndef NULL
#define NULL                (void*)(0)
#endif

#ifndef FALSE
#define FALSE                0
#endif
#ifndef TRUE
#define TRUE                !FALSE
#endif

#ifndef BOOL
#define BOOL                bool
#endif
#ifndef false
#define false                FALSE
#endif
#ifndef true
#define true                TRUE
#endif

#define BIT0        (0x00000001ul)
#define BIT1        (0x00000002ul)
#define BIT2        (0x00000004ul)
#define BIT3        (0x00000008ul)
#define BIT4        (0x00000010ul)
#define BIT5        (0x00000020ul)
#define BIT6        (0x00000040ul)
#define BIT7        (0x00000080ul)
#define BIT8        (0x00000100ul)
#define BIT9        (0x00000200ul)
#define BIT10    (0x00000400ul)
#define BIT11    (0x00000800ul)
#define BIT12    (0x00001000ul)
#define BIT13    (0x00002000ul)
#define BIT14    (0x00004000ul)
#define BIT15    (0x00008000ul)
#define BIT16    (0x00010000ul)
#define BIT17    (0x00020000ul)
#define BIT18    (0x00040000ul)
#define BIT19    (0x00080000ul)
#define BIT20    (0x00100000ul)
#define BIT21    (0x00200000ul)
#define BIT22    (0x00400000ul)
#define BIT23    (0x00800000ul)
#define BIT24    (0x01000000ul)
#define BIT25    (0x02000000ul)
#define BIT26    (0x04000000ul)
#define BIT27    (0x08000000ul)
#define BIT28    (0x10000000ul)
#define BIT29    (0x20000000ul)
#define BIT30    (0x40000000ul)
#define BIT31    (0x80000000ul)

#ifndef U8_MAX
#define U8_MAX     (255)
#endif
#ifndef S8_MAX
#define S8_MAX     (127)
#endif
#ifndef S8_MIN
#define S8_MIN     (-128)
#endif
#ifndef U16_MAX
#define U16_MAX    (65535u)
#endif
#ifndef S16_MAX
#define S16_MAX    (32767)
#endif
#ifndef S16_MIN
#define S16_MIN    (-32768)
#endif
#ifndef U32_MAX
#define U32_MAX    (4294967295uL)
#endif
#ifndef S32_MAX
#define S32_MAX    (2147483647)
#endif
#ifndef S32_MIN
#define S32_MIN    (-2147483648uL)
#endif
/**
  * @brief  This macro returns the maximum value between two inputs,
  *                    the evaluation is done with the ">" operator
  * @param  a  First input
  * @param  b  Second input  
  * @retval Max(a,b)
  */
#define MAX(a,b)        ((((uint32_t)(a)) > ((uint32_t)(b)))  ? (a)  : (b))

/**
  * @brief  This macro returns the minumum value between two inputs,
  *                     the evaluation is done with the ">" operator
  * @param  a  First input
  * @param  b  Second input  
  * @retval Min(a,b)
  */
#define MIN(a,b)        ((((uint32_t)(a)) > ((uint32_t)(b)))  ? (b)  : (a))


#define GET_STR_BYTE(x ,i)  (((strToInt(x[i])<<4)&0xf0)|(strToInt(x[i+1])&0x0f)) 

extern uint32_t    sysDebugFlag;
                                      

#define ERR                 1
#define WARN                2
#define INFO                3
#define DEBUG               4

#define LOG_PRESENT_BIT(x)        (((uint32_t)((uint32_t)1<<(x))))
#define LOG_BIT_ON(m, b)          (((m) & LOG_PRESENT_BIT(b)) != 0)
#define LOG_SET_BIT(m, b)         ((m) |= LOG_PRESENT_BIT(b))
#define LOG_CLEAR_BIT(m, b)       ((m) &= ~LOG_PRESENT_BIT(b))

void printf_time( void );

void log_err( char* fmt, ...);

#define log(level, fmt, args...)  do{\
    if(LOG_BIT_ON(sysDebugFlag,level)){\
        if(level == ERR){\
        printf("\033[0;31m");}\
        else if(level == WARN)\
        printf("\033[0;32m");\
        else if(level == INFO)\
        printf("\033[0;33m");\
        else if(level == DEBUG)\
        printf("\033[0;37m");\
        printf("[#]");\
        printf("[%d%]" , osGetCPUUsage());\
        printf_time();\
        printf(fmt, ##args);\
        printf("\033[0;39m");\
    }\
}while(0)


void read_task_stack( char const *name , xTaskHandle taskHandle);

void sys_delay(uint32_t ms);

uint64_t atol64( char *str);

void log_arry(uint32_t level ,unsigned char *pst , unsigned char *arry , unsigned int leng);

void log_arry10(uint32_t level ,unsigned char *pst , unsigned char *arry , unsigned int leng);

unsigned char aiot_strcmp( unsigned char *pst , unsigned char *str , unsigned char len);

void soft_system_resert( const char *funs );

#define GMIN(a,b)  ((a)<(b))?(a):(b) 

#define SHOWME       log(ERR,"#文件%s##函数%s##行数%d##\r\n",__FILE__,__FUNCTION__,__LINE__);
#define NEVERSHOW    log(ERR,"\r\n----*********-----*********-----*********------*********------【%d】【%s】\r\n",__LINE__,__func__);

uint8_t Gequal(uint32_t A,uint32_t B,uint8_t range);

int memcpy_down( void *respone, void *data , int length);

void memcpy_up(void* strings, void* bytes,unsigned char len);

uint8_t mycrc8(uint8_t *ps1,uint8_t uLen);

unsigned short CRC16_CCITT(unsigned char *puchMsg, unsigned int usDataLen) ;

uint8_t bcd_to_bin(uint8_t bcd);

#define GMAX_LEN_TOPIC        55/* 文本最长33 + 点码是22 那就是55 */
extern char topicPath0[];
extern char topicPath1[GMAX_LEN_TOPIC];
extern char topicPath2[GMAX_LEN_TOPIC];
extern char topicPath3[GMAX_LEN_TOPIC];
extern char topicPath4[GMAX_LEN_TOPIC];

void StringVer(char *s,uint32_t u) ;

uint32_t InterVer(char *s);

char * ip_port_handle(char *  sor);


extern char wrirenfc;
extern char diyota;

#endif

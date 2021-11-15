#ifndef _SYSCNTSAVE_H_
#define _SYSCNTSAVE_H_

//必须能被4096整除
typedef struct
{
    uint32_t listUpdataTime;
    uint32_t listRainbowUseTime;
    uint32_t oncePwdTime;
    uint32_t temp;
}sysCntSaveType;


void syscnt_clear_all( void );
void syscnt_write_user_time( uint32_t time);
void syscnt_write_list_time( uint32_t time);
void syscnt_write_oncepwd_time( uint32_t time);

extern sysCntSaveType  sysCnt;


#endif


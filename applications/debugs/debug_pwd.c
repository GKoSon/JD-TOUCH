#include "components_ins.h"
#include <stdlib.h>
#include "sysCfg.h"
#include "rtc.h"
#include "tempwd.h"
#include "iwdg.h"
void debug_pwd_show_all( void )
{
    int32_t  tempPwd=0 , cnt =0;
    tempwdType  pwd;
    
    flash.get_lock();
    for(uint32_t i = 0 ; i < PWD_MAX_NUM ; i++)
    {
        flash.read(PASSWORD_BOOT_ADDR+i*PWD_INDEX_SIZE , (uint8_t *)&tempPwd , 4);
        if(tempPwd != 0xFFFFFFFF)
        {
            flash.read(PASSWORD_DATA_ADDR+i*PWD_SIZE , (uint8_t *)&pwd , PWD_SIZE);
            log(DEBUG , "[%d]INDEX = %d ,PWD = %04x , TIME = %u\n" , i ,tempPwd, pwd.pwd , pwd.time);
            cnt++;
        }
        HAL_IWDG_Refresh(&hiwdg);
    }
    flash.release_lock();
    if(cnt ==0 )
    {
        log(DEBUG,"设备没有临时密码\n");
    }
}

int debug_pwd(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{

    if( strcmp("-s" , argv[1]) == 0)
    {
        if( strcmp("-a" , argv[2]) == 0)
        {
            debug_pwd_show_all();
        }
    }
    if( strcmp("-clr" , argv[1]) == 0)
    {
        tempwd.clear_overdue();
    }
    
    if( strcmp("-a" , argv[1]) == 0)
    {
        tempwdType apwd;
        
        apwd.time = atol(argv[3]);
                
        apwd.pwd = str_to_hex(argv[2][0])<<12|str_to_hex(argv[2][1])<<8|str_to_hex(argv[2][2])<<4|str_to_hex(argv[2][3]);
        
        tempwd.add(&apwd);
    }
    
    if( strcmp("-m" , argv[1]) == 0)
    {
        tempwdType apwd;
        uint32_t intpwd , cntMax;
        char strPwd[4];
        
        apwd.time = atol(argv[4]);
        
        intpwd = atol(argv[2]);
        cntMax = atol(argv[3]);
        
        if(( cntMax > 5000) || ( intpwd > 9999) || ((intpwd+cntMax) > 9999))
        {
            log(WARN,"输入数字过大，请重新输入\n");
            return 0;
        }
        
        for(uint16_t i = 0 ; i < cntMax ; i++)
        {
            sprintf(strPwd , "%04d" , intpwd);
            apwd.pwd = str_to_hex(strPwd[0])<<12|str_to_hex(strPwd[1])<<8|str_to_hex(strPwd[2])<<4|str_to_hex(strPwd[3]);
            tempwd.add(&apwd);
            HAL_IWDG_Refresh(&hiwdg);
            intpwd++;
        }
        
       
    }
    
	return 1;
}


       
U_BOOT_CMD(
    pwd,     6,     1,      debug_pwd,
	"pwd\t-一次性密码调试  \n",
	"\t-一次性密码调试\n\t\
     pwd -a [pwd] , [timer] , examp:pwd -a 1111 1900000000 增加临时密码\n\t\
	 pwd -m [pwd],[add number],[timer] , examp:pwd -m 1111 10 1900000000 批量增加临时密码\n\t\
     pwd -clr 清空过期密码\n"
);


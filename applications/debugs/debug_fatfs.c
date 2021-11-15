#include "components_ins.h"
#include "bsp_rtc.h"
#include "err_log.h"
#include <stdlib.h>

int debug_fatfs (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    if( argc < 2)
    {
        log_err("No parameter input\n");
        return -1;
    }
    
    if( strcmp("-s" , argv[1]) == 0)
    {
        if( strcmp("-c" , argv[2]) == 0)
        {
            //err_log_get_capacity();
        }        
    }
    
    if( strcmp("cat" , argv[1]) == 0)
    {
        err_log_read();       
    }
    
    if( strcmp("format" , argv[1]) == 0)
    {
        err_log_format();       
    }
	return 0;
}


U_BOOT_CMD(
    log,     3,     1,      debug_fatfs,
	"log\t-错误日志调试入口\n",
	"-s -a\t-显示当前设备容量信息\n"
);

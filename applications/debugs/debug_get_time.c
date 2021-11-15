#include "components_ins.h"
#include "bsp_rtc.h"
#include <stdlib.h>

int debug_get_time (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    uint8_t mode = *argv[1];
   

    
    if( argc < 2)
    {
        log_err("No parameter input\n");
        return -1;
    }
    
    switch( mode )
    {
        case 't':
        {
            rtcTimeType time;
            
            rtc.read_time(&time);
            
            log(INFO,"Time  =  20%02d-%02d-%02d %02d:%02d:%02d ,week:%d\r\n" ,
                                            time.year,time.mon,time.day,
                                            time.hour,time.min,time.sec,
                                            time.week);

            
        }break;
        case 'u':
        {
            log(INFO,"Unix stamp = %ld.\n" ,rtc.read_stamp());
        }break;
        case 's':
        {
            uint32_t stamp = 0;
            rtcTimeType time;
            
            stamp = atoi(argv[2]);
            
            rtc.set_time_form_stamp(stamp);
            
            rtc.read_time(&time);
            
            log(INFO,"Time  =  20%02d-%02d-%02d %02d:%02d:%02d ,week:%d\r\n" ,
                                            time.year,time.mon,time.day,
                                            time.hour,time.min,time.sec,
                                            time.week);

        }break;
        default:
        {
            log_err("Without this parameter\n");
        }break;
    }
    
	return 0;
}


U_BOOT_CMD(
    time,     3,     1,      debug_get_time,
	"time\t-显示当前设备的时间\n",
	"\t-显示当前设备的时间\n\t-t:显示年月日时间\n\t-u:显示unix时间错\n"
);

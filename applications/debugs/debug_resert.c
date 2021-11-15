#include "components_ins.h"
#include "unit.h"
#include "tempwd.h"
#include "permi_list.h"
#include "open_log.h"

int debug_resert(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{

    if( strcmp("-restore" , argv[1]) == 0)
    {
        log(WARN,"设备即将恢复出厂设置...\n");
        
        if( strcmp("-all" , argv[2]) == 0)
        {   
            permi.clear();
            journal.clear();
            tempwd.clear();
        }
        
        config.write(CFG_SET_RESTORE , NULL , FALSE);
    }
    soft_system_resert(__func__);
    return 0;
}


U_BOOT_CMD(
    resert,     3,     1,      debug_resert,
	"resert\t-复位设备  \n",
	"\t-复位设备\n\t\
     resert -restore 设备恢复出厂设置，并自动重启\n"
);


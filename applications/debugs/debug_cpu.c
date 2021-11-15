#include "components_ins.h"
#include "cpu_utils.h"
#include "unit.h"


int debug_cpu (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{

    if( strcmp("-s" , argv[1]) == 0)
    {
        log(DEBUG,"CPU使用频率 = %d%\n" ,osGetCPUUsage());
    }


    return 0;
}


U_BOOT_CMD(
    cpu,     3,     1,      debug_cpu,
	"cpu\t-查看CPU使用频率\n",
	"\t-查看CPU使用频率\n\n\
cpu -s\t-查看CPU使用频率\n"
);

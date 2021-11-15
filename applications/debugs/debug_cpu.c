#include "components_ins.h"
#include "cpu_utils.h"
#include "unit.h"


int debug_cpu (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{

    if( strcmp("-s" , argv[1]) == 0)
    {
        log(DEBUG,"CPUʹ��Ƶ�� = %d%\n" ,osGetCPUUsage());
    }


    return 0;
}


U_BOOT_CMD(
    cpu,     3,     1,      debug_cpu,
	"cpu\t-�鿴CPUʹ��Ƶ��\n",
	"\t-�鿴CPUʹ��Ƶ��\n\n\
cpu -s\t-�鿴CPUʹ��Ƶ��\n"
);

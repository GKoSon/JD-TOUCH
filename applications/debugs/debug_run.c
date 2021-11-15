#include "components_ins.h"
#include "bsp_rtc.h"
#include <stdlib.h>

int debug_run_set (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{


    
	return 0;
}



U_BOOT_CMD(
    run,     4,     1,      debug_run_set,
	"run\t-设置设备运行模式\n",
	"\t-设置设备运行模式\n\t-bt module -nfc module,参数选填\n\t\
例如：run -ST95 , run -ST25 , run -BM77 ,run -BB0906 , run -BM77 -ST95 , run -ST25 -BB0906\n"
);

#include "components_ins.h"
#include "bsp_rtc.h"
#include <stdlib.h>

int debug_run_set (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{


    
	return 0;
}



U_BOOT_CMD(
    run,     4,     1,      debug_run_set,
	"run\t-�����豸����ģʽ\n",
	"\t-�����豸����ģʽ\n\t-bt module -nfc module,����ѡ��\n\t\
���磺run -ST95 , run -ST25 , run -BM77 ,run -BB0906 , run -BM77 -ST95 , run -ST25 -BB0906\n"
);

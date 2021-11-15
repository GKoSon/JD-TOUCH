#include "components_ins.h"
#include "unit.h"
#include "tempwd.h"
#include "permi_list.h"
#include "open_log.h"

int debug_resert(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{

    if( strcmp("-restore" , argv[1]) == 0)
    {
        log(WARN,"�豸�����ָ���������...\n");
        
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
	"resert\t-��λ�豸  \n",
	"\t-��λ�豸\n\t\
     resert -restore �豸�ָ��������ã����Զ�����\n"
);


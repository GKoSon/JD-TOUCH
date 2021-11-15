#include "components_ins.h"


#include "sysCfg.h"



void debug_nfc_show_community_msg( void )
{
   
}



int debug_nfc (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{

    if( strcmp("show" , argv[1]) == 0)
    {
        if( strcmp("comm" , argv[2]) == 0)
        {
            debug_nfc_show_community_msg();
        }
        else
        {
            log_err("No parameter input\n");
        }
    }


    return 0;
}


U_BOOT_CMD(
    nfc,     3,     1,      debug_nfc,
	"nfc\t-NFC信息调试\n",
	"\t-NFC信息调试\n\n\
nfc show\t显示NFC信息,如设备当前小区楼栋单元编号等\n\t\
    -comm:显示设备目前所支持的小区楼栋单元编号\n"
);

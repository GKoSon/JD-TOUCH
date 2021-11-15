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
	"nfc\t-NFC��Ϣ����\n",
	"\t-NFC��Ϣ����\n\n\
nfc show\t��ʾNFC��Ϣ,���豸��ǰС��¥����Ԫ��ŵ�\n\t\
    -comm:��ʾ�豸Ŀǰ��֧�ֵ�С��¥����Ԫ���\n"
);

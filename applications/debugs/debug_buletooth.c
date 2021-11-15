#include "components_ins.h"

/*#include "bluetoothOperation.h"
#include "sysCfg.h"


void debug_buletooth_show_cmd( void )
{
    bt_cmd_tbl_t *cmdtp;
    uint8_t count = 0;
    
    log(INFO , "This is interactive command for all Bluetooth and APP\n");
	for (cmdtp = __bt_cmd_start;cmdtp != __bt_cmd_end;cmdtp++)
    {
		log(DEBUG , "----cmd = %02d , name = %s.\n" ,cmdtp->id , cmdtp->name);
        count++;
	}
    log(INFO , "There are %d orders in all.\n" , count);
}

void debug_buletooth_show_versoion( void )
{
    log(DEBUG,"Bluetooth version number : %d\n" ,config.read(CFG_BLE_VERSION , NULL));
}


void debug_buletooth_show_pwd( void )
{
    uint8_t *pair_pwd , *user_pwd;
    
    config.read(CFG_PAIR_PWD , (void **)&pair_pwd);
      
    config.read(CFG_USER_PWD , (void **)&user_pwd);
    
    log_arry(DEBUG,"Buletooth pair password" , pair_pwd ,3);
    log_arry(DEBUG,"Buletooth user password" , user_pwd ,3);
}



int debug_buletooth (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{

    if( strcmp("show" , argv[1]) == 0)
    {
        if( strcmp("cmd" , argv[2]) == 0)
        {
            debug_buletooth_show_cmd();
        }
        else if( strcmp("pwd" , argv[2]) == 0)
        {
            debug_buletooth_show_pwd();
        }
        else
        {
            log_err("No parameter input\n");
        }
    }
    else if( strcmp("--v" , argv[1]) == 0)
    {
        debug_buletooth_show_versoion();
    }
    else
    {
        log_err("Parameter input is error\n");
    }


    return 0;
}


U_BOOT_CMD(
    bt,     3,     1,      debug_buletooth,
	"bt\t-调试蓝牙模块\n",
	"\t-调试蓝牙模块\n\n\
bt show\t显示蓝牙信息\n\t\
    -cmd:显示所有蓝牙交互命令\n\t\
    -pwd:显示蓝牙密码\n\
bt --v\t显示蓝牙模块版本号\n"
);*/
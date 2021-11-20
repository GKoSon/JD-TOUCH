#include "modules_init.h"
#include "unit.h"
#include "iwdg.h"
#include "chipFlash.h"

modules_init_tbls  *__modules_init_start = __section_begin("__modules_init");
modules_init_tbls  *__modules_init_end = __section_end("__modules_init");

void modules_init( void )
{
    modules_init_tbls *tp;

    for (tp = __modules_init_start; tp < __modules_init_end; tp++)
    {
        HAL_IWDG_Refresh(&hiwdg);
        if( tp != NULL)
        { 
            mdu_log(DEBUG,"[modules_init]%s\n", tp->name);
            tp->fun();
        }
        else
        {
            mdu_log(ERR , "conponents init is error , start=%x , end = %x\n" ,
                        __modules_init_start ,__modules_init_end );
        }
    }
    chip_flash_init();
}

#include "component.h"
#include "iwdg.h"
#include "permi_list.h"

component_init  *__component_start = __section_begin("__component_init");
component_init  *__component_end = __section_end("__component_init");

void components_init( void )
{
    component_init *tp;
    
    for (tp = __component_start; tp < __component_end; tp++)
    {
        HAL_IWDG_Refresh(&hiwdg);
        if( tp != NULL)
        { 
            comp_log(DEBUG,"[components_init]%s\n", tp->name);
            tp->fun();
        }
        else
        {
            comp_log(ERR , "conponents init is error , start=%x , end = %x\n" ,
                        __component_start ,__component_end );
        }
    }
    permiList_clear_overdue();

}

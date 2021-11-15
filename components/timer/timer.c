#include "timer.h"
#include "stdbool.h"

uint8_t			timerTaskId = 0;
//osal time queue
time_type		    *time = NULL;


void timer_isr( void )
{
	time_type 	 *priv = time;

	while( priv != NULL )
    {
        if( priv->start)
        {
            if( ++priv->cnt >= priv->time_out)
            {
                priv->cnt = 0;
                if(priv->fun != NULL)	priv->fun();
            }
        }
        priv = priv->next;      
	}
}


void *timer_malloc(size_t size)
{
	void *address;

	address = malloc(size);
	if(address == NULL)
	{
		log_err("memory is full.\r\n");
		soft_system_resert(__func__);
		return (address);
	}
    memset(address , 0x00 , size);

    return (address);
}

uint8_t timer_stop_time(uint8_t handle)
{
	time_type *priv = time;
	
	while( priv != NULL )
    {
        if( priv->handle == handle)
        {
            priv->start = false;
			priv->cnt = 0;
            return true;
        }
        priv = priv->next;     
	}  
    
    return false;
}

uint8_t timer_start_time(uint8_t handle)
{
	time_type *priv = time;
	
	while( priv != NULL )
    {
        if( priv->handle == handle)
        {
            priv->start = true;
            priv->cnt = 0;
            return true;
        }
        priv = priv->next;     
	}  
    
    return false;
}

uint8_t timer_register_isr(  uint32_t time_out ,uint8_t start, time_call_back call_back)
{
	time_type *priv;
	time_type	*this;

	this = (time_type *)timer_malloc(sizeof(time_type));
	if( this != NULL)
	{

		this->cnt = 0;
		this->start = start;
		this->handle = timerTaskId++;
		this->time_out = time_out;
		this->fun = call_back;
		this->next = NULL;
        if( time == NULL)
		{
	 		time = this;
		}
		else
		{
			priv = time;
			while( priv->next != NULL )	priv = priv->next;
			priv->next = this;
		}    
	}
	else
	{
		return 0xFF;
	}


    return (this->handle);

}


time_ops_type   timer =
{
    .creat = timer_register_isr,
    .stop = timer_stop_time , 
    .start = timer_start_time,
};

/*
void timer_init( void )
{
    bsp_hwtimer_ops.register_isr(timer_isr);
    if(bsp_hwtimer_ops.init(1000000 , 1000))
    {
        log_err("Timer creat error.\r\n");
    }
}
#include "modules_init.h"
MODULES_INIT_EXPORT(timer_init , "timer");
*/

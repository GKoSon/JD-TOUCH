#include "card.h"
#include "swipeTag.h"
#include "config.h"
#include "beep.h"
#include "timer.h"
#include "tagComponent.h"

static xTaskHandle     swipTask;

void swipe_task( void const *pvPar )
{
    configASSERT( ( ( unsigned long ) pvPar ) == 0 );
    uint8_t result = TAG_NONE , rt = TAG_LIST_NULL;
    tagBufferType tag;
    while(1)
    {
          result = read_tag(&tag);

          if(( result != TAG_SAME_ID_ERR)&&(result != TAG_NONE))
          {
                  if( ( rt = read_list_name(&tag)) != TAG_LIST_NULL )
                  {
                          result = rt;
                  }
          }
          
          if( result == TAG_SUCESS)
          {
                result = tag_data_process(&tag);
          }
               
          tag_interaction_buzzer(&tag , &result); 
          tagComp->turnOffField(&tag);
          memset(&tag , 0x00 , sizeof(tagBufferType));
          task_keep_alive(TASK_SWIPE_BIT); 
    }
}


void creat_swipe_task( void )
{  
    osThreadDef( swipe, swipe_task , osPriorityRealtime, 0, configMINIMAL_STACK_SIZE*6);
    swipTask = osThreadCreate(osThread(swipe), NULL);
    configASSERT(swipTask);   
}

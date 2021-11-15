#include "tagComponent.h"
#include "st25Drv.h"
#include "component.h"
#include "sysCfg.h"

tagObjType	*tagComp = NULL;


void tag_component_init( void )
{
      log(DEBUG,"NFCÊ¹ÓÃST25\n");
      tagComp = &st25Tag;
      tagComp->init();
}



#include "sysCfg.h"
#include "unit.h"
#include "relay.h"
#include "mate.h"

uint16_t openDoorIndex = 0xFFFF;



void open_door( void )
{
    relay.open(config.read(CFG_SYS_OPEN_TIME,NULL));
    SHOWME
}



void close_door( void )
{
    relay.control (0);
    SHOWME
}

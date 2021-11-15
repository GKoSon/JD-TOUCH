#include "sysCfg.h"
#include "unit.h"
#include "relay.h"
#include "mate.h"

uint16_t openDoorIndex = 0xFFFF;



void open_door( void )
{
    relay.open(config.read(CFG_SYS_OPEN_TIME,NULL));

}



void close_door( void )
{
    relay.control (0);
    printf("\r\n--relay.control (0)--\r\n");
}


void open_ladder( void )
{

}
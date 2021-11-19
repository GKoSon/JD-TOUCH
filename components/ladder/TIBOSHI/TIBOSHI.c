
#include "ladder.h"

#if defined (TIBOSHI)
#include "stdint.h"
#include "unit.h"

uint8_t data_check(uint8_t *data , uint8_t len)
{
    uint8_t crc = 0;
    
    for(uint8_t i = 0 ; i < len ;i++)
    {
        crc += data[i];
    }
    
    return crc;
}

uint8_t  get_control_data(LadderUserMsgType *pst , uint8_t *respone)
{
    uint8_t buff[]={0xFA,0XA2,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00};
    uint8_t autoSelect = 0 ,userFloorNum = 1;;
    
    log(DEBUG,"用户楼层权限有:");
    for( uint8_t i = 0 ; i < 7 ; i++ )
    {
        for(uint8_t j = 0 ; j < 8 ; j++)
        {
            if( (pst->floor[i]&(0x01<<j) ) )
            {
                autoSelect++;
                printf("%d " , userFloorNum);
            } 
            userFloorNum++;
        }
    }
    
    buff[1] =  (autoSelect>1)?0xA1:0xA2;
    
    memcpy(buff+2 , pst->floor , 7);
    
    buff[9]=data_check(buff , 9);
    
    memcpy(respone , buff , 10);
    
    return 10;
}


#endif
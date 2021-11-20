
#ifndef _PROTOCOL_OLD_ANALYSIS_H_
#define _PROTOCOL_OLD_ANALYSIS_H_


#include "stdint.h"
#include "unit.h"
#include "bb0906.h"

#define MSB(x)                          ((x)>>8) 
#define LSB(x)                          ((x)&0xff) 


#define BLEMODE_PHONE_MAX               (2)
#define BLEMODE_SEND_MAX_LENG           (BLEMODE_FARM_MAX*BLEMODE_PHONE_MAX)

#define HAL_READ_TIME_OUT_GAP           (5)


#define BLE_CALSEE_LENG                 (4)
#define BLE_COMMAND_LENG                (2)
#define BLE_RESPONSE_LENG               (1)
#define BLE_LENG_LENG                   (2)
#define BLE_CRC_LENG                    (1)

#define BLE_HEARD_SIZE                  (BLE_CALSEE_LENG+BLE_COMMAND_LENG+BLE_RESPONSE_LENG+BLE_LENG_LENG+BLE_CRC_LENG)              

#define BLE_ADDR_SIZE                   (6)
#define BLE_HANDLE_SIZE                 (2)
#define BLE_WRITE_TYPE_SIZE             (1)
#define BLE_DATA_HEARD_LENG             (BLE_ADDR_SIZE+BLE_HANDLE_SIZE+BLE_WRITE_TYPE_SIZE)

#define USART_CLASS_BIT                 (0)
#define USART_CMD_BIT                   (USART_CLASS_BIT+BLE_CALSEE_LENG)
#define USART_RESPONE_BIT               (USART_CMD_BIT+BLE_COMMAND_LENG)
#define USART_LENG_BIT                  (USART_RESPONE_BIT+BLE_RESPONSE_LENG)
#define USART_DATA_BIT                  (USART_LENG_BIT+BLE_LENG_LENG)


typedef enum
{
    HEARD1_POS = 0,
    HEARD2_POS,
    HEARD3_POS,
    HEARD4_POS,
    COMMAND_H_POS,
    COMMAND_L_POS,
    RESPONSE_POS,
    LENGTH_H_POS,
    LENGTH_L_POS,
    ADDR_POS,
    HANDLE_H_POS,
    HANDLE_L_POS,
    WRITE_TYPE_POS,
    DATA_POS,
    CRC_POS,
    DATA_TRANS,
}UsartPosType;





extern void BleReceiveUsartByteHandle( uint8_t ucData);
extern void ble_clear_buffer( void );
extern void ble_easyclear_buffer( char i );
#endif

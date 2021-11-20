#ifndef _READ_TAG_H_
#define _READ_TAG_H_

#include "stdint.h"
#include "config.h"

#define    TRACK_NONE             0x00
#define    TRACK_NFCTYPE1         0x01 /* 0000 0001 */
#define    TRACK_NFCTYPE2         0x02 /* 0000 0010 */
#define    TRACK_NFCTYPE3         0x04 /* 0000 0100 */
#define    TRACK_NFCTYPE4A        0x08 /* 0000 1000 */
#define    TRACK_NFCTYPE4B        0x10 /* 0001 0000 */
#define    TRACK_NFCTYPE5         0x20 /* 0010 0000 */


#define    TRACK_NOTHING          0x00
#define    TRACK_NFCTYPE1         0x01 /* 0000 0001 */
#define    TRACK_NFCTYPE2         0x02 /* 0000 0010 */
#define    TRACK_NFCTYPE3         0x04 /* 0000 0100 */
#define    TRACK_NFCTYPE4A        0x08 /* 0000 1000 */
#define    TRACK_NFCTYPE4B        0x10 /* 0001 0000 */
#define    TRACK_NFCTYPE5         0x20 /* 0010 0000 */
#define    TRACK_ALL              0xFF /* 1111 1111 */


typedef enum
{
    TAG_SUCESS,
    TAG_WRITE_LIST,
    TAG_ERR,
    TAG_BALCK_LIST_ERR,//3
    TAG_LIST_NULL,
    TAG_TYPE4A_ERR,
    TAG_TYPE4B_ERR,
    TAG_NONE,
    TAG_NULL,//8
    TAG_NO_SUPPORT,
    TAG_SAME_ID_ERR,//10
    TAG_CRC_ERR,
    TAG_COMM_ERR,
    TAG_TIME_ERR,


}tagDataErrEnum;


typedef enum 
{
    INIT_TAG = 0,
    ID_TAG,
    USER_TAG,
    MANAGENT_TAG,
    TEMP_TAG ,
    
    BLE_CARD,
}tagPowerEnum;

typedef enum
{
    UNKNOW_CARD,
    TAG_ID_CARD,
    TAG_PHONE_CARD,
    TAG_SHANGHAI_CARD,
}tagTypeEnum;


typedef struct
{
    tagTypeEnum     type;
    uint8_t         cardType;
    tagPowerEnum    tagPower;
    uint8_t         allDataCrc;
    uint8_t         UID[16];
    uint8_t         UIDLength;
    uint8_t         buffer[128];
    uint8_t         sak;
    uint8_t         readFinshFlag;
}tagBufferType;

extern uint8_t readFishFlag ;


uint8_t read_tag( tagBufferType *tag);
uint8_t read_list_name( tagBufferType *tag);
uint8_t tag_data_process( tagBufferType *tag);
void tag_interaction_buzzer( tagBufferType *tag , uint8_t *result);


#endif

#ifndef _CMD_PB_H_
#define _CMD_PB_H_



#include "sysCfg.h"
#include "unit.h"

#include "pb.h"
#include "pb_decode.h"
#include "pb_encode.h"
#include "pb_common.h"


typedef struct
{
    uint8_t *data;
    uint8_t length;
}BytesType;


#define pb_status_assert(x) if(!status){ log(ERR,"pb decoding failed: %s\r\n", PB_GET_ERROR(&requestStream));\
respone.response_code = ERROR_PARSE_FAILED;\
goto end;}


#define pb_admin_assert(x) if( config.read(CFG_BLE_INDEX , NULL) != x){\
log(ERR,"User is not admin ,admin index =%ld , get user index = %ld.\r\n",config.read(CFG_BLE_INDEX , NULL) ,x);\
respone.response_code = ERROR_NOT_ADMIN;\
goto end;}


uint8_t bluetooth_find_token(uint8_t *addr  , uint8_t *token);

uint8_t pb_encode_respone(pb_ostream_t *stream, const pb_field_t fields[], const void *src_struct);
void pb_add_bytes(BytesType *Bytes ,uint8_t *data , uint8_t length);
bool pb_decode_bytes( pb_callback_t *this , uint8_t *data );
bool pb_encode_bytes( pb_callback_t *this , BytesType *bytes);
bool bytes_decode(pb_istream_t *stream, const pb_field_t *field, void **arg);
bool bytes_encode(pb_ostream_t *stream, const pb_field_t *field, void * const *arg);
#endif


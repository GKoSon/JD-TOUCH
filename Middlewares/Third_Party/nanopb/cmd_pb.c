#include "cmd_pb.h"


bool bytes_encode(pb_ostream_t *stream, const pb_field_t *field, void * const *arg)
{
    BytesType *bytes = *arg;
    return pb_encode_tag_for_field(stream, field) &&
           pb_encode_string(stream, bytes->data, bytes->length);
}

bool bytes_decode(pb_istream_t *stream, const pb_field_t *field, void **arg)
{

    size_t len = stream->bytes_left;
    
    //if (len > sizeof(buf) - 1 || !pb_read(stream, *arg, len))
	if (!pb_read(stream, *arg, len))
        return false;
    
    return true;
}

bool pb_encode_bytes( pb_callback_t *this , BytesType *bytes)
{

    this->funcs.encode = bytes_encode;
    this->arg = bytes;
    
    return true;
}


bool pb_decode_bytes( pb_callback_t *this , uint8_t *data )
{
    this->funcs.decode = bytes_decode;
    this->arg = data;
    
    return true;
}

void pb_add_bytes(BytesType *Bytes ,uint8_t *data , uint8_t length)
{
    Bytes->data = data;
    Bytes->length = length;
}


uint8_t pb_encode_respone(pb_ostream_t *stream, const pb_field_t fields[], const void *src_struct)
{
    bool status =0;
    
    status = pb_encode(stream, fields, src_struct);

    if (!status)
    {
        log(ERR,"pb encoding failed: %s\n", PB_GET_ERROR(stream));
        return FALSE;
    }

    return TRUE;
}


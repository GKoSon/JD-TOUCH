#ifndef SN75176BDR_H_
#define SN75176BDR_H_

#include "stdint.h"
#include "rs485.h"

void sn75176_send( uint8_t *pData , uint16_t len);

extern rs485OpsType sn7517Ops;

#endif
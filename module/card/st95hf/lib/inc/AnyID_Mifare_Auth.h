#include "stdint.h"
#include "model.h"

#ifndef _ANYID_MIFARE_AUTH_
#define _ANYID_MIFARE_AUTH_

//#include "Types.h"

/* Reverse the bit order in the 8 bit value x */
#define rev8(x)            ((((x)>>7)&1)^((((x)>>6)&1)<<1)^((((x)>>5)&1)<<2)^((((x)>>4)&1)<<3)^((((x)>>3)&1)<<4)^((((x)>>2)&1)<<5)^((((x)>>1)&1)<<6)^(((x)&1)<<7))
/* Reverse the bit order in the 16 bit value x */
#define rev16(x)        (rev8 (x)^(rev8 (x>> 8)<< 8))
/* Reverse the bit order in the 32 bit value x */
#define rev32(x)        (rev16(x)^(rev16(x>>16)<<16))
/* Return the nth bit from x */
#define bit(x,n)        (((x)>>(n))&1)

/* Convert 4 array entries (a[0], a[1], a[2] and a[3]) into a 32 bit integer, where
 * a[0] is the MSByte and a[3] is the LSByte */
#define ARRAY_TO_UINT32(a) ( ((uint32_t)((a)[0]&0xff) << 24) | ((uint32_t)((a)[1]&0xff) << 16) | \
    ((uint32_t)((a)[2]&0xff) << 8) | ((uint32_t)((a)[3]&0xff) << 0) )
/* Copy an uint32_t i into 4 array entries (a[0], a[1], a[2] and a[3]), where a[0] is the MSByte */
#define UINT32_TO_ARRAY(i, a) do{ \
    (a)[0] = ((i)>>24) & 0xff; \
    (a)[1] = ((i)>>16) & 0xff; \
    (a)[2] = ((i)>> 8) & 0xff; \
    (a)[3] = ((i)>> 0) & 0xff; \
}while(0);

/* Calculate the odd parity bit for one byte of input */
#define ODD_PARITY(i) (( (i) ^ (i)>>1 ^ (i)>>2 ^ (i)>>3 ^ (i)>>4 ^ (i)>>5 ^ (i)>>6 ^ (i)>>7 ^ 1) & 0x01)

/* Like UINT32_TO_ARRAY, but put the correct parity in the 8th bit of each array entry (thus the
 * array entries must at least be 16 bit wide) */
#define UINT32_TO_ARRAY_WITH_PARITY(i, a) do{ \
    (a)[0] = ((i)>>24) & 0xff; (a)[0] |= ODD_PARITY((a)[0])<<8; \
    (a)[1] = ((i)>>16) & 0xff; (a)[1] |= ODD_PARITY((a)[1])<<8; \
    (a)[2] = ((i)>> 8) & 0xff; (a)[2] |= ODD_PARITY((a)[2])<<8; \
    (a)[3] = ((i)>> 0) & 0xff; (a)[3] |= ODD_PARITY((a)[3])<<8; \
}while(0);


/* == keystream generating filter function === */
/* This macro selects the four bits at offset a, b, c and d from the value x and returns the
 * concatenated bitstring  x_d || x_c || x_b || x_a  as an integer
 */
#define i4(x,a,b,c,d) ((uint32_t)( \
      (((x)>>(a)) & 1)<<0 \
    | (((x)>>(b)) & 1)<<1 \
    | (((x)>>(c)) & 1)<<2 \
    | (((x)>>(d)) & 1)<<3 \
))

uint32_t mf20(uint64_t x);
uint8_t MIFARE_update(uint8_t in, uint8_t feedback);
uint8_t MIFARE_update_byte(uint8_t in, uint8_t feedback);
uint32_t MIFARE_update_word(uint32_t in, uint8_t feedback);
uint32_t prng_next(uint8_t n);
//±àÂë»ò½âÂë
void Mifare_Cipher(uint16_t *pFrame, uint8_t byteLen, uint8_t bitLen);

//extern ISO14443A_MIFAREAUTH g_sISO14443AMifareAuth;

#endif
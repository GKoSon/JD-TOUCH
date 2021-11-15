#include "AnyID_Mifare_Auth.h"
#include "model.h"
//ISO14443A_MIFAREAUTH g_sISO14443AMifareAuth = {0};
/* These macros are linearized boolean tables for the output filter functions.
 * E.g. fa(0,1,0,1) is  (mf2_f4a >> 0x5)&1
 */
const uint32_t mf2_f4a = 0x9E98;
const uint32_t mf2_f4b = 0xB48E;
const uint32_t mf2_f5c = 0xEC57E80A;
extern ISO14443A_MIFAREAUTH MifareCard;
uint32_t mf20(uint64_t x)
{
    uint32_t d = 2;       /* number of cycles between when key stream is produced
                                 * and when key stream is used.
                                 * Irrelevant for software implmentations, but important
                                 * to consider in side-channel attacks */

    uint32_t i5 =
        ((mf2_f4b >> i4(x, 7 + d, 9 + d, 11 + d, 13 + d)) & 1) << 0 |
        ((mf2_f4a >> i4(x, 15 + d, 17 + d, 19 + d, 21 + d)) & 1) << 1 |
        ((mf2_f4a >> i4(x, 23 + d, 25 + d, 27 + d, 29 + d)) & 1) << 2 |
        ((mf2_f4b >> i4(x, 31 + d, 33 + d, 35 + d, 37 + d)) & 1) << 3 |
        ((mf2_f4a >> i4(x, 39 + d, 41 + d, 43 + d, 45 + d)) & 1) << 4;

    return (mf2_f5c >> i5) & 1;
}
#if 1

uint8_t MIFARE_update(uint8_t in, uint8_t feedback)
{
    //uint64_t x = nfc->TagVar->TagProtocol.ISO14443A_Card.MifareCard.lfsr;
    //uint8_t ks = mf20(nfc->TagVar->TagProtocol.ISO14443A_Card.MifareCard.lfsr);
    uint64_t x = MifareCard.lfsr;
    uint8_t ks = mf20(MifareCard.lfsr);

    //nfc->TagVar->TagProtocol.ISO14443A_Card.MifareCard.lfsr = (x >> 1) |
    MifareCard.lfsr = (x >> 1) |
                                    ((((x >> 0) ^ (x >> 5)
                                   ^ (x >> 9) ^ (x >> 10) ^ (x >> 12) ^ (x >> 14)
                                   ^ (x >> 15) ^ (x >> 17) ^ (x >> 19) ^ (x >> 24)
                                   ^ (x >> 25) ^ (x >> 27) ^ (x >> 29) ^ (x >> 35)
                                   ^ (x >> 39) ^ (x >> 41) ^ (x >> 42) ^ (x >> 43)
                                   ^ in ^ (feedback ? ks : 0)) & 1) << 47);

    return ks;
}


//uint8_t MIFARE_update_byte(uint8_t in, uint8_t feedback, PNfc_t nfc)
uint8_t MIFARE_update_byte(uint8_t in, uint8_t feedback)
{
    uint8_t ret = 0;
    int i;
    for(i = 0; i < 8; i++)
    {
        //ret |= MIFARE_update(bit(in, i), feedback, nfc) << i;
        ret |= MIFARE_update(bit(in, i), feedback) << i;
    }
    return ret;
}


//uint32_t MIFARE_update_word(uint32_t in, uint8_t feedback, PNfc_t nfc)
uint32_t MIFARE_update_word(uint32_t in, uint8_t feedback)
{
    uint32_t ret = 0;
    int i = 0;
    for(i = 3; i >= 0; i--)
    {
        //ret |= MIFARE_update_byte((in >> (i * 8)) & 0xff, feedback, nfc) << (i * 8);
        ret |= MIFARE_update_byte((in >> (i * 8)) & 0xff, feedback) << (i * 8);
    }
    return ret;
}

//uint32_t prng_next(uint8_t n, PNfc_t nfc)
uint32_t prng_next(uint8_t n)
{
    int i;
    //uint32_t rng = nfc->TagVar->TagProtocol.ISO14443A_Card.MifareCard.nt;
    uint32_t rng = MifareCard.nt;

    /* The register is stored and returned in reverse bit order, this way, even
     * if we cast the returned 32 bit value to a 16 bit value, the necessary
     * state will be retained. */
    rng = rev32(rng);
    for(i = 0; i < n; i++)
        rng =
            ((rng << 1) |
             (((rng >> 15) ^ (rng >> 13) ^ (rng >> 12) ^ (rng >> 10)) &
              1));
    return rev32(rng);
}

//void Mifare_Cipher(uint16_t *pFrame, uint8_t byteLen, uint8_t bitLen, PNfc_t nfc)
void Mifare_Cipher(uint16_t *pFrame, uint8_t byteLen, uint8_t bitLen)
{
    uint8_t i = 0;
    if(bitLen > 0)
    {
        byteLen--;
    }
    for(i = 0; i < byteLen; i++)
    {
        //pFrame[i] = pFrame[i] ^ MIFARE_update_byte(0, 0, nfc);
        //pFrame[i] = pFrame[i] ^ (mf20(nfc->TagVar->TagProtocol.ISO14443A_Card.MifareCard.lfsr) << 8);
        pFrame[i] = pFrame[i] ^ MIFARE_update_byte(0, 0);
        pFrame[i] = pFrame[i] ^ (mf20(MifareCard.lfsr) << 8);
    }
    for(i = 0; i < bitLen; i++)
    {
        //pFrame[byteLen] ^= MIFARE_update(0, 0, nfc) << i;
        pFrame[byteLen] ^= MIFARE_update(0, 0) << i;
    }
}
#endif

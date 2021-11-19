#ifndef ISO14443B_READ_H_
#define ISO14443B_READ_H_




   
 /* Sizes of ISO14443B variables */
#define ISO14443B_ATQB_SIZE                            13
#define ISO14443B_MAX_PUPI_SIZE                        0x04
#define ISO14443B_MAX_APPLI_SIZE                    0x04
#define ISO14443B_MAX_PROTOCOL_SIZE                    0x04
#define ISO14443B_MAX_LOG_MSG                        60

#define TR0_64_FS                                    0
#define TR0_32_FS                                    1<<6
#define TR0_16_FS                                    1<<7

#define TR1_64_FS                                    0
#define TR1_32_FS                                    1<<4
#define TR1_16_FS                                    1<<5

#define SOF_REQUIRED                                0
#define SOF_NOT_REQUIRED                             1<<2

#define EOF_REQUIRED                                0
#define EOF_NOT_REQUIRED                             1<<3


#define MAX_FRAME_SIZE_16_BYTES                      0
#define MAX_FRAME_SIZE_24_BYTES                         1
#define MAX_FRAME_SIZE_32_BYTES                         2
#define MAX_FRAME_SIZE_40_BYTES                         3
#define MAX_FRAME_SIZE_48_BYTES                         4
#define MAX_FRAME_SIZE_64_BYTES                         5
#define MAX_FRAME_SIZE_96_BYTES                         6
#define MAX_FRAME_SIZE_128_BYTES                     7
#define MAX_FRAME_SIZE_256_BYTES                     8

#define PCD_TO_PICC_106K                             0
#define PCD_TO_PICC_212K                             1<<4
#define PCD_TO_PICC_424K                             1<<5
#define PCD_TO_PICC_848K                             3<<4

#define PICC_TO_PCD_106K                             0
#define PICC_TO_PCD_212K                             1<<6
#define PICC_TO_PCD_424K                             1<<7
#define PICC_TO_PCD_848K                             3<<6


#define TR2_32_FS                                      0
#define TR2_128_FS                                    1<<1
#define TR2_256_FS                                    1<<2
#define TR2_512_FS                                    3<<1

#define PICC_COMPLIANT_ISO14443_4                       1
#define PICC_NOT_COMPLIANT_ISO14443_4                  0

#define CID_0                                          0
#define CID_1                                        1
#define CID_2                                        2
#define CID_3                                        3
#define CID_4                                        4
#define CID_5                                        5
#define CID_6                                        6
#define CID_7                                        7
#define CID_8                                        8
#define CID_9                                        9
#define CID_10                                        10
#define CID_11                                        11
#define CID_12                                        12
#define CID_13                                         13
#define CID_14                                        14


uint8_t st25Iso14443bReadUid( tagBufferType *tag );

#endif


#ifndef _SW_I2C_H_
#define _SW_I2C_H_

#include "stdint.h"
#include "stdbool.h"
#include "unit.h"

#define I2C_ACKNOWLEDGE        	FALSE
#define I2C_NON_ACKNOWLEDGE     TRUE

typedef enum _I2cIoTransType{
    I2C_TRANS_READ,
    I2C_TRANS_WRITE
}I2cIoTransType;

typedef struct
{
    uint8_t sdaPin;
    uint8_t sclPin;
	void    (*set_scl)		(void);
	void    (*clr_scl)		(void);
	uint8_t (*get_scl)      (void);
	void    (*set_sda)		(void);
	void    (*clr_sda)		(void);
	uint8_t (*get_sda)      (void);
	void    (*delay_bit)    (void);
    void    (*delay_byte)   (void);
	uint16_t	dummy;
}i2cPortType;


typedef struct	_bsp_i2c_ops
{
	void* (*open)			( char *name );
    void (*init)    		(i2cPortType *i2c );
    uint8_t (*access_start) (i2cPortType* i2c, uint8_t ucSlaveAdr, I2cIoTransType Trans);
    void    (*stop)         (i2cPortType *i2c);
    uint8_t (*send_byte)	(i2cPortType* i2c, uint8_t val);
    uint8_t (*receive_byte) (i2cPortType* i2c, BOOL ack);
}bsp_i2c_ops;

typedef struct
{
    char *name;
    void *i2c;
}device_i2c;


extern bsp_i2c_ops swI2c;

#endif

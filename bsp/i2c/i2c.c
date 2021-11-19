
#include "i2c.h"
#include "bsp_gpio.h"
#include "unit.h"
#include "string.h"


static void sw_i2c_set_scl_check(i2cPortType *i2c, uint8_t set)
{
    volatile uint16_t dummy;

    if (set== TRUE)
    {
        i2c->set_scl();
    }
    else
    {
        i2c->clr_scl();
    }

    if (set == TRUE)
    {
        dummy = i2c->dummy;
        while ((i2c->get_scl() == FALSE) && (dummy--)) ;
    }
}


static void sw_i2c_set_sda_check(i2cPortType *i2c, uint8_t set)
{
    volatile uint16_t dummy;

    if (set== TRUE)
    {
        i2c->set_sda();
    }
    else
    {
        i2c->clr_sda();
    }

    if (set == TRUE)
    {
        dummy = i2c->dummy;
        while ((i2c->get_sda() == FALSE) && (dummy--)) ;
    }

}


static BOOL sw_i2c_start(i2cPortType *i2c)
{
    volatile uint8_t status = TRUE;

    sw_i2c_set_sda_check(i2c, TRUE);

    i2c->delay_bit();

    sw_i2c_set_scl_check(i2c, TRUE);

    i2c->delay_bit();

    if ((i2c->get_sda() == FALSE) || (i2c->get_scl() == FALSE))
    {
        status = FALSE;
    }
    else
    {
        i2c->clr_sda();
        i2c->delay_bit();
        i2c->clr_scl();
        i2c->delay_bit();
    }
    return status;
}

void sw_i2c_stop(i2cPortType *i2c)
{
    i2c->clr_scl();
    i2c->delay_bit();
    i2c->clr_sda();
    i2c->delay_bit();
    sw_i2c_set_scl_check(i2c, TRUE);
    i2c->delay_bit();
    sw_i2c_set_sda_check(i2c, TRUE);
    i2c->delay_bit();
}



uint8_t sw_i2c_receive_byte(i2cPortType* i2c, BOOL ack)
{
    volatile uint8_t receive = 0;
    uint8_t mask = BIT7;
    while(mask)
    {
        i2c->set_sda();
        sw_i2c_set_scl_check(i2c, TRUE);
        i2c->delay_bit();
        if(i2c->get_sda() == TRUE)
            receive |= mask;
        i2c->clr_scl();
        i2c->delay_bit();
        mask >>= 1;
    }
    if (ack== TRUE)
    {
        sw_i2c_set_sda_check(i2c, I2C_ACKNOWLEDGE);
    }
    else{
        sw_i2c_set_sda_check(i2c, I2C_NON_ACKNOWLEDGE);
    }
    i2c->delay_bit();
    sw_i2c_set_scl_check(i2c, TRUE);
    i2c->delay_bit();
    i2c->clr_scl();
    i2c->delay_bit();
    return receive;
}

/*
void IIC_write_byte(unsigned char dat)
{
    unsigned char i;
    i2c->clr_scl();
    i2c->delay_bit();
    for(i=0;i<8;i++)
    {
        if((dat<<i)&0x80)
        {
            IIC_dat_set;
        }
        else
        {
            IIC_dat_clr;
        }
        i2c->delay_bit();
        IIC_clk_set;
        i2c->delay_bit();
        i2c->clr_scl();
        i2c->delay_bit();
    }
     IIC_dat_set;  
     i2c->delay_bit();
     IIC_clk_set;
     i2c->delay_bit();
     if(IIC_dat==1) ack=0;
     else ack=1;
     i2c->clr_scl();
     i2c->delay_bit();
}*/

uint8_t sw_i2c_send_byte(i2cPortType* i2c, uint8_t val)
{
    volatile uint8_t mark = BIT7;
    volatile uint8_t ack = I2C_NON_ACKNOWLEDGE;
    
    i2c->clr_scl();
    i2c->delay_bit();    
    while(mark)
    {
        if (val & mark)
            sw_i2c_set_sda_check(i2c, TRUE);
            //i2c->set_sda();
        else
            sw_i2c_set_sda_check(i2c, FALSE);
            //i2c->clr_sda();
        
        i2c->delay_bit();
        sw_i2c_set_scl_check(i2c, TRUE);
        //i2c->set_scl();
        i2c->delay_bit();
        i2c->clr_scl();
        i2c->delay_bit();
        mark >>= 1;
    }
    // recieve acknowledge
    i2c->set_sda();
    i2c->delay_bit();
    sw_i2c_set_scl_check(i2c, TRUE);
    i2c->delay_bit();
    ack = i2c->get_sda();
    i2c->clr_scl();
    i2c->delay_bit();
    return (ack);
}

uint8_t sw_i2c_access_start(i2cPortType* i2c, uint8_t ucSlaveAdr, I2cIoTransType Trans)
{
    volatile uint8_t dummy;
    
    if (Trans == I2C_TRANS_READ)
    {
        ucSlaveAdr |= BIT0;
    }
    else{
        ucSlaveAdr &= ~BIT0;
    }
    dummy = i2c->dummy;
    while (dummy--)
    {
        if (sw_i2c_start(i2c) == FALSE)
        {
            continue;
        }
        if (sw_i2c_send_byte(i2c, ucSlaveAdr) == I2C_ACKNOWLEDGE)
        { // check acknowledge            
            
            return TRUE;
        }
        i2c->delay_byte();
        sw_i2c_stop(i2c);
    }
    return FALSE;
}

void i2c_init( i2cPortType *i2c )
{
     pin_ops.pin_mode(i2c->sclPin , PIN_MODE_OUTPUT_PULLUP);
     pin_ops.pin_mode(i2c->sdaPin , PIN_MODE_OUTPUT_PULLUP);
}

void sw_i2c_burst_read_bytes
(i2cPortType* i2c,uint8_t ucSlaveAdr,uint8_t ucSubAdr,uint8_t *pucBuff,uint16_t ucBufLen)
{
    volatile uint8_t dummy;
    dummy = i2c->dummy;
    while(dummy--)
    {
        if (sw_i2c_access_start(i2c, ucSlaveAdr, I2C_TRANS_WRITE) == FALSE)
        {
            continue;
        }
        if (sw_i2c_send_byte(i2c, ucSubAdr) == I2C_NON_ACKNOWLEDGE)
        {
            continue;
        }
        if (sw_i2c_access_start(i2c, ucSlaveAdr, I2C_TRANS_READ) == FALSE)
        {
            continue;
        }
        while(ucBufLen--)
        {
            *pucBuff = sw_i2c_receive_byte(i2c, (ucBufLen>0)?TRUE:FALSE);
            pucBuff++;
        }
        break;
    }
    sw_i2c_stop(i2c);
}

void sw_i2c_burst_write_bytes(
i2cPortType* i2c,uint8_t ucSlaveAdr,uint8_t ucSubAdr,uint8_t *pucBuff,uint8_t ucBufLen)
{
    volatile uint8_t dummy;
    dummy = i2c->dummy;
    while(dummy--){
        if (sw_i2c_access_start(i2c, ucSlaveAdr, I2C_TRANS_WRITE) == FALSE)
        {
            continue;
        }
        if (sw_i2c_send_byte(i2c, ucSubAdr) == I2C_NON_ACKNOWLEDGE)
        {
            continue;
        }
        while(ucBufLen--)
        {
            sw_i2c_send_byte(i2c, *pucBuff);
            pucBuff++;
        }
        break;
    }
    sw_i2c_stop(i2c);
}

uint8_t sw_i2c_read_byte(i2cPortType* i2c,uint8_t ucSlaveAdr,uint8_t ucSubAdr)
{
    volatile uint8_t ucBuf;
    sw_i2c_burst_read_bytes(i2c, ucSlaveAdr, ucSubAdr, (uint8_t*)&ucBuf, 1);
    return ucBuf;
}

void sw_i2c_write_byte(i2cPortType* i2c,uint8_t ucSlaveAdr,uint8_t ucSubAdr,uint8_t ucVal)
{
    sw_i2c_burst_write_bytes(i2c, ucSlaveAdr, ucSubAdr, &ucVal, 1);
}


void i2c_delay_us(uint32_t us)
{
    while (us--)
    {
        __asm("NOP");__asm("NOP");__asm("NOP");__asm("NOP");
        __asm("NOP");__asm("NOP");__asm("NOP");__asm("NOP");
        __asm("NOP");__asm("NOP");__asm("NOP");__asm("NOP");
        __asm("NOP");__asm("NOP");__asm("NOP");__asm("NOP");
        __asm("NOP");__asm("NOP");__asm("NOP");__asm("NOP");
        __asm("NOP");__asm("NOP");__asm("NOP");__asm("NOP");
        __asm("NOP");__asm("NOP");__asm("NOP");__asm("NOP");
        __asm("NOP");__asm("NOP");__asm("NOP");__asm("NOP");
        __asm("NOP");__asm("NOP");__asm("NOP");__asm("NOP");
        __asm("NOP");__asm("NOP");__asm("NOP");__asm("NOP");
        __asm("NOP");__asm("NOP");__asm("NOP");__asm("NOP");
        __asm("NOP");__asm("NOP");__asm("NOP");__asm("NOP");
    }
}


    


void i2c2_delay_bit(void)
{
      i2c_delay_us(50);
}
void i2c2_delay_byte(void)
{
      i2c_delay_us(100);
}

void i2c2_set_scl(void)
{
    pin_ops.pin_write(I2C2_SCL_PIN , PIN_HIGH);
}
void i2c2_clr_scl(void)
{
    pin_ops.pin_write(I2C2_SCL_PIN , PIN_LOW);
}
void i2c2_set_sda(void)
{
    pin_ops.pin_write(I2C2_SDA_PIN , PIN_HIGH);
}
void i2c2_clr_sda(void)
{
    pin_ops.pin_write(I2C2_SDA_PIN , PIN_LOW);
}

uint8_t i2c2_get_sda(void)
{
    uint8_t Ret;
    
    pin_ops.set_mode(I2C2_SDA_PIN , PIN_MODE_INPUT_PULLUP);
    if (pin_ops.pin_read(I2C2_SDA_PIN) == PIN_HIGH)
    {
        Ret= TRUE;
    }
    else
    {
        Ret= FALSE;
    }
    pin_ops.set_mode(I2C2_SDA_PIN , PIN_MODE_OUTPUT_PULLUP);
    return Ret;
}
uint8_t i2c2_get_scl(void)
{
    uint8_t Ret;
    pin_ops.set_mode(I2C2_SCL_PIN , PIN_MODE_INPUT_PULLUP);    
    if (pin_ops.pin_read(I2C2_SCL_PIN)== PIN_HIGH)
    {
        Ret= TRUE;
    }
    else
    {
        Ret= FALSE;
    }
    pin_ops.set_mode(I2C2_SCL_PIN , PIN_MODE_OUTPUT_PULLUP);
    return Ret;
}





i2cPortType i2c2=
{
    .sdaPin = I2C2_SDA_PIN,
    .sclPin = I2C2_SCL_PIN,
    .set_scl = i2c2_set_scl,
    .clr_scl = i2c2_clr_scl,
    .get_scl = i2c2_get_scl,
    .set_sda = i2c2_set_sda,
    .clr_sda = i2c2_clr_sda,
    .get_sda = i2c2_get_sda,
    .delay_bit = i2c2_delay_bit,
    .delay_byte = i2c2_delay_byte,
    .dummy = 50,
};

device_i2c   i2c_t[]=
{
    {"i2c1" , NULL},
    {"i2c2" , &i2c2},
    {"i2c3" , NULL},
};

void *i2c_open( char *name )
{
    for( uint8_t i = 0 ; i < sizeof (i2c_t) / sizeof (i2c_t[0]);  i++)
    {
        if( strcmp(name , i2c_t[i].name) == 0 )
        {
            if(i2c_t[i].i2c != NULL)
            {
                return i2c_t[i].i2c;
            }
            else
            {
                log_err("%s没有初始化，请添加BSP驱动\n" , name);
                return NULL;
            }
        }
    }
    
    return NULL;
}


bsp_i2c_ops swI2c=
{
    .open = i2c_open,
    .init = i2c_init,
    .access_start = sw_i2c_access_start,
    .stop = sw_i2c_stop,
    .send_byte = sw_i2c_send_byte,
    .receive_byte = sw_i2c_receive_byte,
};






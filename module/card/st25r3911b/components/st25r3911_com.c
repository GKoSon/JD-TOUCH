
/******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT 2016 STMicroelectronics</center></h2>
  *
  * Licensed under ST MYLIBERTY SOFTWARE LICENSE AGREEMENT (the "License");
  * You may not use this file except in compliance with the License.
  * You may obtain a copy of the License at:
  *
  *        http://www.st.com/myliberty
  *
  * Unless required by applicable law or agreed to in writing, software 
  * distributed under the License is distributed on an "AS IS" BASIS, 
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied,
  * AND SPECIFICALLY DISCLAIMING THE IMPLIED WARRANTIES OF MERCHANTABILITY,
  * FITNESS FOR A PARTICULAR PURPOSE, AND NON-INFRINGEMENT.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  *
******************************************************************************/


/*
 *      PROJECT:   ST25R3911 firmware
 *      $Revision: $
 *      LANGUAGE:  ISO C99
 */

/*! \file
 *
 *  \author Ulrich Herrmann
 *
 *  \brief Implementation of ST25R3911 communication.
 *
 */

/*
******************************************************************************
* INCLUDES
******************************************************************************
*/
#include "st25r3911_com.h"
#include "st_errno.h"
#include "platform.h"
#include "st25r3911.h"
#include "utils.h"


/*
******************************************************************************
* LOCAL DEFINES
******************************************************************************
*/

#define ST25R3911_SPI_SELECT()   platformGpioClear( ST25R3911_SS_PORT, ST25R3911_SS_PIN ) /*!< ST25R3911 HAL macro to do a SPI chip select  ST25R3911_SS_PIN = 0 */
#define ST25R3911_SPI_DESELECT() platformGpioSet( ST25R3911_SS_PORT, ST25R3911_SS_PIN )   /*!< ST25R3911 HAL macro to do a SPI chip deselect ST25R3911_SS_PIN = 1*/

#define ST25R3911_WRITE_MODE  (0)                           /*!< ST25R3911 SPI Operation Mode: Write                            */
#define ST25R3911_READ_MODE   (1 << 6)                      /*!< ST25R3911 SPI Operation Mode: Read                             */
#define ST25R3911_FIFO_LOAD   (2 << 6)                      /*!< ST25R3911 SPI Operation Mode: FIFO Load                        */
#define ST25R3911_FIFO_READ   (0xbf)                        /*!< ST25R3911 SPI Operation Mode: FIFO Read                        */
#define ST25R3911_CMD_MODE    (3 << 6)                      /*!< ST25R3911 SPI Operation Mode: Direct Command                   */

/*
******************************************************************************
* LOCAL FUNCTION PROTOTYPES
******************************************************************************
*/
static void st25r3911Write(uint8_t cmd, const uint8_t* values, uint8_t length);

static inline void st25r3911CheckFieldSetLED(uint8_t val)
{
    if (ST25R3911_REG_OP_CONTROL_tx_en & val)
    {
#ifdef PLATFORM_LED_FIELD_PIN
        platformLedOn( PLATFORM_LED_FIELD_PORT, PLATFORM_LED_FIELD_PIN );
    }
    else
    {
        platformLedOff( PLATFORM_LED_FIELD_PORT, PLATFORM_LED_FIELD_PIN );
#endif /* PLATFORM_LED_FIELD_PIN */
    }
}


/*
******************************************************************************
* GLOBAL FUNCTIONS
******************************************************************************
*/
void st25r3911ReadRegister(uint8_t reg, uint8_t* val)
{
    uint8_t buf[2];

    buf[0] = reg | ST25R3911_READ_MODE;
    buf[1] = 0;

    platformIrqST25R3911Disable();

    ST25R3911_SPI_SELECT();
    platformSpiTxRx(buf, buf, 2);
    ST25R3911_SPI_DESELECT();
    *val = buf[1];

    platformIrqST25R3911Enable();

    return;
}

void st25r3911ReadMultipleRegisters(uint8_t reg, uint8_t* val, uint8_t length)
{
    uint8_t firstbyte = reg | ST25R3911_READ_MODE;

    /* make this operation atomic */
    platformIrqST25R3911Disable();
    ST25R3911_SPI_SELECT();

    /* Since the result comes one byte later, let's first transmit the 
       adddress with discarding the result */
    platformSpiTxRx(&firstbyte, &firstbyte, 1);
    platformSpiTxRx(NULL, val, length);

    ST25R3911_SPI_DESELECT();
    platformIrqST25R3911Enable();
    return;
}

void st25r3911ReadTestRegister(uint8_t reg, uint8_t* val)
{
    uint8_t buf[3];

    buf[0] = ST25R3911_CMD_TEST_ACCESS;
    buf[1] = reg | ST25R3911_READ_MODE;
    buf[2] = 0x00;

    platformIrqST25R3911Disable();
    ST25R3911_SPI_SELECT();
    platformSpiTxRx(buf, buf, 3);
    ST25R3911_SPI_DESELECT();
    platformIrqST25R3911Enable();
    
    *val = buf[2];

    return;
}

void st25r3911WriteTestRegister(uint8_t reg, uint8_t val)
{
    uint8_t buf[3];

    buf[0] = ST25R3911_CMD_TEST_ACCESS;
    buf[1] = reg | ST25R3911_WRITE_MODE;
    buf[2] = val;

    platformIrqST25R3911Disable();
    ST25R3911_SPI_SELECT();
    platformSpiTxRx(buf, NULL, 3);
    ST25R3911_SPI_DESELECT();
    platformIrqST25R3911Enable();

    return;
}

void st25r3911WriteRegister(uint8_t reg, uint8_t val)
{
    uint8_t buf[2];
    
    if (ST25R3911_REG_OP_CONTROL == reg)
    {
        st25r3911CheckFieldSetLED(val);
    }

    buf[0] = reg | ST25R3911_WRITE_MODE;
    buf[1] = val;

    platformIrqST25R3911Disable();
    ST25R3911_SPI_SELECT();
    platformSpiTxRx(buf, NULL, 2);
    ST25R3911_SPI_DESELECT();
    platformIrqST25R3911Enable();

    return;
}

void st25r3911ClrRegisterBits( uint8_t reg, uint8_t clr_mask )
{
    uint8_t tmp;

    /* make this operation atomic */
    platformIrqST25R3911Disable();

    st25r3911ReadRegister(reg, &tmp);
    tmp &= ~clr_mask;
    st25r3911WriteRegister(reg, tmp);
    
    platformIrqST25R3911Enable();
    return;
}


void st25r3911SetRegisterBits( uint8_t reg, uint8_t set_mask )
{
    uint8_t tmp;

    /* make this operation atomic */
    platformIrqST25R3911Disable();

    st25r3911ReadRegister(reg, &tmp);
    tmp |= set_mask;
    st25r3911WriteRegister(reg, tmp);
    
    platformIrqST25R3911Enable();
    return;
}

void st25r3911ChangeRegisterBits(uint8_t reg, uint8_t valueMask, uint8_t value)
{
    st25r3911ModifyRegister(reg, valueMask, (valueMask & value) );
}

void st25r3911ModifyRegister(uint8_t reg, uint8_t clr_mask, uint8_t set_mask)
{
    uint8_t tmp;

    /* make this operation atomic */
    platformIrqST25R3911Disable();

    st25r3911ReadRegister(reg, &tmp);

    /* mask out the bits we don't want to change */
    tmp &= ~clr_mask;
    /* set the new value */
    tmp |= set_mask;
    st25r3911WriteRegister(reg, tmp);
    
    platformIrqST25R3911Enable();

    return;
}

void st25r3911WriteMultipleRegisters(uint8_t reg, const uint8_t* values, uint8_t length)
{
    reg |= ST25R3911_WRITE_MODE;

    if (reg <= ST25R3911_REG_OP_CONTROL && reg+length >= ST25R3911_REG_OP_CONTROL)
    {
        st25r3911CheckFieldSetLED(values[ST25R3911_REG_OP_CONTROL-reg]);
    }
    
    st25r3911Write(reg, values, length);
}


void st25r3911WriteFifo(const uint8_t* values, uint8_t length)
{
    uint8_t cmd = ST25R3911_FIFO_LOAD;

    /* make this operation atomic */
    platformIrqST25R3911Disable();
    {
        ST25R3911_SPI_SELECT();
        {
            platformSpiTxRx(&cmd, NULL, 1);
            platformSpiTxRx(values, NULL, length);  
        }
        ST25R3911_SPI_DESELECT();
    }
    platformIrqST25R3911Enable();

    return;
}

void st25r3911ReadFifo(uint8_t* buf, uint8_t length)
{
    uint8_t cmd = ST25R3911_FIFO_READ;

    if (length > 0)
    {
        /* make this operation atomic */
        platformIrqST25R3911Disable();
        ST25R3911_SPI_SELECT();
        
        platformSpiTxRx(&cmd, NULL, 1);
        platformSpiTxRx(NULL, buf, length);
            
        ST25R3911_SPI_DESELECT();
        platformIrqST25R3911Enable();
    }

    return;
}

void st25r3911ExecuteCommand(uint8_t cmd)
{
#ifdef PLATFORM_LED_FIELD_PIN
    if ( cmd >= ST25R3911_CMD_TRANSMIT_WITH_CRC && cmd <= ST25R3911_CMD_RESPONSE_RF_COLLISION_0)
    {
        platformLedOff(PLATFORM_LED_FIELD_PORT, PLATFORM_LED_FIELD_PIN);
    }
#endif /* PLATFORM_LED_FIELD_PIN */
    
    cmd |= ST25R3911_CMD_MODE;

    platformIrqST25R3911Disable();
    ST25R3911_SPI_SELECT();
    
    platformSpiTxRx(&cmd, NULL, 1);
    
    ST25R3911_SPI_DESELECT();
    platformIrqST25R3911Enable();

    return;
}


void st25r3911ExecuteCommands(uint8_t *cmds, uint8_t length)
{
    platformIrqST25R3911Disable();
    ST25R3911_SPI_SELECT();
    
    platformSpiTxRx( cmds, NULL, length );
    
    ST25R3911_SPI_DESELECT();
    platformIrqST25R3911Enable();

    return;
}

bool st25r3911IsRegValid( uint8_t reg )
{
    if( !(( (int8_t)reg >= ST25R3911_REG_IO_CONF1) && (reg <= ST25R3911_REG_CAPACITANCE_MEASURE_RESULT)) && 
        (reg != ST25R3911_REG_IC_IDENTITY)                                                         )
    {
        return false;
    }
    return true;
}

/*
******************************************************************************
* LOCAL FUNCTIONS
******************************************************************************
*/
static void st25r3911Write(uint8_t cmd, const uint8_t* values, uint8_t length)
{

    /* make this operation atomic */
    platformIrqST25R3911Disable();
    ST25R3911_SPI_SELECT();
    
    platformSpiTxRx(&cmd, NULL, 1);
    platformSpiTxRx(values, NULL, length);
    
    ST25R3911_SPI_DESELECT();
    platformIrqST25R3911Enable();

    return;
}

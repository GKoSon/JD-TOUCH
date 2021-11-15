
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
 *  \brief ST25R3911 high level interface
 *
 */

/*
******************************************************************************
* INCLUDES
******************************************************************************
*/

#include "st25r3911.h"
#include "st25r3911_com.h"
#include "st25r3911_interrupt.h"
#include "utils.h"
#include "timer.h"
#include "stdio.h"
#include <limits.h>

/*
******************************************************************************
* LOCAL DEFINES
******************************************************************************
*/

#define ST25R3911_OSC_STABLE_TIMEOUT           10 /*!< Timeout for Oscillator to get stable, datasheet: 700us, take 5 ms */
#define ST25R3911_CA_TIMEOUT                   10 /*!< Timeout for Collision Avoidance command                           */

/*
******************************************************************************
* LOCAL VARIABLES
******************************************************************************
*/
static bool st25r3911ReceivePrepared; /*!< flag indicating whether receive
                                       has been prepared */
static uint32_t st25r3911NoResponseTime_64fcs;

/*
******************************************************************************
* LOCAL FUNCTION PROTOTYPES
******************************************************************************
*/
static ReturnCode st25r3911DecodeErrorInterrupts(uint32_t mask)
{
    mask &= ST25R3911_IRQ_MASK_ERR1 | ST25R3911_IRQ_MASK_PAR | ST25R3911_IRQ_MASK_CRC;

    if (mask == 0) return ERR_NONE;

    if (mask & ST25R3911_IRQ_MASK_CRC) 
    {
        return ERR_CRC;
    }
    if (mask & ST25R3911_IRQ_MASK_PAR) 
    {
        return ERR_PAR;
    }
    if (mask & ST25R3911_IRQ_MASK_ERR1) 
    {
        return ERR_FRAMING;
    }
    
    return ERR_REQUEST;
}
static ReturnCode st25r3911ExecuteCommandAndGetResult(uint8_t cmd, uint8_t resreg, uint8_t sleeptime, uint8_t* result);

/*
******************************************************************************
* GLOBAL FUNCTIONS
******************************************************************************
*/

void st25r3911TxRxOn( void )
{
    st25r3911SetRegisterBits(ST25R3911_REG_OP_CONTROL, (ST25R3911_REG_OP_CONTROL_rx_en | ST25R3911_REG_OP_CONTROL_tx_en) );
}

void st25r3911TxRxOff( void )
{
    st25r3911ClrRegisterBits(ST25R3911_REG_OP_CONTROL, (ST25R3911_REG_OP_CONTROL_rx_en | ST25R3911_REG_OP_CONTROL_tx_en) );
}


void st25r3911OscOn( void )
{
    uint8_t reg = 0;
    
    // Make sure that oscillator is turned on and stable
    st25r3911ReadRegister( ST25R3911_REG_OP_CONTROL, &reg );
    
    // Use ST25R3911_REG_OP_CONTROL_en instead of ST25R3911_REG_AUX_DISPLAY_osc_ok to be on the safe side
    if (!(ST25R3911_REG_OP_CONTROL_en & reg))
    {
        /* enable oscillator frequency stable interrupt */
        st25r3911EnableInterrupts(ST25R3911_IRQ_MASK_OSC);

        /* enable oscillator and regulator output */
        st25r3911ModifyRegister(ST25R3911_REG_OP_CONTROL, 0x00, ST25R3911_REG_OP_CONTROL_en);

        /* wait for the oscillator interrupt */
        st25r3911WaitForInterruptsTimed(ST25R3911_IRQ_MASK_OSC, ST25R3911_OSC_STABLE_TIMEOUT);
        st25r3911DisableInterrupts(ST25R3911_IRQ_MASK_OSC);
    }
    
} // st25r3911OscOn()


void st25r3911Initialize()
{
    uint16_t vdd_mV;
    //uint8_t reg;
    
    st25r3911ReceivePrepared = false;

    /* first, reset the st25r3911 */
    st25r3911ExecuteCommand(ST25R3911_CMD_SET_DEFAULT);

        
    /* enable pull downs on miso line */
    st25r3911ModifyRegister(ST25R3911_REG_IO_CONF2, 0, 
            ST25R3911_REG_IO_CONF2_miso_pd1 |
            ST25R3911_REG_IO_CONF2_miso_pd2);

    /* after reset all interrupts are enabled. so disable them at first */
    st25r3911DisableInterrupts(ST25R3911_IRQ_MASK_ALL);
    /* and clear them, just to be sure... */
    st25r3911ClearInterrupts();

    /* trim settings for VHBR board, will anyway changed later on */
    st25r3911WriteRegister(ST25R3911_REG_ANT_CAL_TARGET, 0x80);
    
    st25r3911OscOn();
    
    /* Measure vdd and set sup3V bit accordingly */
    vdd_mV = st25r3911MeasureVoltage(ST25R3911_REG_REGULATOR_CONTROL_mpsv_vdd);

    st25r3911ModifyRegister(ST25R3911_REG_IO_CONF2,
                         ST25R3911_REG_IO_CONF2_sup3V,
                         (vdd_mV < 3600)?ST25R3911_REG_IO_CONF2_sup3V:0);

    /* Make sure Transmitter and Receiver are disabled */
    st25r3911TxRxOff();
    
    return;
}

void st25r3911Deinitialize()
{
    st25r3911DisableInterrupts(ST25R3911_IRQ_MASK_ALL);    

    // Disabe Tx and Rx, Keep OSC
    st25r3911TxRxOff();

    return;
}

ReturnCode st25r3911AdjustRegulators(uint16_t* result_mV)
{
    uint8_t result;
    uint8_t io_conf2;
    ReturnCode err = ERR_NONE;

    /* first check the status of the reg_s bit in ST25R3911_REG_VREG_DEF register.
       if this bit is set adjusting the regulators is not allowed */
    st25r3911ReadRegister(ST25R3911_REG_REGULATOR_CONTROL, &result);

    if (result & ST25R3911_REG_REGULATOR_CONTROL_reg_s)
    {
        return ERR_REQUEST;
    }

    st25r3911ExecuteCommandAndGetResult(ST25R3911_CMD_ADJUST_REGULATORS,
                                    ST25R3911_REG_REGULATOR_RESULT,
                                    5,
                                    &result);
    st25r3911ReadRegister(ST25R3911_REG_IO_CONF2, &io_conf2);

    result >>= ST25R3911_REG_REGULATOR_RESULT_shift_reg;
    result -= 5;
    if (result_mV)
    {
        if(io_conf2 & ST25R3911_REG_IO_CONF2_sup3V)
        {
            *result_mV = 2400;
            *result_mV += result * 100;
        }
        else
        {
            *result_mV = 3900;
            *result_mV += result * 120;
        }
    }
    return err;
}

void st25r3911MeasureRF(uint8_t* result)
{
    st25r3911ExecuteCommandAndGetResult(ST25R3911_CMD_MEASURE_AMPLITUDE,
                                    ST25R3911_REG_AD_RESULT,
                                    10,
                                    result);
}

void st25r3911MeasureCapacitance(uint8_t* result)
{
    st25r3911ExecuteCommandAndGetResult(ST25R3911_CMD_MEASURE_CAPACITANCE, 
                                    ST25R3911_REG_AD_RESULT,
                                    10,
                                    result);
}

void st25r3911MeasureAntennaResonance(uint8_t* result)
{
    st25r3911ExecuteCommandAndGetResult(ST25R3911_CMD_MEASURE_PHASE,
                                    ST25R3911_REG_AD_RESULT,
                                    10,
                                    result);
}

void st25r3911CalibrateAntenna(uint8_t* result)
{
    st25r3911ExecuteCommandAndGetResult(ST25R3911_CMD_CALIBRATE_ANTENNA,
                                    ST25R3911_REG_ANT_CAL_RESULT,
                                    10,
                                    result);
}

void st25r3911CalibrateModulationDepth(uint8_t* result)
{
    st25r3911ExecuteCommandAndGetResult(ST25R3911_CMD_CALIBRATE_MODULATION,
                                    ST25R3911_REG_AM_MOD_DEPTH_RESULT,
                                    10,
                                    result);
}


void st25r3911CalibrateCapacitiveSensor(uint8_t* result)
{
  st25r3911ExecuteCommandAndGetResult(ST25R3911_CMD_CALIBRATE_C_SENSOR,
                                    ST25R3911_REG_CAP_SENSOR_RESULT,
                                    10,
                                    result);
}


void st25r3911PrepareReceive(bool reset)
{

    if (reset)
    {
        /*******************************************************************************/
        /* REMARK: Silicon workaround ST25R3911 Errata #3                                 */
        /* Fifo problems: always have crc in FIFO                                      */
        st25r3911ModifyRegister(ST25R3911_REG_AUX, ST25R3911_REG_AUX_crc_2_fifo, ST25R3911_REG_AUX_crc_2_fifo);
        /*******************************************************************************/
        
        /* reset receive logic */
        st25r3911ExecuteCommand(ST25R3911_CMD_CLEAR_FIFO);
        /* clear interrupts */
        st25r3911GetInterrupt(ST25R3911_IRQ_MASK_RXS | ST25R3911_IRQ_MASK_RXE  |
                           ST25R3911_IRQ_MASK_PAR  | ST25R3911_IRQ_MASK_CRC  |
                           ST25R3911_IRQ_MASK_ERR1 | ST25R3911_IRQ_MASK_FWL |
                           ST25R3911_IRQ_MASK_NRE);
    }
    st25r3911EnableInterrupts(ST25R3911_IRQ_MASK_RXS | ST25R3911_IRQ_MASK_RXE  |
                                 ST25R3911_IRQ_MASK_PAR  | ST25R3911_IRQ_MASK_CRC  |
                                 ST25R3911_IRQ_MASK_ERR1 | ST25R3911_IRQ_MASK_FWL |
                                 ST25R3911_IRQ_MASK_NRE);

    st25r3911ReceivePrepared = true;
    return;
}

ReturnCode st25r3911RxNBytes(uint8_t* buf, uint16_t maxlength, uint16_t* length, uint16_t timeout_ms)
{
    ReturnCode err = ERR_NONE;
    uint8_t bytesToRead;
    uint32_t mask;

    *length = 0;

    /* check if st25r3911PrepareReceive was called before */
    if (!st25r3911ReceivePrepared)
    {
        return ERR_REQUEST;
    }

    if (0 == timeout_ms) timeout_ms = (st25r3911GetNoResponseTime_64fcs() * 5 + 999) / 1000;

#if ST25R3911_TXRX_ON_CSX
    st25r3911WriteTestRegister(0x1,0x04); /* digital demodulation on pins CSI and CSO(AM) */
#endif


    /* wait for start of receive or no response interrupt */
    mask = st25r3911WaitForInterruptsTimed(ST25R3911_IRQ_MASK_RXS | ST25R3911_IRQ_MASK_RXE | ST25R3911_IRQ_MASK_NRE , timeout_ms); 

    if (mask & (ST25R3911_IRQ_MASK_NRE))
    {
        err = ERR_TIMEOUT;
        goto out;
    }

    
    if (!(mask & ST25R3911_IRQ_MASK_RXS) && (mask & ST25R3911_IRQ_MASK_RXE))
    { /* rx end but not rx start interrupt */
        err = st25r3911DecodeErrorInterrupts(st25r3911GetInterrupt(ST25R3911_IRQ_MASK_ALL));
        EVAL_ERR_NE_GOTO(ERR_NONE, err, out);
    }
    
    if (!mask)
    {
        err = ERR_TIMEOUT;
        goto out;
    }

    do
    {
        /* wait either for FIFO waterlevel or end of receive interrupt */
        mask = st25r3911WaitForInterruptsTimed(ST25R3911_IRQ_MASK_RXE |
                                            ST25R3911_IRQ_MASK_FWL |
                                            ST25R3911_IRQ_MASK_CRC |
                                            ST25R3911_IRQ_MASK_PAR |
                                            ST25R3911_IRQ_MASK_ERR1|
                                            0
                                            , 10);

        st25r3911ReadRegister(ST25R3911_REG_FIFO_RX_STATUS1, &bytesToRead);
        
        /*******************************************************************************/
        /* REMARK: Silicon workaround ST25R3911 Errata #TBD                               */
        /* Reading it two times to workaround clock syncronisation bug in ST25R3911       */
        /* This bug was fixed in silicon v2.0 and newer. Retest and remove             */
        st25r3911ReadRegister(ST25R3911_REG_FIFO_RX_STATUS1, &bytesToRead);
        /*******************************************************************************/        

        if ((*length + bytesToRead) > maxlength)
        {
            st25r3911ReadFifo(buf, maxlength - *length);
            *length = maxlength;
            break;
        }

        *length += bytesToRead;
        st25r3911ReadFifo(buf, bytesToRead);
        buf += bytesToRead;

        /* loop as long as rx is in progress */
    } while (mask & ST25R3911_IRQ_MASK_FWL);

    if(bytesToRead > ST25R3911_FIFO_DEPTH)
    {
        err = ERR_FIFO;
        goto out;
    }

    err = st25r3911DecodeErrorInterrupts(mask);

out:
    /* FIXME: perform the delay */
    //delayNMicroSeconds(500);
    st25r3911DisableInterrupts(ST25R3911_IRQ_MASK_RXS |
                        ST25R3911_IRQ_MASK_RXE |
                        ST25R3911_IRQ_MASK_FWL |
                        ST25R3911_IRQ_MASK_PAR |
                        ST25R3911_IRQ_MASK_CRC |
                        ST25R3911_IRQ_MASK_NRE |
                        ST25R3911_IRQ_MASK_ERR1);
    st25r3911ReceivePrepared = false;
    st25r3911ExecuteCommand(ST25R3911_CMD_MASK_RECEIVE_DATA);
#if ST25R3911_TXRX_ON_CSX
    st25r3911WriteTestRegister(0x1,0x00); /* default value */
#endif
    return err;
}

ReturnCode st25r3911TxNBytes(const uint8_t* frame, uint16_t numbytes, uint8_t numbits, st25r3911TxFlag_t flags, uint16_t fdt )
{
    uint16_t txbytes;
    uint32_t mask;
    ReturnCode err = ERR_NONE;
    uint8_t antcl = (flags & ST25R3911_TX_FLAG_ANTCL) ? ST25R3911_REG_ISO14443A_NFC_antcl : 0;
    uint8_t reg, fifo_refill;
    /* calculate total num of bits, use variable numbytes since it is 16-bit */
    numbytes = numbytes * 8 + numbits;

    /* Sometimes the FIFO is not in a proper state e.g. at HLTA */
    st25r3911ExecuteCommand(ST25R3911_CMD_CLEAR_FIFO);

    st25r3911ReadRegister(ST25R3911_REG_IO_CONF1,&reg);
    fifo_refill = (reg & ST25R3911_REG_IO_CONF1_fifo_lt)?80:64;

    /* enable FIFO water level and end of transmit interrupt */
    st25r3911EnableInterrupts(ST25R3911_IRQ_MASK_FWL | ST25R3911_IRQ_MASK_TXE);

    /* Set antcl bit for iso14443a anti-collison: proper timings and proper parity extraction */
    st25r3911ModifyRegister(ST25R3911_REG_ISO14443A_NFC, ST25R3911_REG_ISO14443A_NFC_antcl, antcl);

    /* write number of bytes to be transmitted */
    st25r3911WriteRegister(ST25R3911_REG_NUM_TX_BYTES2, numbytes & 0xff);

    st25r3911WriteRegister(ST25R3911_REG_NUM_TX_BYTES1, (uint8_t)(numbytes >> 8) & 0xff);    

    
    /* Configure GPT to start at RX end */
    if( fdt != ST25R3911_FDT_NONE)
    {
        st25r3911StartGPTimer_8fcs( fdt, ST25R3911_REG_GPT_CONTROL_gptc_erx );
    }
    
    /* Now numbytes is again bytes */
    numbytes = (numbytes + 7) / 8;

    /* fifo depth is ST25R3911_FIFO_DEPTH */
    if (numbytes > ST25R3911_FIFO_DEPTH)
    {
        txbytes = ST25R3911_FIFO_DEPTH;
        numbytes -= ST25R3911_FIFO_DEPTH;
    }
    else
    {
        txbytes = numbytes;
        numbytes = 0;
    }

    /* fill up the FIFO */
    st25r3911WriteFifo(frame, txbytes);

#if ST25R3911_TXRX_ON_CSX
    st25r3911WriteTestRegister(0x1,0x0a); /* digital modulation on pin CSI */
#endif

    /* and transmit data */
    if (flags & ST25R3911_TX_FLAG_CRC)
    {
        st25r3911ExecuteCommand(ST25R3911_CMD_TRANSMIT_WITH_CRC);
    }
    else if (flags & ST25R3911_TX_FLAG_NFC_INITIATOR)
    {
        st25r3911ExecuteCommand(ST25R3911_CMD_INITIAL_RF_COLLISION);
    }
    else if (flags & ST25R3911_TX_FLAG_NFC_TARGET)
    {
        st25r3911ExecuteCommand(ST25R3911_CMD_RESPONSE_RF_COLLISION_N);
    }
    else
    {
        st25r3911ExecuteCommand(ST25R3911_CMD_TRANSMIT_WITHOUT_CRC);
    }

    /* check if there is still some more data to be transmitted */
    while (numbytes > 0)
    {
        frame += txbytes;
        if (numbytes > fifo_refill)
        {
            txbytes = fifo_refill;
            numbytes -= fifo_refill;
        }
        else
        {
            txbytes = numbytes;
            numbytes = 0;
        }

        /* wait for FIFO waterlevel interrupt */
        mask = st25r3911WaitForInterruptsTimed(ST25R3911_IRQ_MASK_FWL, 100);
        if (0 == mask)
        {
           return ERR_TIMEOUT;
        }

        /* fill up the FIFO */
        st25r3911WriteFifo(frame, txbytes);
    }

    /* wait for transmit finished interrupt. */
    mask = st25r3911WaitForInterruptsTimed(ST25R3911_IRQ_MASK_TXE, 100);
    if (0 == mask)
    {
        return ERR_TIMEOUT;
    }

#if ST25R3911_TXRX_ON_CSX
    st25r3911WriteTestRegister(0x1,0x04); /* digital demodulation on pins CSI and CSO(AM) */
#endif

    st25r3911ReadRegister(ST25R3911_REG_FIFO_RX_STATUS2,&reg);
    if (reg & (ST25R3911_REG_FIFO_RX_STATUS2_fifo_ovr | ST25R3911_REG_FIFO_RX_STATUS2_fifo_unf))
    {
        return ERR_FIFO;
    }

    st25r3911DisableInterrupts(ST25R3911_IRQ_MASK_TXE);
    return err;

}


ReturnCode st25r3911SetBitrate(uint8_t txRate, uint8_t rxRate)
{
    uint8_t reg;

    st25r3911ReadRegister(ST25R3911_REG_BIT_RATE, &reg);
    if (rxRate != ST25R3911_BR_DO_NOT_SET)
    {
        if(rxRate > ST25R3911_BR_3390)
        {
            return ERR_PARAM;
        }
        else
        {
            reg &= ~ST25R3911_REG_BIT_RATE_mask_rxrate;
            reg |= rxRate << ST25R3911_REG_BIT_RATE_shift_rxrate;
        }
    }
    if (txRate != ST25R3911_BR_DO_NOT_SET)
    {
        if(txRate > ST25R3911_BR_6780)
        {
            return ERR_PARAM;
        }
        else
        {
            reg &= ~ST25R3911_REG_BIT_RATE_mask_txrate;
            reg |= txRate<<ST25R3911_REG_BIT_RATE_shift_txrate;
        }
    }
    st25r3911WriteRegister(ST25R3911_REG_BIT_RATE, reg);
    
    return ERR_NONE;
}


uint16_t st25r3911MeasureVoltage(uint8_t mpsv)
{
    uint8_t result; 
    uint16_t mV;

    mpsv &= ST25R3911_REG_REGULATOR_CONTROL_mask_mpsv;

    st25r3911ModifyRegister(ST25R3911_REG_REGULATOR_CONTROL,
                         ST25R3911_REG_REGULATOR_CONTROL_mask_mpsv,
                         mpsv);

    st25r3911ExecuteCommandAndGetResult(ST25R3911_CMD_MEASURE_VDD,
            ST25R3911_REG_AD_RESULT,
            100,
            &result);
    mV = ((uint16_t)result) * 23;
    mV += ((((uint16_t)result) * 438) + 500) / 1000;

    return mV;
}


uint8_t st25r3911GetNumFIFOLastBits( void )
{
    uint8_t  reg;
    
    st25r3911ReadRegister( ST25R3911_REG_FIFO_RX_STATUS2, &reg );
    
    return ((reg & ST25R3911_REG_FIFO_RX_STATUS2_mask_fifo_lb) >> ST25R3911_REG_FIFO_RX_STATUS2_shift_fifo_lb);
}

uint32_t st25r3911GetNoResponseTime_64fcs()
{
    return st25r3911NoResponseTime_64fcs;
}

void st25r3911StartGPTimer_8fcs(uint16_t gpt_8fcs, uint8_t trigger_source)
{
    st25r3911SetGPTime_8fcs(gpt_8fcs);

    st25r3911ModifyRegister(ST25R3911_REG_GPT_CONTROL, 
            ST25R3911_REG_GPT_CONTROL_gptc_mask, 
            trigger_source);
    if (!trigger_source)
        st25r3911ExecuteCommand(ST25R3911_CMD_START_GP_TIMER);

    return;
}

void st25r3911SetGPTime_8fcs(uint16_t gpt_8fcs)
{
    st25r3911WriteRegister(ST25R3911_REG_GPT1, gpt_8fcs >> 8);
    st25r3911WriteRegister(ST25R3911_REG_GPT2, gpt_8fcs & 0xff);

    return;
}

bool st25r3911CheckReg( uint8_t reg, uint8_t mask, uint8_t val )
{    
    uint8_t regVal;
    
    regVal = 0;
    st25r3911ReadRegister( reg, &regVal );
    
    return ((regVal & mask) == val );
}


bool st25r3911CheckChipID( uint8_t *rev )
{
    uint8_t ID;
    
    ID = 0;    
    st25r3911ReadRegister( ST25R3911_REG_IC_IDENTITY, &ID );
    
    /* Check if IC Identity Register contains ST25R3911's IC type code */
    if( (ID & ST25R3911_REG_IC_IDENTITY_mask_ic_type) != ST25R3911_REG_IC_IDENTITY_ic_type )
    {
        return false;
    }
        
    if(rev != NULL)
    {
        *rev = (ID & ST25R3911_REG_IC_IDENTITY_mask_ic_rev);
    }
    
    return true;
}

ReturnCode st25r3911SetNoResponseTime_64fcs(uint32_t nrt_64fcs)
{
    ReturnCode err = ERR_NONE;
    uint8_t nrt_step = 0;

    st25r3911NoResponseTime_64fcs = nrt_64fcs;
    if (nrt_64fcs > USHRT_MAX)
    {
        nrt_step = ST25R3911_REG_GPT_CONTROL_nrt_step;
        nrt_64fcs = (nrt_64fcs + 63) / 64;
        if (nrt_64fcs > USHRT_MAX)
        {
            nrt_64fcs = USHRT_MAX;
            err = ERR_PARAM;
        }
        st25r3911NoResponseTime_64fcs = 64 * nrt_64fcs;
    }

    st25r3911ModifyRegister(ST25R3911_REG_GPT_CONTROL, ST25R3911_REG_GPT_CONTROL_nrt_step, nrt_step);
    st25r3911WriteRegister(ST25R3911_REG_NO_RESPONSE_TIMER1, nrt_64fcs >> 8);
    st25r3911WriteRegister(ST25R3911_REG_NO_RESPONSE_TIMER2, nrt_64fcs & 0xff);

    return err;
}

ReturnCode st25r3911SetStartNoResponseTime_64fcs(uint32_t nrt_64fcs)
{
    ReturnCode err;
    
    err = st25r3911SetNoResponseTime_64fcs( nrt_64fcs );
    if(err == ERR_NONE)
    {
        st25r3911ExecuteCommand(ST25R3911_CMD_START_NO_RESPONSE_TIMER);
    }
    
    return err;
}

ReturnCode st25r3911PerformCollisionAvoidance( uint8_t FieldONCmd, uint8_t pdThreshold, uint8_t caThreshold, uint8_t nTRFW )
{
    uint8_t  treMask;
    uint32_t irqs;
    
    if( (FieldONCmd != ST25R3911_CMD_INITIAL_RF_COLLISION)    && 
        (FieldONCmd != ST25R3911_CMD_RESPONSE_RF_COLLISION_0) && 
        (FieldONCmd != ST25R3911_CMD_RESPONSE_RF_COLLISION_N)   )
    {
        return ERR_PARAM;
    }
    
    /* Check if new thresholds are to be applied */
    if( (pdThreshold != ST25R3911_THRESHOLD_DO_NOT_SET) || (caThreshold != ST25R3911_THRESHOLD_DO_NOT_SET) )
    {
        treMask = 0;
        
        if(pdThreshold != ST25R3911_THRESHOLD_DO_NOT_SET)
        {
            treMask |= ST25R3911_REG_FIELD_THRESHOLD_mask_trg;
        }
        
        if(caThreshold != ST25R3911_THRESHOLD_DO_NOT_SET)
        {
            treMask |= ST25R3911_REG_FIELD_THRESHOLD_mask_rfe;
        }
            
        /* Set Detection Threshold and|or Collision Avoidance Threshold */
        st25r3911ChangeRegisterBits( ST25R3911_REG_FIELD_THRESHOLD, treMask, (pdThreshold & ST25R3911_REG_FIELD_THRESHOLD_mask_trg) | (caThreshold & ST25R3911_REG_FIELD_THRESHOLD_mask_rfe ) );
    }
    
    /* Set n x TRFW */
    st25r3911ModifyRegister(ST25R3911_REG_AUX, ST25R3911_REG_AUX_mask_nfc_n, (nTRFW & ST25R3911_REG_AUX_mask_nfc_n) );
    
    /* Enable and clear CA specific interrupts and execute command */
    st25r3911GetInterrupt( (ST25R3911_IRQ_MASK_CAC | ST25R3911_IRQ_MASK_CAT) );
    st25r3911EnableInterrupts( (ST25R3911_IRQ_MASK_CAC | ST25R3911_IRQ_MASK_CAT) );
    
    st25r3911ExecuteCommand(FieldONCmd);
    
    irqs = st25r3911WaitForInterruptsTimed(ST25R3911_IRQ_MASK_CAC | ST25R3911_IRQ_MASK_CAT, ST25R3911_CA_TIMEOUT );
    
    /* Clear any previous External Field events and disable CA specific interrupts */
    st25r3911GetInterrupt( (ST25R3911_IRQ_MASK_EOF | ST25R3911_IRQ_MASK_EON) );
    st25r3911DisableInterrupts(ST25R3911_IRQ_MASK_CAC | ST25R3911_IRQ_MASK_CAT);
    
    
    if(ST25R3911_IRQ_MASK_CAC & irqs)                             /* Collision occurred */
    {
        return ERR_RF_COLLISION;
    }
    
    if(ST25R3911_IRQ_MASK_CAT & irqs)                             /* No Collision detected, Field On */
    {
        return ERR_NONE;
    }

    /* No interrupt detected */
    return ERR_INTERNAL;
}

ReturnCode st25r3911GetRegsDump(uint8_t* resRegDump, uint8_t* sizeRegDump)
{
    uint8_t regIt;
    uint8_t regDump[ST25R3911_REG_IC_IDENTITY+1];
    
    if(!sizeRegDump || !resRegDump)
    {
        return ERR_PARAM;
    }
    
    for( regIt = ST25R3911_REG_IO_CONF1; regIt < SIZEOF_ARRAY(regDump); regIt++ )
    {
        st25r3911ReadRegister(regIt, &regDump[regIt] );
    }
    
    *sizeRegDump = MIN(*sizeRegDump, regIt);    
    ST_MEMCPY(resRegDump, regDump, *sizeRegDump );

    return ERR_NONE;
}


void st25r3911SetNumTxBits( uint32_t nBits )
{
    st25r3911WriteRegister(ST25R3911_REG_NUM_TX_BYTES2, (uint8_t)((nBits >> 0) & 0xff)); 
    st25r3911WriteRegister(ST25R3911_REG_NUM_TX_BYTES1, (uint8_t)((nBits >> 8) & 0xff));    
}


bool st25r3911IsCmdValid( uint8_t cmd )
{
    if( !((cmd >= ST25R3911_CMD_SET_DEFAULT)       && (cmd <= ST25R3911_CMD_ANALOG_PRESET))           && 
        !((cmd >= ST25R3911_CMD_MASK_RECEIVE_DATA) && (cmd <= ST25R3911_CMD_CLEAR_RSSI))              &&
        !((cmd >= ST25R3911_CMD_TRANSPARENT_MODE)  && (cmd <= ST25R3911_CMD_START_NO_RESPONSE_TIMER)) &&
        !((cmd >= ST25R3911_CMD_TEST_CLEARA)       && (cmd <= ST25R3911_CMD_FUSE_PPROM))               )        
    {
        return false;
    }
    return true;
}

ReturnCode st25r3911StreamConfigure(const struct st25r3911StreamConfig *config)
{
    uint8_t smd = 0;
    uint8_t mode;

    if (config->useBPSK)
    {
        mode = ST25R3911_REG_MODE_om_bpsk_stream;
        if (config->din<2 || config->din>4) /* not in fc/4 .. fc/16 */
        {
            return ERR_PARAM;
        }
        smd |= (4 - config->din) << ST25R3911_REG_STREAM_MODE_shift_scf;

    }
    else
    {
        mode = ST25R3911_REG_MODE_om_subcarrier_stream;
        if (config->din<3 || config->din>6) /* not in fc/8 .. fc/64 */
        {
            return ERR_PARAM;
        }
        smd |= (6 - config->din) << ST25R3911_REG_STREAM_MODE_shift_scf;
        if (config->report_period_length == 0) 
        {
            return ERR_PARAM;
        }
    }

    if (config->dout<1 || config->dout>7) /* not in fc/2 .. fc/128 */
    {
        return ERR_PARAM;
    }
    smd |= (7 - config->dout) << ST25R3911_REG_STREAM_MODE_shift_stx;

    if (config->report_period_length > 3) 
    {
        return ERR_PARAM;
    }
    smd |= config->report_period_length << ST25R3911_REG_STREAM_MODE_shift_scp;

    st25r3911ModifyRegister(ST25R3911_REG_AUX, ST25R3911_REG_AUX_no_crc_rx, ST25R3911_REG_AUX_no_crc_rx);
    st25r3911WriteRegister(ST25R3911_REG_STREAM_MODE, smd);
    st25r3911WriteRegister(ST25R3911_REG_MODE, mode);

    return ERR_NONE;
}

bool st25r3911IrqIsWakeUpCap( void )
{
  return ( st25r3911GetInterrupt(ST25R3911_IRQ_MASK_WCAP) ? true : false );
}


bool st25r3911IrqIsWakeUpPhase( void )
{
  return ( st25r3911GetInterrupt(ST25R3911_IRQ_MASK_WPH) ? true : false );
}


bool st25r3911IrqIsWakeUpAmplitude( void )
{
  return ( st25r3911GetInterrupt(ST25R3911_IRQ_MASK_WAM) ? true : false );
}

/*
******************************************************************************
* LOCAL FUNCTIONS
******************************************************************************
*/
/*! 
 *****************************************************************************
 *  \brief  Executes a direct command and returns the result
 *
 *  This function executes the direct command given by \a cmd waits for
 *  \a sleeptime and returns the result read from register \a resreg.
 *
 *  \param[in] cmd: direct command to execute.
 *  \param[in] resreg: Address of the register containing the result.
 *  \param[in] sleeptime: time in milliseconds to wait before reading the result.
 *  \param[out] result: 8 bit long result
 *
 *****************************************************************************
 */
static ReturnCode st25r3911ExecuteCommandAndGetResult(uint8_t cmd, uint8_t resreg, uint8_t sleeptime, uint8_t* result)
{

    if (   (cmd >= ST25R3911_CMD_INITIAL_RF_COLLISION && cmd <= ST25R3911_CMD_RESPONSE_RF_COLLISION_0)
            || (cmd == ST25R3911_CMD_MEASURE_AMPLITUDE)
            || (cmd >= ST25R3911_CMD_ADJUST_REGULATORS && cmd <= ST25R3911_CMD_MEASURE_PHASE)
            || (cmd >= ST25R3911_CMD_CALIBRATE_C_SENSOR && cmd <= ST25R3911_CMD_MEASURE_VDD)
            || (cmd >= 0xFD && cmd <= 0xFE )
       )
    {
        st25r3911EnableInterrupts(ST25R3911_IRQ_MASK_DCT);
        st25r3911GetInterrupt(ST25R3911_IRQ_MASK_DCT);
        st25r3911ExecuteCommand(cmd);
        st25r3911WaitForInterruptsTimed(ST25R3911_IRQ_MASK_DCT, sleeptime);
        st25r3911DisableInterrupts(ST25R3911_IRQ_MASK_DCT);
    }
    else
    {
        return ERR_PARAM;
    }

    /* read out the result if the pointer is not NULL */
    if (result)
        st25r3911ReadRegister(resreg, result);

    return ERR_NONE;

}

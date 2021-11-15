
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
 *  \brief ST25R3911 declaration file
 *
 * API:
 * - Initialize ST25R3911 driver: #st25r3911Initialize
 * - Deinitialize ST25R3911 driver: #st25r3911Deinitialize
 *
 *
 * @addtogroup RFAL
 * @{
 *
 * @addtogroup RFAL-HAL
 * @brief RFAL Hardware Abstraction Layer
 * @{
 *
 * @addtogroup ST25R3911
 * @brief RFAL ST25R3911 Driver
 * @{
 * 
 * @addtogroup ST25R3911_Driver
 * @brief RFAL ST25R3911 Driver
 * @{
 * 
 */

#ifndef ST25R3911_H
#define ST25R3911_H

/*
******************************************************************************
* INCLUDES
******************************************************************************
*/
#include <stdint.h>
#include <stdbool.h>
#include "st_errno.h"

/*
******************************************************************************
* GLOBAL DATATYPES
******************************************************************************
*/
/*! 
 * flags used for #st25r3911TxNBytes
 */
typedef enum
{
    ST25R3911_TX_FLAG_NONE = 0x0,  /*!< nomen est omen */
    ST25R3911_TX_FLAG_CRC = 0x1,  /*!< append crc sum to frame */
    ST25R3911_TX_FLAG_ANTCL = 0x2, /*!< send anticollision frame for 14443a*/
    ST25R3911_TX_FLAG_NFC_INITIATOR = 0x4, /*!< frame sent with initial collision avoidance */
    ST25R3911_TX_FLAG_NFC_TARGET = 0x8, /*!< frame sent with response collision avoidance */
}st25r3911TxFlag_t;


/*! Parameters how the stream mode should work */
struct st25r3911StreamConfig {
    uint8_t useBPSK; /*!< 0: subcarrier, 1:BPSK */
    uint8_t din; /*!< the divider for the in subcarrier frequency: fc/2^din  */
    uint8_t dout; /*!< the divider for the in subcarrier frequency fc/2^dout */
    uint8_t report_period_length; /*!< the length of the reporting period 2^report_period_length*/
};


/*
******************************************************************************
* GLOBAL DEFINES
******************************************************************************
*/
#define ST25R3911_FDT_NONE                     0x00    /*!< Value indicating not to perform FDT  */

#define MS_TO_64FCS(A)               ((A) * 212)    /*!< Converts from ms to 64/fc steps      */
#define MS_FROM_64FCS(A)             ((A) / 212)    /*!< Converts from 64/fc steps to ms      */

/* ST25R3911 direct commands */
#define ST25R3911_CMD_SET_DEFAULT              0xC1    /*!< Puts the chip in default state (same as after power-up) */
#define ST25R3911_CMD_CLEAR_FIFO               0xC2    /*!< Stops all activities and clears FIFO */
#define ST25R3911_CMD_TRANSMIT_WITH_CRC        0xC4    /*!< Transmit with CRC */
#define ST25R3911_CMD_TRANSMIT_WITHOUT_CRC     0xC5    /*!< Transmit without CRC */
#define ST25R3911_CMD_TRANSMIT_REQA            0xC6    /*!< Transmit REQA */
#define ST25R3911_CMD_TRANSMIT_WUPA            0xC7    /*!< Transmit WUPA */
#define ST25R3911_CMD_INITIAL_RF_COLLISION     0xC8    /*!< NFC transmit with Initial RF Collision Avoidance */
#define ST25R3911_CMD_RESPONSE_RF_COLLISION_N  0xC9    /*!< NFC transmit with Response RF Collision Avoidance */
#define ST25R3911_CMD_RESPONSE_RF_COLLISION_0  0xCA    /*!< NFC transmit with Response RF Collision Avoidance with n=0 */
#define ST25R3911_CMD_NORMAL_NFC_MODE          0xCB    /*!< NFC switch to normal NFC mode */
#define ST25R3911_CMD_ANALOG_PRESET            0xCC    /*!< Analog Preset */
#define ST25R3911_CMD_MASK_RECEIVE_DATA        0xD0    /*!< Mask recive data */
#define ST25R3911_CMD_UNMASK_RECEIVE_DATA      0xD1    /*!< Unmask recive data */
#define ST25R3911_CMD_MEASURE_AMPLITUDE        0xD3    /*!< Measure singal amplitude on RFI inputs */
#define ST25R3911_CMD_SQUELCH                  0xD4    /*!< Squelch */
#define ST25R3911_CMD_CLEAR_SQUELCH            0xD5    /*!< Clear Squelch */
#define ST25R3911_CMD_ADJUST_REGULATORS        0xD6    /*!< Adjust regulators */
#define ST25R3911_CMD_CALIBRATE_MODULATION     0xD7    /*!< Calibrate modulation depth */
#define ST25R3911_CMD_CALIBRATE_ANTENNA        0xD8    /*!< Calibrate antenna */
#define ST25R3911_CMD_MEASURE_PHASE            0xD9    /*!< Measure phase between RFO and RFI signal */
#define ST25R3911_CMD_CLEAR_RSSI               0xDA    /*!< clear RSSI bits and restart the measurement */
#define ST25R3911_CMD_TRANSPARENT_MODE         0xDC    /*!< Transparent mode */
#define ST25R3911_CMD_CALIBRATE_C_SENSOR       0xDD    /*!< Calibrate the capacitive sensor */
#define ST25R3911_CMD_MEASURE_CAPACITANCE      0xDE    /*!< Measure capacitance */
#define ST25R3911_CMD_MEASURE_VDD              0xDF    /*!< Measure power supply voltage */
#define ST25R3911_CMD_START_GP_TIMER           0xE0    /*!< Start the general purpose timer */
#define ST25R3911_CMD_START_WUP_TIMER          0xE1    /*!< Start the wake-up timer */
#define ST25R3911_CMD_START_MASK_RECEIVE_TIMER 0xE2    /*!< Start the mask-receive timer */
#define ST25R3911_CMD_START_NO_RESPONSE_TIMER  0xE3    /*!< Start the no-repsonse timer */
#define ST25R3911_CMD_TEST_CLEARA              0xFA    /*!< Clear Test register */
#define ST25R3911_CMD_TEST_CLEARB              0xFB    /*!< Clear Test register */
#define ST25R3911_CMD_TEST_ACCESS              0xFC    /*!< Enable R/W access to the test registers */
#define ST25R3911_CMD_LOAD_PPROM               0xFD    /*!< Load data from the poly fuses to RAM */
#define ST25R3911_CMD_FUSE_PPROM               0xFE    /*!< Fuse poly fuses with data from the RAM */


#define ST25R3911_FIFO_DEPTH                   96      /*!< Depth of FIFO                            */

#define ST25R3911_THRESHOLD_DO_NOT_SET         0xFF    /*!< Indicates not to change this Threshold   */

#define ST25R3911_BR_DO_NOT_SET                0xFF    /*!< Indicates not to change this Bit Rate    */
#define ST25R3911_BR_106                       0x00    /*!< ST25R3911 Bit Rate 106 kbit/s (fc/128)   */
#define ST25R3911_BR_212                       0x01    /*!< ST25R3911 Bit Rate 212 kbit/s (fc/64)    */
#define ST25R3911_BR_424                       0x02    /*!< ST25R3911 Bit Rate 424 kbit/s (fc/32)    */
#define ST25R3911_BR_848                       0x03    /*!< ST25R3911 Bit Rate 848 kbit/s (fc/16)    */
#define ST25R3911_BR_1695                      0x04    /*!< ST25R3911 Bit Rate 1696 kbit/s (fc/8)    */
#define ST25R3911_BR_3390                      0x05    /*!< ST25R3911 Bit Rate 3390 kbit/s (fc/4)    */
#define ST25R3911_BR_6780                      0x06    /*!< ST25R3911 Bit Rate 6780 kbit/s (fc/2)    */


/*
******************************************************************************
* GLOBAL MACROS
******************************************************************************
*/

/*! Checks if General Purpose Timer is still running by reading gpt_on flag */
#define st25r3911IsGPTRunning( )     ( st25r3911CheckReg(ST25R3911_REG_REGULATOR_RESULT, ST25R3911_REG_REGULATOR_RESULT_gpt_on, ST25R3911_REG_REGULATOR_RESULT_gpt_on) )

/*! Checks if CRC is configured to be in FIFO                               */
#define st25r3911IsCRCinFIFO( )      ( st25r3911CheckReg(ST25R3911_REG_AUX, ST25R3911_REG_AUX_crc_2_fifo, ST25R3911_REG_AUX_crc_2_fifo) )

/*! Checks if External Filed is detected by reading ST25R3911 External Field  
 * Detector output                                                          */
#define st25r3911IsExtFieldOn()      ( st25r3911CheckReg(ST25R3911_REG_AUX_DISPLAY, ST25R3911_REG_AUX_DISPLAY_efd_o, ST25R3911_REG_AUX_DISPLAY_efd_o ) )

/*! Checks if Transmitter is enabled (Field On) */
#define st25r3911IsTxEnabled()       ( st25r3911CheckReg(ST25R3911_REG_OP_CONTROL, ST25R3911_REG_OP_CONTROL_tx_en, ST25R3911_REG_OP_CONTROL_tx_en ) )

/*! Turn Off Tx (Field Off) */
#define st25r3911TxOff()              st25r3911ClrRegisterBits(ST25R3911_REG_OP_CONTROL, ST25R3911_REG_OP_CONTROL_tx_en );

/*! Checks if last FIFO byte is complete */
#define st25r3911IsLastFIFOComplete() st25r3911CheckReg( ST25R3911_REG_FIFO_RX_STATUS2, ST25R3911_REG_FIFO_RX_STATUS2_mask_fifo_lb, 0 )

/*! Checks if the Oscillator is enabled  */
#define st25r3911IsOscOn()            st25r3911CheckReg( ST25R3911_REG_OP_CONTROL, ST25R3911_REG_OP_CONTROL_en, ST25R3911_REG_OP_CONTROL_en )

/*
******************************************************************************
* GLOBAL FUNCTION PROTOTYPES
******************************************************************************
*/

/*! 
 *****************************************************************************
 *  \brief  Turn on Oscillator and Regulator
 *  
 *  This function turn on oscillator and regulator and wait for the oscillator to 
 *  become stable.
 * 
 *****************************************************************************
 */
extern void st25r3911OscOn( void );

/*! 
 *****************************************************************************
 *  \brief  Turn On Tx and Rx
 *
 *  This function turns On Tx and Rx (Field On)
 *
 *****************************************************************************
 */
extern void st25r3911TxRxOn( void );

/*! 
 *****************************************************************************
 *  \brief  Turn Off Tx and Rx
 *
 *  This function turns Off Tx and Rx (Field Off)
 *
 *****************************************************************************
 */
extern void st25r3911TxRxOff( void );

/*! 
 *****************************************************************************
 *  \brief  Initialise ST25R3911 driver
 *
 *  This function initialises the ST25R3911 driver.
 *
 *****************************************************************************
 */
extern void st25r3911Initialize( void );

/*! 
 *****************************************************************************
 *  \brief  Deinitialize ST25R3911 driver
 *
 *  Calling this function deinitializes the ST25R3911 driver.
 *
 *****************************************************************************
 */
extern void st25r3911Deinitialize( void );


/*! 
 *****************************************************************************
 *  \brief  Sets the bitrate registers
 *
 *  This function sets the bitrate register for rx and tx
 *
 *  \param txRate : speed is 2^txrate * 106 kb/s
 *                  0xff : don't set txrate
 *  \param rxRate : speed is 2^rxrate * 106 kb/s
 *                  0xff : don't set rxrate
 *
 *  \return ERR_NONE : No error, both bit rates were set
 *  \return ERR_PARAM: At least one bit rate was invalid
 *
 *****************************************************************************
 */
extern ReturnCode st25r3911SetBitrate(uint8_t txRate, uint8_t rxRate);

/*! 
 *****************************************************************************
 *  \brief  Adjusts supply regulators according to the current supply voltage
 *
 *  This function the power level is measured in maximum load conditions and
 *  the regulated voltage reference is set to 250mV below this level.
 *  Execution of this function lasts arround 5ms. 
 *  
 *  \param [out] result_mV : Result of calibration in milliVolts.
 *
 *  \return ERR_REQUEST : Adjustment not possible since reg_s bit is set.
 *  \return ERR_IO : Error during communication with ST25R3911.
 *  \return ERR_NONE : No error.
 *
 *****************************************************************************
 */
extern ReturnCode st25r3911AdjustRegulators(uint16_t* result_mV);

/*! 
 *****************************************************************************
 *  \brief  Measure RF
 *
 *  This function measured the amplitude on the RFI inputs and stores the
 *  result in parameter \a result.
 *
 *  \param[out] result: 8 bit long result of RF measurement.
 *
 *****************************************************************************
 */
extern void st25r3911MeasureRF(uint8_t* result);

/*! 
 *****************************************************************************
 *  \brief  Measure Capacitance
 *
 *  This function performs the capacitance measurement and stores the
 *  result in parameter \a result.
 *
 *  \param[out] result: 8 bit long result of RF measurement.
 *
 *****************************************************************************
 */
extern void st25r3911MeasureCapacitance(uint8_t* result);

/*! 
 *****************************************************************************
 *  \brief  Measure Voltage
 *
 *  This function measures the voltage on one of VDD and VSP_*
 *  result in parameter \a result.
 *
 *  \param[in] mpsv : one of ST25R3911_REG_REGULATOR_CONTROL_mpsv_vdd
 *                           ST25R3911_REG_REGULATOR_CONTROL_mpsv_vsp_rf
 *                           ST25R3911_REG_REGULATOR_CONTROL_mpsv_vsp_a
 *                    or     ST25R3911_REG_REGULATOR_CONTROL_mpsv_vsp_d
 *
 *  \return the measured voltage in mV
 *
 *****************************************************************************
 */
extern uint16_t st25r3911MeasureVoltage(uint8_t mpsv);

/*! 
 *****************************************************************************
 *  \brief  Calibrate antenna
 *
 *  This function is used to calibrate the antenna using a special sequence.
 *  The result is stored in the \a result parameter.
 *
 *  \param[out] result: 8 bit long result of antenna calibration algorithm.
 *
 *****************************************************************************
 */
extern void st25r3911CalibrateAntenna(uint8_t* result);

/*! 
 *****************************************************************************
 *  \brief  Check antenna resonance
 *
 *  This function is used to measure the antenna LC tank resconance to determine
 *  whether a calibration is needed.
 *  The result is stored in the \a result parameter.
 *
 *  \param[out] result: 8 bit long result of the measurement.
 *
 *****************************************************************************
 */
extern void st25r3911MeasureAntennaResonance(uint8_t* result);

/*! 
 *****************************************************************************
 *  \brief  Calibrate modulation depth
 *
 *  This function is used to calibrate the modulation depth using a special sequence.
 *  The result is stored in the \a result parameter.
 *
 *  \param[out] result: 8 bit long result of antenna calibration algorithm.
 *
 *****************************************************************************
 */
extern void st25r3911CalibrateModulationDepth(uint8_t* result);


/*! 
 *****************************************************************************
 *  \brief  Calibrate Capacitive Sensor
 *
 *  This function is used to calibrates the Capacitive Sensor.
 *  The result is stored in the \a result parameter.
 *
 *  \param[out] result: 8 bit long result of antenna calibration algorithm.
 *
 *****************************************************************************
 */
extern void st25r3911CalibrateCapacitiveSensor(uint8_t* result);

/*! 
 *****************************************************************************
 *  \brief  Prepare for upcomming receive
 *
 *  This function \b must be called before calling #st25r3911RxNBytes.
 *  It enables all required interrupt and prepares receive logic.
 *  It should be called before a request is sent to a PICC, i.e.
 *  first this function is to be called, then the request is sent to the PICC
 *  and the the result is fetched using #st25r3911RxNBytes.
 *  \note This function enables RXS and FWL interrupt of ST25R3911.
 *
 *  \param[in] reset : Reset receive logic
 *
 *****************************************************************************
 */
extern void st25r3911PrepareReceive(bool reset);

/*! 
 *****************************************************************************
 *  \brief  Read a given number of bytes out of the ST25R3911 FIFO
 *
 *  This function needs to be called in order to read from ST25R3911 FIFO.
 *  \note After leaving this function RXS and FWL interrupt of ST25R3911 are
 *  disabled regardless on their enable status before calling this function.
 *  It is important to call #st25r3911PrepareReceive prior to this function call.
 *
 *  \param[out]  buf: pointer to a buffer where the FIFO content shall be
 *                       written to.
 *  \param[in]  maxlength: Maximum number of values to be read. (= size of \a buf)
 *  \param[out] length: Actual read FIFO entries
 *  \param[in]  timeout_ms: timeout value for waiting for an interrupt, if 0 
 *                          value aof st25r3911GetNoResponseTime_64fcs() is used.
 *
 *  \return ERR_NOMSG : Parity for frameing error
 *  \return ERR_REQUEST : Function can't be executed as it wasn't prepared
 *                     properly using #st25r3911PrepareReceive
 *  \return ERR_TIMEOUT : Receive timeout, No Resonpse IRQ or General Purpose IRQ
 *  \return ERR_IO : Error during communication.
 *  \return ERR_NONE : No error, \a length values read.
 *
 *****************************************************************************
 */
extern ReturnCode st25r3911RxNBytes(uint8_t* buf, uint16_t maxlength, uint16_t* length, uint16_t timeout_ms);

/*! 
 *****************************************************************************
 *  \brief  Transmit a given number of bytes
 *
 *  This function takes the data given by \a frame and writes them into the FIFO of
 *  the ST25R3911. After that transmission is started. Depending on \a flags parameter
 *  ISO14443a/b or NFC frame is sent.
 *  For ISO14443a it is  possible to send a bit-oriented frame used for anticollision.
 *  \note This function only sends this frame. It doesn't wait for a reply.
 *
 *  \param[in] frame: data to be transmitted.
 *  \param[in] numbytes : Number of bytes to transmit.
 *  \param[in] numbits: Only used in case of ISO14443a anti-collision. If
 *                this parameter is > 0 then the last byte of \a frame contains
 *                these bits.
 *  \param[in] flags : Flags to control this function. Possible values:
 *        #ST25R3911_TX_FLAG_CRC : ST25R3911 appends CRC checksum to frame
 *        #ST25R3911_TX_FLAG_ANTCL: for ISO14443a anti-collision is sent. Parameter
 *                            \a numbits is used in this case)
 *        #ST25R3911_TX_FLAG_NFC_INITIATOR : Frame is sent with initial collision
 *                                        avoidance.
 *        #ST25R3911_TX_FLAG_NFC_TARGET : Frame is sent with response collision
 *                                        avoidance.
 *  \param[in] fdt : fdt (Frame Delay Time) to be used between this and following Tx
 *  
 *  \return ERR_TIMEOUT : Timeout waiting for interrupts
 *  \return ERR_IO : Error during communication.
 *  \return ERR_NONE : No error.
 *
 *****************************************************************************
 */
ReturnCode st25r3911TxNBytes( const uint8_t* frame, uint16_t numbytes, uint8_t numbits, st25r3911TxFlag_t flags, uint16_t fdt );

/*! 
 *****************************************************************************
 *  \brief  set no response time
 *
 *  This function executes sets the no response time to the defines value
 *
 *  \param nrt_64fcs : no response time in 64/fc = 4.72us
 *                    completion interrupt
 *
 *  \return ERR_PARAM : if time is too large
 */
extern ReturnCode st25r3911SetNoResponseTime_64fcs(uint32_t nrt_64fcs);

/*! 
 *****************************************************************************
 *  \brief  set no response time
 *
 *  This function executes sets and immediately start the no response timer
 *   to the defines value
 *   This is used when needs to add more time before timeout whitout Tx
 *
 *  \param nrt_64fcs : no response time in 64/fc = 4.72us
 *                    completion interrupt
 *
 *  \return ERR_PARAM : if time is too large
 */
extern ReturnCode st25r3911SetStartNoResponseTime_64fcs(uint32_t nrt_64fcs);

/*! 
 *****************************************************************************
 *  \brief  Perform Collision Avoidance
 *
 *  Performs Collision Avoidance with the given threshold and with the  
 *  n number of TRFW 
 *  
 *  \param[in] FieldONCmd  : Field ON command to be executed ST25R3911_CMD_INITIAL_RF_COLLISION
 *                           or ST25R3911_CMD_RESPONSE_RF_COLLISION_0/N    
 *  \param[in] pdThreshold : Peer Detection Threshold  (ST25R3916_REG_FIELD_THRESHOLD_trg_xx)
 *                           0xff : don't set Threshold (ST25R3916_THRESHOLD_DO_NOT_SET)
 *  \param[in] caThreshold : Collision Avoidance Threshold (ST25R3916_REG_FIELD_THRESHOLD_rfe_xx)
 *                           0xff : don't set Threshold (ST25R3916_THRESHOLD_DO_NOT_SET)
 *  \param[in] nTRFW       : Number of TRFW
 * 
 *  \return ERR_NONE : no collision detected
 *  \return ERR_RF_COLLISION : collision detected
 */
extern ReturnCode st25r3911PerformCollisionAvoidance( uint8_t FieldONCmd, uint8_t pdThreshold, uint8_t caThreshold, uint8_t nTRFW );


/*! 
 *****************************************************************************
 *  \brief  Get amount of bits of the last FIFO byte if incomplete
 *  
 *  Gets the number of bits of the last FIFO byte if incomplete
 *  
 *  \return the number of bits of the last FIFO byte if incomplete, 0 if 
 *          the last byte is complete
 *    
 *****************************************************************************
 */
extern uint8_t st25r3911GetNumFIFOLastBits( void );

/*! 
 *****************************************************************************
 *  \brief  Get NRT time
 *
 *  This returns the last value set on the NRT
 *   
 *  \warning it does not reads chip register, just the sw var that contains the 
 *  last value set before
 *
 *  \return the value of the NRT
 */
extern uint32_t st25r3911GetNoResponseTime_64fcs(void);

/*! 
 *****************************************************************************
 *  \brief  set general purpose timer timeout
 *
 *  This function sets the proper registers but does not start the timer actually
 *
 *  \param gpt_8fcs : general purpose timer timeout in 8/fc = 590ns
 *
 */
extern void st25r3911SetGPTime_8fcs(uint16_t gpt_8fcs);
/*! 
 *****************************************************************************
 *  \brief  Starts GPT with given timeout
 *
 *  This function starts the general purpose timer with the given timeout
 *
 *  \param gpt_8fcs : general purpose timer timeout in 8/fc = 590ns
 *  \param trigger_source : no trigger, start of Rx, end of Rx, end of Tx in NFC mode 
 */
extern void st25r3911StartGPTimer_8fcs(uint16_t gpt_8fcs, uint8_t trigger_source);

/*! 
 *****************************************************************************
 *  \brief  Checks if register contains a expected value
 *
 *  This function checks if the given reg contains a value that once masked
 *  equals the expected value
 *
 *  \param reg  : the register to check the value
 *  \param mask : the mask apply on register value
 *  \param val  : expected value to be compared to
 *    
 *  \return  true when reg contains the expected value | false otherwise
 */
bool st25r3911CheckReg( uint8_t reg, uint8_t mask, uint8_t val );

/*! 
 *****************************************************************************
 *  \brief  Sets the number Tx Bits
 *  
 *  Sets ST25R3911 internal registers with correct number of complete bytes and
 *  bits to be sent
 *
 *  \param nBits : the number bits to be transmitted  
 *****************************************************************************
 */
void st25r3911SetNumTxBits( uint32_t nBits );

/*! 
 *****************************************************************************
 *  \brief  Check Identity
 *
 *  Checks if the chip ID is as expected.
 *  
 *  5 bit IC type code for ST25R3911: 00001
 *  The 3 lsb contain the IC revision code
 *  
 *
 *  \param[out] rev  : the IC revision code 
 *    
 *  \return  true when IC type is as expected
 *  
 *****************************************************************************
 */
bool st25r3911CheckChipID( uint8_t *rev );

/*! 
 *****************************************************************************
 *  \brief  Check if command is valid
 *
 *  Checks if the given command is a valid ST25R3911 command
 *
 *  \param[in] cmd: Command to check
 *  
 *  \return  true if is a valid command
 *  \return  false otherwise
 *
 *****************************************************************************
 */
bool st25r3911IsCmdValid( uint8_t cmd );

/*! 
 *****************************************************************************
 *  \brief  Configure the stream mode of ST25R3911
 *
 *  This function initializes the stream with the given parameters
 *
 *  \param[in] config : all settings for bitrates, type, etc.

 *  \return ERR_NONE : No error, stream mode driver initialized.
 *
 *****************************************************************************
 */
extern ReturnCode st25r3911StreamConfigure(const struct st25r3911StreamConfig *config);

/*! 
 *****************************************************************************
 *  \brief  Retrieves all  internal registers from st25r3911
 */
extern ReturnCode st25r3911GetRegsDump(uint8_t* resRegDump, uint8_t* sizeRegDump);


/*! 
 *****************************************************************************
 *  \brief  Cheks if a Wakeup IRQ due to Capacitive measument has happen
 */
extern bool st25r3911IrqIsWakeUpCap( void );


/*! 
 *****************************************************************************
 *  \brief  Cheks if a Wakeup IRQ due to Phase measument has happen
 */
extern bool st25r3911IrqIsWakeUpPhase( void );


/*! 
 *****************************************************************************
 *  \brief  Cheks if a Wakeup IRQ due to Amplitude measument has happen
 */
extern bool st25r3911IrqIsWakeUpAmplitude( void );

#endif /* ST25R3911_H */

/**
  * @}
  *
  * @}
  *
  * @}
  * 
  * @}
  */


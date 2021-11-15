
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
 *      PROJECT:   ST25R391x firmware
 *      $Revision: $
 *      LANGUAGE:  ISO C99
 */

/*! \file rfal_nfcv.h
 *
 *  \author Gustavo Patricio
 *
 *  \brief Implementation of NFC-V Poller (ISO15693) device
 *
 *  The definitions and helpers methods provided by this module 
 *  are aligned with NFC-V Digital 2.0 (Candidate)  
 *
 *
 * @addtogroup RFAL
 * @{
 *
 * @addtogroup RFAL-AL
 * @brief RFAL Abstraction Layer
 * @{
 *
 * @addtogroup NFC-V
 * @brief RFAL NFC-V Module
 * @{
 * 
 */

#ifndef RFAL_NFCV_H
#define RFAL_NFCV_H

/*
 ******************************************************************************
 * INCLUDES
 ******************************************************************************
 */
#include <stdint.h>
#include <stdbool.h>
#include "st_errno.h"
#include "rfal_rf.h"

/*
 ******************************************************************************
 * GLOBAL DEFINES
 ******************************************************************************
 */
#define RFAL_NFCV_UID_LEN                           8    /*!< NFC-V UID length  */

/*
 ******************************************************************************
 * GLOBAL MACROS
 ******************************************************************************
 */


/*
******************************************************************************
* GLOBAL TYPES
******************************************************************************
*/

/*! NFC-V Number of slots  Digital 2.0 (Candidate) 9.6.1 */
typedef enum 
{
    RFAL_NFCV_NUM_SLOTS_1    =  0x20,   /*!< Number of slots: 1 */
    RFAL_NFCV_NUM_SLOTS_16   =  0x00,   /*!< Number of slots: 16 */
} rfalNfcvNumSlots;


/*! NFC-V INVENTORY_RES format   Digital 2.0 (Candidate) 9.6.2 */
typedef struct 
{
    uint8_t RES_FLAG;                   /*!< Response Flags                 */
    uint8_t DSFID;                      /*!< Data Storage Format Identifier */
    uint8_t UID[RFAL_NFCV_UID_LEN];     /*!< NFC-V device UID               */
    uint8_t crc[RFAL_CRC_LEN];          /*!< CRC                            */
} rfalNfcvInventoryRes;


/*! NFC-V listener device (VICC) struct  */
typedef struct
{
    rfalNfcvInventoryRes    InvRes;     /*!< INVENTORY_RES                  */
    bool                    isSleep;    /*!< Device sleeping flag           */
} rfalNfcvListenDevice;


/*
******************************************************************************
* GLOBAL FUNCTION PROTOTYPES
******************************************************************************
*/

/*! 
 *****************************************************************************
 * \brief  Initialize NFC-V Poller mode
 *  
 * This methods configures RFAL RF layer to perform as a 
 * NFC-F Poller/RW (ISO15693) including all default timings 
 *
 * \return ERR_WRONG_STATE  : RFAL not initialized or mode not set
 * \return ERR_PARAM        : Incorrect bitrate
 * \return ERR_NONE         : No error
 *****************************************************************************
 */
ReturnCode rfalNfcvPollerInitialize( void );

/*! 
 *****************************************************************************
 * \brief  NFC-V Poller Check Presence
 *  
 * This method checks if a NFC-V Listen device (VICC) is present on the field
 * by sending an Inventory (INVENTORY_REQ) 
 *  
 * \param[out] invRes : If received, the INVENTORY_RES
 *
 * \return ERR_WRONG_STATE  : RFAL not initialized or incorrect mode
 * \return ERR_PARAM        : Invalid parameters
 * \return ERR_IO           : Generic internal error
 * \return ERR_TIMEOUT      : Timeout error, no listener device detectedd
 * \return ERR_NONE         : No error, one or more device in the field
 *****************************************************************************
 */
ReturnCode rfalNfcvPollerCheckPresence( rfalNfcvInventoryRes *invRes );

/*! 
 *****************************************************************************
 * \brief NFC-F Poller Poll
 * 
 * This function sends to all VICCs in field the INVENTORY command with the 
 * given number of slots
 * 
 * If more than one slot is used the following EOF need to be handled
 * by the caller using rfalISO15693TransceiveAnticollisionEOF()
 *
 * \param[in]  nSlots  : Number of Slots to be sent (1 or 16)
 * \param[in]  maskLen : Number bits on the Mask value
 * \param[in]  maskVal : location of the Mask value
 * \param[out] invRes  : location to place the INVENTORY_RES
 * 
 * \return ERR_WRONG_STATE  : RFAL not initialized or incorrect mode
 * \return ERR_PARAM        : Invalid parameters
 * \return ERR_IO           : Generic internal error
 * \return ERR_RF_COLLISION : Collision detected 
 * \return ERR_CRC          : CRC error detected
 * \return ERR_PROTO        : Protocol error detected
 * \return ERR_NONE         : No error
 *****************************************************************************
 */ 
ReturnCode rfalNfcvPollerInventory( rfalNfcvNumSlots nSlots, uint8_t maskLen, uint8_t *maskVal, rfalNfcvInventoryRes *invRes );

/*! 
 *****************************************************************************
 * \brief  NFC-V Poller Full Collision Resolution
 *  
 * Performs a full Collision resolution as defined in Activity 2.0 (Candidate) 9.3.7
 *
 * \param[in]  devLimit    : device limit value, and size nfcaDevList
 * \param[out] nfcvDevList : NFC-v listener devices list
 * \param[out] devCnt      : Devices found counter
 *
 * \return ERR_WRONG_STATE  : RFAL not initialized or mode not set
 * \return ERR_PARAM        : Invalid parameters
 * \return ERR_IO           : Generic internal error
 * \return ERR_NONE         : No error
 *****************************************************************************
 */
ReturnCode rfalNfcvPollerCollisionResolution( uint8_t devLimit, rfalNfcvListenDevice *nfcvDevList, uint8_t *devCnt );

/*! 
 *****************************************************************************
 * \brief  NFC-V Poller Sleep
 *  
 * This function is used to send the SLPV_REQ (Stay Quiet) command to put the VICC 
 * with the given UID to state QUIET so that they do not reply to more Inventory
 * 
 * \param[in]  uid          : UID of the device to be put to Sleep
 *  
 * \return ERR_WRONG_STATE  : RFAL not initialized or incorrect mode
 * \return ERR_PARAM        : Invalid parameters
 * \return ERR_IO           : Generic internal error
 * \return ERR_NONE         : No error
 *****************************************************************************
 */
ReturnCode rfalNfvPollerSleep( uint8_t* uid );

#endif /* RFAL_NFCV_H */

/**
  * @}
  *
  * @}
  *
  * @}
  */


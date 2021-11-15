
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

/*! \file rfal_nfcv.c
 *
 *  \author Gustavo Patricio
 *
 *  \brief Implementation of NFC-V Poller (ISO15693) device
 *
 *  The definitions and helpers methods provided by this module are 
 *  aligned with NFC-V (ISO15693)
 *
 */

/*
 ******************************************************************************
 * INCLUDES
 ******************************************************************************
 */
#include "rfal_nfcv.h"
#include "utils.h"

/*
 ******************************************************************************
 * GLOBAL DEFINES
 ******************************************************************************
 */
#define RFAL_NFCV_INV_REQ_FLAG           0x06  /*!< INVENTORY_REQ  INV_FLAG Digital 2.0 (Candidate) 9.6.1  */
#define RFAL_NFCV_MASKVAL_MAX_LEN        8     /*!< Mask value max length: 64 bits                         */
#define RFAL_NFCV_INV_REQ_HEADER_LEN     3     /*!< INVENTORY_REQ header length (INV_FLAG, CMD, MASK_LEN)  */
#define RFAL_NFCV_INV_RES_LEN            10    /*!< INVENTORY_RES length                                   */
#define RFAL_NFCV_CRC_LEN                2     /*!< NFC-V CRC length                                       */

#define RFAL_NFCV_SLPREQ_REQ_FLAG        0x22  /*!< SLPV_REQ request flags Digital 2.0 (Candidate) 9.7.1.1 */


/*! NFC-V command set   ISO15693 9.1 */
enum 
{
    RFAL_NFCF_CMD_INVENTORY       = 0x01,      /*!< INVENTORY_REQ (Inventory )   */
    RFAL_NFCF_CMD_SLPV            = 0x02       /*!< SLPV_REQ (Stay quiet )       */    
};


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

/*! NFC-V INVENTORY_REQ format   Digital 2.0 (Candidate) 9.6.1 */
typedef struct
{
    uint8_t  INV_FLAG;                              /*!< Inventory Flags    */
    uint8_t  CMD;                                   /*!< Command code: 01h  */
    uint8_t  MASK_LEN;                              /*!< Mask Value Length  */
    uint8_t  MASK_VALUE[RFAL_NFCV_MASKVAL_MAX_LEN]; /*!< Mask Value         */
} rfalNfcvInventoryReq;


/*! NFC-V INVENTORY_REQ format   Digital 2.0 (Candidate) 9.7.1 */
typedef struct
{
    uint8_t  REQ_FLAG;                              /*!< Request Flags      */
    uint8_t  CMD;                                   /*!< Command code: 02h  */
    uint8_t  UID[RFAL_NFCV_UID_LEN];                /*!< Mask Value         */
} rfalNfcvSlpvReq;


/*
******************************************************************************
* LOCAL FUNCTION PROTOTYPES
******************************************************************************
*/

/*
******************************************************************************
* LOCAL VARIABLES
******************************************************************************
*/

/*
******************************************************************************
* GLOBAL FUNCTIONS
******************************************************************************
*/

/*******************************************************************************/
ReturnCode rfalNfcvPollerInitialize( void )
{
    ReturnCode ret;
            
    EXIT_ON_ERR( ret, rfalSetMode( RFAL_MODE_POLL_NFCV, RFAL_BR_26p48, RFAL_BR_26p48 ) );
    rfalSetErrorHandling( RFAL_ERRORHANDLING_NFC );
    
    rfalSetGT( RFAL_GT_NFCV_ADJUSTED );
    rfalSetFDTListen( RFAL_FDT_LISTEN_NFCV_POLLER );
    rfalSetFDTPoll( RFAL_FDT_POLL_NFCV_POLLER );
    
    return ERR_NONE;
}

/*******************************************************************************/
ReturnCode rfalNfcvPollerCheckPresence( rfalNfcvInventoryRes *invRes )
{
    ReturnCode ret;
    
    /* INVENTORY_REQ with 1 slot and no Mask   Activity 2.0 (Candidate) 9.2.3.32 */
    ret = rfalNfcvPollerInventory( RFAL_NFCV_NUM_SLOTS_1, 0, NULL, invRes );
    
    if( (ret == ERR_RF_COLLISION) || (ret == ERR_CRC)  || 
        (ret == ERR_FRAMING)      || (ret == ERR_PROTO)  )
    {
        ret = ERR_NONE;
    }
    
    return ret;
}

/*******************************************************************************/
ReturnCode rfalNfcvPollerInventory( rfalNfcvNumSlots nSlots, uint8_t maskLen, uint8_t *maskVal, rfalNfcvInventoryRes *invRes )
{
    uint16_t             rcvdLen;
    ReturnCode           ret;
    rfalNfcvInventoryReq invReq;
    
    if( ((maskVal == NULL) && (maskLen != 0)) || (invRes == NULL) )
    {
        return ERR_PARAM;
    }
    
    invReq.INV_FLAG = (RFAL_NFCV_INV_REQ_FLAG | nSlots);
    invReq.CMD      = RFAL_NFCF_CMD_INVENTORY;
    invReq.MASK_LEN = MIN( maskLen, rfalConvBytesToBits(RFAL_NFCV_MASKVAL_MAX_LEN) );
    ST_MEMCPY( invReq.MASK_VALUE, maskVal, rfalConvBitsToBytes(invReq.MASK_LEN) );
    
    ret = rfalISO15693TransceiveAnticollisionFrame( (uint8_t*)&invReq, (RFAL_NFCV_INV_REQ_HEADER_LEN + rfalConvBitsToBytes(invReq.MASK_LEN)), (uint8_t*)invRes, sizeof(rfalNfcvInventoryRes), &rcvdLen );
    
    if( ret == ERR_NONE )
    {
        if( rcvdLen != rfalConvBytesToBits(RFAL_NFCV_INV_RES_LEN + RFAL_NFCV_CRC_LEN) )
        {
            return ERR_PROTO;
        }
    }
    return ret;
}

/*******************************************************************************/
ReturnCode rfalNfcvPollerCollisionResolution( uint8_t devLimit, rfalNfcvListenDevice *nfcvDevList, uint8_t *devCnt )
{
    ReturnCode ret;
    
    if( nfcvDevList == NULL || devCnt == NULL )
    {
        return ERR_PARAM;
    }
    
    *devCnt = 0;
    ST_MEMSET(nfcvDevList, 0x00, (sizeof(rfalNfcvListenDevice)*devLimit) );
    
    ret = rfalNfcvPollerInventory( RFAL_NFCV_NUM_SLOTS_1, 0, NULL, &nfcvDevList->InvRes );
    if( ret == ERR_NONE )
    {
        (*devCnt)++;
    }
    
    /* REMARK: To be added  */
    return ERR_NONE;
}

/*******************************************************************************/
ReturnCode rfalNfvPollerSleep( uint8_t* uid )
{
    ReturnCode      ret;
    rfalNfcvSlpvReq slpReq;
    uint8_t         rxBuf;    /* dummy buffer, just to perform Rx */
    
    if( uid == NULL )
    {
        return ERR_PARAM;
    }
    
    /* Compute SLPV_REQ */
    slpReq.REQ_FLAG = RFAL_NFCV_SLPREQ_REQ_FLAG;
    slpReq.CMD      = RFAL_NFCF_CMD_SLPV;
    ST_MEMCPY( slpReq.UID, uid, RFAL_NFCV_UID_LEN );
    
    /* NFC Forum device SHALL wait at least FDTVpp to consider the SLPV acknowledged (FDTVpp = FDTVpoll)  Digital 2.0 (Candidate)  9.7  9.8.2  */
    ret = rfalTransceiveBlockingTxRx( (uint8_t*)&slpReq, sizeof(rfalNfcvSlpvReq), &rxBuf, sizeof(rxBuf), NULL, RFAL_TXRX_FLAGS_DEFAULT, RFAL_FDT_POLL_NFCV_POLLER );
    if( ret != ERR_TIMEOUT )
    {
        return ret;
    }
    
    return ERR_NONE;
}


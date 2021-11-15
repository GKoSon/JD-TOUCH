/*
 *****************************************************************************
 * Copyright by ams AG                                                       *
 * All rights are reserved.                                                  *
 *                                                                           *
 * IMPORTANT - PLEASE READ CAREFULLY BEFORE COPYING, INSTALLING OR USING     *
 * THE SOFTWARE.                                                             *
 *                                                                           *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS       *
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT         *
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS         *
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT  *
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,     *
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT          *
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,     *
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY     *
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT       *
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE     *
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.      *
 *****************************************************************************
 */

/*
******************************************************************************
* INCLUDES
******************************************************************************
*/
#include "st25r3911.h"
#include "mcc.h"
#include <string.h>
#include <st_errno.h>
#include "utils.h"
#include "rfal_rf.h"
#include "mcc_raw_request.h"
#include "st25r3911_com.h"
#ifdef __cplusplus
extern "C" {
#endif
/*
******************************************************************************
* DEFINES
******************************************************************************
*/

/*
******************************************************************************
* MACROS
******************************************************************************
*/

/*
******************************************************************************
* LOCAL VARIABLES
******************************************************************************
*/
static uint8_t mccRawBuffer[((MCC_BUFFER_SIZE*9)+7)/8];

/*
******************************************************************************
* LOCAL FUNCTION PROTOTYPES
******************************************************************************
*/
static uint16_t mccCopyToRawBuffer ( uint16_t *message, uint16_t length );
static uint16_t mccExtractMessage ( uint8_t* response, uint16_t responseLength );

/*
******************************************************************************
* GLOBAL FUNCTIONS
******************************************************************************
*/
ReturnCode mccSendRawRequest ( uint16_t *request,
                       uint16_t requestLength,
                       uint8_t *response,
                       uint16_t maxResponseLength,
                       uint16_t *responseLength,
                       uint16_t timeout,
                       uint8_t fourBitResponse )
{
    ReturnCode err = ERR_NONE;
 //   uint8_t lsbs_in_last_byte;
    uint16_t len_bits;
		uint16_t fdt;

    /* Setup receive operation. */
    //err = st25r3911PrepareReceive(TRUE);
		st25r3911PrepareReceive(true);
    EVAL_ERR_NE_GOTO(ERR_NONE, err, out);

    /* Mask data reception. */
    //err = st25r3911ExecuteCommand(ST25R3911_CMD_MASK_RECEIVE_DATA);
		st25r3911ExecuteCommand(ST25R3911_CMD_MASK_RECEIVE_DATA);
    EVAL_ERR_NE_GOTO(ERR_NONE, err, out);

    len_bits = mccCopyToRawBuffer(request, requestLength);

//ReturnCode st25r3911TxNBytes(const uint8_t* frame, uint16_t numbytes, uint8_t numbits, st25r3911TxFlag_t flags, uint16_t fdt )	
		fdt=rfalGetFDTListen( );
  	err = st25r3911TxNBytes(mccRawBuffer, len_bits/8, len_bits%8, ST25R3911_TX_FLAG_NONE, fdt);//ST25R3911_FDT_NONE	);
    EVAL_ERR_NE_GOTO(ERR_NONE, err, out);

    /* Receive response. */
    err = st25r3911RxNBytes(mccRawBuffer, sizeof(mccRawBuffer), responseLength, timeout);
    EVAL_ERR_NE_GOTO(ERR_NONE, err, out);

    if (((*responseLength * 8) / 9) > maxResponseLength)
    {
        //MCC_DBG("limiting l=%hx ,ml=%hx\n",*responseLength,maxResponseLength);
        *responseLength = maxResponseLength / 8 * 9;
        err = ERR_NONE; /* This will but an existing CRC */
    }
    *responseLength = mccExtractMessage(response, *responseLength);

out:
    return err;
}

/*
******************************************************************************
* LOCAL FUNCTIONS
******************************************************************************
*/
static uint16_t mccCopyToRawBuffer ( uint16_t *message, uint16_t length )
{
    int i, bytepos = 0;
    int bitpos = 0;
    memset(mccRawBuffer,0,sizeof(mccRawBuffer));
    //MCC_DBG("transmitting: ");
    for(i = 0; i<length; i++)
    {
        //MCC_DBG("%hx,",message[i]);
    }
    //MCC_DBG("\n");


    for (i = 0; i < length; i++)
    {
        uint16_t m = message[i];
        mccRawBuffer[bytepos] |= (m & ((1<<(8 - bitpos))-1)) << bitpos;
        bytepos++;
        mccRawBuffer[bytepos] |= (m >> (8-bitpos));

        bitpos += 1;
        if (bitpos >=8 )
        {
            bitpos -= 8;
            bytepos++;
        }
    }
    //MCC_DBG("  raw: ");
    for ( i= 0; i< ((length*9)+7)/8;i++)
    {
        //MCC_DBG("%hhx,",mccRawBuffer[i]);
    }
    //MCC_DBG("\n");
    return length*9;
}

static uint16_t mccExtractMessage ( uint8_t* response, uint16_t responseLength )
{
    int bytes = responseLength * 8 / 9;
    int i, bytepos = 0;
    int bitpos = 0;
    //MCC_DBG("extracting (%d bytes)", responseLength);
    //MCC_DUMP(mccRawBuffer,responseLength);
    if (responseLength==1)
    {
        response[0] = mccRawBuffer[0];
        return 1;
    }
    for (i = 0; i < bytes; i++)
    {
        uint8_t m;
        m = (mccRawBuffer[bytepos] >> bitpos);
        bytepos++;
        m |= (mccRawBuffer[bytepos] << (8-bitpos));

        bitpos += 1;
        if (bitpos >=8 )
        {
            bitpos -= 8;
            bytepos++;
        }

        response[i] = m;
    }
    //MCC_DBG(" extracted: (%d bytes)", bytes);
    //MCC_DUMP(response,bytes);
    return bytes;
}

#ifdef __cplusplus
}
#endif

/** \file
    ---------------------------------------------------------------
    SPDX-License-Identifier: BSD-3-Clause

    Copyright (c) 2026, Renesas Electronics Corporation and/or its affiliates


    Redistribution and use in source and binary forms, with or without modification,
    are permitted provided that the following conditions are met:

    1. Redistributions of source code must retain the above copyright notice, this
       list of conditions and the following disclaimer.

    2. Redistributions in binary form must reproduce the above copyright notice, this list of
       conditions and the following disclaimer in the documentation and/or other
       materials provided with the distribution.

    3. Neither the name of Renesas nor the names of its
       contributors may be used to endorse or promote products derived from this
       software without specific prior written permission.



    THIS SOFTWARE IS PROVIDED BY Renesas "AS IS" AND ANY EXPRESS
    OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
    OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL RENESAS OR CONTRIBUTORS BE
    LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
    CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
    GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
    HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
    LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
    OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
    ---------------------------------------------------------------

    Project     : PTX2xxR/W
    Module      : High Level TDC Protocol
    File        : EXAMPLES/ptx_Example_WLC/ptx_RUL_WLC_TDC_Helper.h

    Description : Example implementation of a very light weight high level protocol on top
                  of the TDC.
*/


/**
 * \addtogroup RUL_WLC SDK_EXAMPLES.
 *
 * @{
 */

#if !defined(_PTX_SDK_EXAMPLE_RUL_WLC_TDC_HELPER_H_)
#define _PTX_SDK_EXAMPLE_RUL_WLC_TDC_HELPER_H_

#if defined(__cplusplus)
extern "C" {
#endif


/*
 * ####################################################################################################################
 * INCLUDES
 * ####################################################################################################################
 */

#include "ptx_RUL_WLC.h"
#include "ptx_Status.h"

/*
 * ####################################################################################################################
 * DEFINES / CONSTANTS
 * ####################################################################################################################
 */

/*
 * ####################################################################################################################
 * TYPES
 * ####################################################################################################################
 */

/*
 * ####################################################################################################################
 * API FUNCTIONS
 * ####################################################################################################################
 */

/**
 * \brief Example implementation of a very simple and light weight protocol on top of the Transparent Data Channel (TDC)
 *         to receive larger amounts of data than 63 bytes. The protocol implements an initial handshake and chunks
 *         the data into frames of 62 bytes of payload.
 *
 *
 * \param[in]  ctx         Pointer to WLC Component.
 * \param[in]  rxData      Pointer to store the received data.
 * \param[in]  rxDataLen   Size (= amount of bytes) of the buffer
 *
 * \return Status, indicating whether the operation was successful. See \ref ptx_Status_t.
 */
ptx_Status_t ptx_RUL_WLC_TDC_HlpRx(ptx_RUL_WLC_t *ctx, uint8_t *rxData, uint32_t *rxDataLen);


/**
 * \brief Example implementation of a very simple and light weight protocol on top of the Transparent Data Channel (TDC)
 *         to send larger amounts of data than 63 bytes. The protocol implements an initial handshake and chunks
 *         the data into frames of 62 bytes of payload.
 *
 *
 * \param[in]  ctx         Pointer to WLC Component.
 * \param[in]  txData      Pointer to a buffer to be transmitted to the listener.
 * \param[in]  txDataLen   Size (= amount of bytes) to be transmitted.
 *
 * \return Status, indicating whether the operation was successful. See \ref ptx_Status_t.
 */
ptx_Status_t ptx_RUL_WLC_TDC_HlpTx(ptx_RUL_WLC_t *ctx, uint8_t *txData, uint32_t txDataLen);

#if defined(__cplusplus)
}
#endif

#endif /* Guard */

/** @} */


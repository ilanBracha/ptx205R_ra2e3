/*
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
    Module      : HCE T3T
    File        : ptx_HCET3T.h

    Description : Implementation of Host Card Emulation (HCE) for T3T. This enables the emulation of the
                  PTX2xxR/W device as a T3T Card.
*/

/**
 * \addtogroup HCE_T3T
 * @{
 */

#if !defined(PTX_HCET3T_H)
#define PTX_HCET3T_H

/*
 * ####################################################################################################################
 * INCLUDES
 * ####################################################################################################################
 */

#include <stddef.h>

#include "ptx_Components.h"
#include "ptx_Status.h"


#if defined(__cplusplus)
extern "C" {
#endif


/*
 * ####################################################################################################################
 * DEFINES
 * ####################################################################################################################
 */

#define PTX_HCET3T_NFCID2_LEN                   (uint8_t)8          /**< Fixed T3T NFCID2 Length */
#define PTX_HCET3T_PMM_LEN                      (uint8_t)8          /**< Fixed T3T PMm Length */

/*
 * ####################################################################################################################
 * TYPES
 * ####################################################################################################################
 */

/**
 * \brief T3T Card Initialisation Parameters.
 */
typedef struct ptx_HCET3T_InitParams
{
    uint8_t                   *NDEFFile;
    uint32_t                  NDEFFileSize;
    uint8_t                   *Nfcid;
    uint32_t                  NfcidLength;
} ptx_HCET3T_InitParams_t;

/**
 * \brief T3T Card Parameters.
 */
typedef struct ptx_HCET3T
{
    ptx_Comps_t             CompId;

    uint8_t                 Nfcid[PTX_HCET3T_NFCID2_LEN];
    uint8_t                 *NDEFFile;
    uint32_t                NDEFFileSize;
} ptx_HCET3T_t;

/*
 * ####################################################################################################################
 * API FUNCTIONS
 * ####################################################################################################################
 */
/**
 * \brief HCE T3T Initialisation.
 *
 * \param[in] T3T               T3T Card Context.
 * \param[in] initParams        Initialisation Parameters.
 *
 * \return PTX_STATUS_SUCCESS                    Function executed successfully.
 * \return PTX_STATUS_E_GEN_INVALID_PARAMETER    Invalid input parameter.
 * \return Others                                An error occurred in lower layers. See \ref ptx_Status_t.
 */
ptx_Status_t ptx_HCET3T_Init(ptx_HCET3T_t *T3T, ptx_HCET3T_InitParams_t *T3TInitParams);

/**
 * \brief HCE T3T Deinitialization.
 *
 * \param[in] T3T               T3T Card Context.
 *
 * \return PTX_STATUS_SUCCESS                    Function executed successfully.
 * \return PTX_STATUS_E_GEN_INVALID_PARAMETER    Invalid input parameter.
 * \return Others                                An error occurred in lower layers. See \ref ptx_Status_t.
 */
ptx_Status_t ptx_HCET3T_DeInit(ptx_HCET3T_t *T3T);

/**
 * \brief Process a received T3T command.
 *
 * \param[in]     T3T           T3T Card Context.
 * \param[in]     rx            Received Data.
 * \param[in]     rxLen         Length of received Data.
 * \param[out]    tx            Data to be Sent.
 * \param[in,out] txLen         Length of Tx buffer as input, Length of Data to be sent as output.
 *
 * \return PTX_STATUS_SUCCESS                    Function executed successfully.
 * \return PTX_STATUS_E_GEN_INVALID_PARAMETER    Invalid input parameter.
 * \return Others                                An error occurred in lower layers. See \ref ptx_Status_t.
 */
ptx_Status_t ptx_HCET3T_CommandProcess(ptx_HCET3T_t *T3T, uint8_t *rx, uint32_t rxLen, uint8_t *tx, uint32_t *txLen);
#if defined(__cplusplus)
}
#endif

#endif /* Guard */

/** @} */


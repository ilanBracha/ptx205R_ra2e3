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
    File        : ptx_HCET3T.c

    Description : Implementation of Host Card Emulation (HCE) for T3T. This enables the emulation of the
                  PTX2xxR/W device as a T3T Card.
*/

/*
 * ####################################################################################################################
 * INCLUDES
 * ####################################################################################################################
 */

#include <string.h>
#include <stdint.h>
#include "ptx_HCET3T.h"

/*
 * ####################################################################################################################
 * DEFINES / TYPES
 * ####################################################################################################################
 */


#define PTX_HCET3T_FILE_SIZE                    (uint16_t)224       /**< Fixed File size 13 Blocks data + 1 Block Attribute Information */
#define PTX_HCET3T_BLOCK_SIZE                   (uint8_t)16         /**< Fixed T3T Block size */

#define PTX_HCET3T_ATTRIBUTE_INF_OFFSET         (uint8_t)0          /**< T3T Attribute Information offset in NDEF File */
#define PTX_HCET3T_NDEF_MSG_OFFSET              (uint8_t)16         /**< T3T NDEF Message offset in NDEF File */

#define PTX_HCET3T_SC_NDEF                      (uint16_t)0x12FC    /**< T3T System Code for NDEF. */
#define PTX_HCET3T_READ_NDEF_SC                 (uint16_t)0x000B    /**< T3T Service Code for NDEF Read. */
#define PTX_HCET3T_WRITE_NDEF_SC                (uint16_t)0x0009    /**< T3T Service Code for NDEF Write. */

#define PTX_HCET3T_OPCODE_OFFSET                (uint8_t)1          /**< T3T Command OPCODE Offset */
#define PTX_HCET3T_OPCODE_CHECK                 (uint8_t)0x06       /**> T3T Check OPCODE */
#define PTX_HCET3T_OPCODE_UPDATE                (uint8_t)0x08       /**> T3T Update OPCODE */
#define PTX_HCET3T_OPCODE_SENSF_REQ             (uint8_t)0x00       /**> T3T SensF Req OPCODE */

#define PTX_HCET3T_CHECK_RES_OPCODE             (uint8_t)0x07       /**< T3T Check Response opcode */
#define PTX_HCET3T_UPDATE_RES_OPCODE            (uint8_t)0x09       /**< T3T Update Response opcode */
#define PTX_HCET3T_SENSF_RES_OPCODE             (uint8_t)1          /**< T3T SensF Response opcode */

#define PTX_HCET3T_CHECK_NFCID2_OFFSET          (uint8_t)2          /**< T3T CHECK command NFCID2 offset */
#define PTX_HCET3T_CHECK_NOS_OFFSET             (uint8_t)10         /**< T3T CHECK command NOS offset */
#define PTX_HCET3T_CHECK_SERVICE_LO_OFFSET      (uint8_t)11         /**< T3T CHECK command Service Code Low byte offset */
#define PTX_HCET3T_CHECK_SERVICE_HI_OFFSET      (uint8_t)12         /**< T3T CHECK command Service Code High byte offset */
#define PTX_HCET3T_CHECK_NOB_OFFSET             (uint8_t)13         /**< T3T CHECK command NOB offset */
#define PTX_HCET3T_CHECK_BLOCK_LIST_OFFSET      (uint8_t)14         /**< T3T CHECK command Block List offset */
#define PTX_HCET3T_CHECK_MAX_NOS_SUPPORTED      (uint8_t)1          /**< T3T CHECK command maximum NOS supported */

#define PTX_HCET3T_STATUS_FLAG1_SUCCESS         (uint8_t)0          /**< T3T Check Response Status Flag 1 for successful command */
#define PTX_HCET3T_STATUS_FLAG1_ERROR           (uint8_t)0xFF       /**< T3T Check Response Status Flag 1 for error */
#define PTX_HCET3T_STATUS_FLAG2_SUCCESS         (uint8_t)0          /**< T3T Check Response Status Flag 2 for successful command */
#define PTX_HCET3T_STATUS_FLAG2_ERROR           (uint8_t)0xFF       /**< T3T Check Response Status Flag 2 for error */
#define PTX_HCET3T_CHECK_RES_FIXED_SIZE         (uint8_t)12         /**< T3T Check Response fixed size (Length + opcode + NFCID2 + Status flag) */
#define PTX_HCET3T_CHECK_RES_NOB_SIZE           (uint8_t)1          /**< T3T Check Response nob sizes */
#define PTX_HCET3T_UPDATE_RES_SIZE              (uint8_t)12         /**< T3T Update Response size (Length + opcode + NFCID2 + Status flag) */
#define PTX_HCET3T_ERROR_RES_SIZE               (uint8_t)4          /**< T3T General Error Response size */

#define PTX_HCET3T_SENSF_RES_FIXED_SIZE         (uint8_t)18         /**< T3T SensF Response fixed size (length + opcode + NFCID2 + manufacture parameter (PMm)) */
#define PTX_HCET3T_SENSF_RES_RC_SIZE            (uint8_t)2          /**< T3T SensF Response RC field size */
#define PTX_HCET3T_SENSF_REQ_RC_OFFSET          (uint8_t)4          /**< T3T SensF Request RC offset */
#define PTX_HCET3T_SENSF_REQ_RC_SC              (uint8_t)1          /**< T3T SensF Request RC value for system code requested */
#define PTX_HCET3T_SENSF_REQ_RC_AP              (uint8_t)2          /**< T3T SensF Request RC value for advanced protocol features requested */
#define PTX_HCET3T_SENSF_RES_RC_AP_UNSUPPORTED  (uint8_t)0          /**< T3T SensF Response RC value for advanced protocol features unsupported */

/*
 * Default defines.
 */
static uint8_t T3TndefFile[PTX_HCET3T_FILE_SIZE];  /**< Default NDEF File. */

static uint8_t Default_Nfcid[PTX_HCET3T_NFCID2_LEN] = { 0x02, 0xFE, 0x80, 0x40, 0x30, 0x20, 0x10, 0x00 };

/*
 * ####################################################################################################################
 * INTERNAL FUNCTIONS DECLARATIONS / HELPERS
 * ####################################################################################################################
 */
static ptx_Status_t ptx_HCET3T_SetupNDEFFile(uint8_t* ndefFile, uint32_t ndefFileSize);
static ptx_Status_t ptx_HCET3T_HandleCheck(ptx_HCET3T_t *T3T, uint8_t *rx, uint32_t rxLen, uint8_t *tx, uint32_t *txLen);
static ptx_Status_t ptx_HCET3T_HandleSensFRequest(ptx_HCET3T_t *T3T, uint8_t *rx, uint32_t rxLen, uint8_t *tx, uint32_t *txLen);
static ptx_Status_t ptx_HCET3T_HandleUnsupportedCommand(ptx_HCET3T_t *T3T, uint8_t *rx, uint32_t rxLen, uint8_t *tx, uint32_t *txLen);
static ptx_Status_t ptx_HCET3T_HandleUpdate(ptx_HCET3T_t *T3T, uint8_t *rx, uint32_t rxLen, uint8_t *tx, uint32_t *txLen);
static ptx_Status_t ptx_HCET3T_ConstructUpdateResponse(ptx_HCET3T_t *T3T, uint8_t *rx, uint32_t rxLen, uint8_t *tx, uint32_t *txLen);
static ptx_Status_t ptx_HCET3T_ConstructCheckResponse(ptx_HCET3T_t *T3T, uint8_t *rx, uint32_t rxLen, uint8_t *tx, uint32_t *txLen);

/*
 * ####################################################################################################################
 * API FUNCTIONS
 * ####################################################################################################################
 */
ptx_Status_t ptx_HCET3T_Init(ptx_HCET3T_t *T3T, ptx_HCET3T_InitParams_t *T3TInitParams)
{
    ptx_Status_t status = PTX_STATUS_E_GEN_INVALID_PARAMETER;

    if (NULL != T3T)
    {
        if (NULL != T3TInitParams)
        {

            if ((NULL != T3TInitParams->NDEFFile) && (0 != T3TInitParams->NDEFFileSize))
            {
                T3T->NDEFFile = T3TInitParams->NDEFFile;
                T3T->NDEFFileSize = T3TInitParams->NDEFFileSize;

                status = PTX_STATUS_SUCCESS;
            }
            else
            {
                status = ptx_HCET3T_SetupNDEFFile(T3TndefFile, PTX_HCET3T_FILE_SIZE);

                if (PTX_STATUS_SUCCESS == status)
                {
                    T3T->NDEFFile = &T3TndefFile[0];
                    T3T->NDEFFileSize = PTX_HCET3T_FILE_SIZE;
                }
            }

            if ((NULL != T3TInitParams->Nfcid) && (PTX_HCET3T_NFCID2_LEN == T3TInitParams->NfcidLength))
            {
                memcpy(T3T->Nfcid, T3TInitParams->Nfcid, PTX_HCET3T_NFCID2_LEN);
            }
            else
            {
                memcpy(T3T->Nfcid, Default_Nfcid, PTX_HCET3T_NFCID2_LEN);
            }
        }
        else
        {
            status = ptx_HCET3T_SetupNDEFFile(T3TndefFile, PTX_HCET3T_FILE_SIZE);

            if (PTX_STATUS_SUCCESS == status)
            {
                T3T->NDEFFile = &T3TndefFile[0];
                T3T->NDEFFileSize = PTX_HCET3T_FILE_SIZE;
            }

            memcpy(T3T->Nfcid, Default_Nfcid, PTX_HCET3T_NFCID2_LEN);
        }

        T3T->CompId = Comp_HCE;
    }

    return status;
}

ptx_Status_t ptx_HCET3T_DeInit(ptx_HCET3T_t *T3T)
{
    ptx_Status_t status = PTX_STATUS_E_GEN_INVALID_PARAMETER;

    if (PTX_COMP_CHECK(T3T, Comp_HCE))
    {
        memset(T3T, 0, sizeof(ptx_HCET3T_t));
        status = PTX_STATUS_SUCCESS;
    }

    return status;
}

ptx_Status_t ptx_HCET3T_CommandProcess(ptx_HCET3T_t *T3T, uint8_t *rx, uint32_t rxLen, uint8_t *tx, uint32_t *txLen)
{
    ptx_Status_t status = PTX_STATUS_E_GEN_INVALID_PARAMETER;

    if (PTX_COMP_CHECK(T3T, Comp_HCE) && (NULL != rx) && (0 != rxLen) && (NULL != tx) && (NULL != txLen))
    {
        switch (rx[PTX_HCET3T_OPCODE_OFFSET])
        {
            case PTX_HCET3T_OPCODE_CHECK:
                status = ptx_HCET3T_HandleCheck(T3T, rx, rxLen, tx, txLen);
                break;
            case PTX_HCET3T_OPCODE_UPDATE:
                status = ptx_HCET3T_HandleUpdate(T3T, rx, rxLen, tx, txLen);
                break;
            case PTX_HCET3T_OPCODE_SENSF_REQ:
                status = ptx_HCET3T_HandleSensFRequest(T3T, rx, rxLen, tx, txLen);
                break;
            default:
                status = ptx_HCET3T_HandleUnsupportedCommand(T3T, rx, rxLen, tx, txLen);
                break;
        }
    }

    return status;
}


/*
 * ####################################################################################################################
 * INTERNAL FUNCTIONS / CALLBACK
 * ####################################################################################################################
 */


static ptx_Status_t ptx_HCET3T_SetupNDEFFile(uint8_t* ndefFile, uint32_t ndefFileSize)
{
    /* Attribute information block according to T3T specification */
    uint8_t attribInformation[PTX_HCET3T_BLOCK_SIZE] = {0x10, 0x04, 0x01, 0x00, 0x0d, 0x00, 0x00, 0x00,
                                                        0x00, 0x00, 0x01, 0x00, 0x00, 0x10, 0x00, 0x33
                                                       };

    /* NDEF message https://www.renesas.com */
    uint8_t initNDEFMessage[PTX_HCET3T_BLOCK_SIZE] = {  0xd1, 0x01, 0x0c, 0x55, 0x02, 0x72, 0x65, 0x6e,
                                                        0x65, 0x73, 0x61, 0x73, 0x2e, 0x63, 0x6F, 0x6D
                                                     };

    memset(ndefFile, 0x00, sizeof(uint8_t) * ndefFileSize);

    memcpy(&ndefFile[PTX_HCET3T_ATTRIBUTE_INF_OFFSET], attribInformation, sizeof(uint8_t) * PTX_HCET3T_BLOCK_SIZE);
    memcpy(&ndefFile[PTX_HCET3T_NDEF_MSG_OFFSET], initNDEFMessage, sizeof(uint8_t) * PTX_HCET3T_BLOCK_SIZE);

    return PTX_STATUS_SUCCESS;
}


static ptx_Status_t ptx_HCET3T_HandleCheck(ptx_HCET3T_t *T3T, uint8_t *rx, uint32_t rxLen, uint8_t *tx, uint32_t *txLen)
{
    ptx_Status_t status = PTX_STATUS_E_GEN_INVALID_PARAMETER;

    if (PTX_COMP_CHECK(T3T, Comp_HCE) && (NULL != rx) && (0 != rxLen) && (NULL != tx) && (NULL != txLen))
    {
        if (PTX_HCET3T_CHECK_MAX_NOS_SUPPORTED >= rx[PTX_HCET3T_CHECK_NOS_OFFSET])
        {
            // Convert Service code from little endian
            uint16_t service_code = (uint16_t)(((uint16_t)(rx[PTX_HCET3T_CHECK_SERVICE_HI_OFFSET] << 8)) | rx[PTX_HCET3T_CHECK_SERVICE_LO_OFFSET]);

            if (PTX_HCET3T_READ_NDEF_SC == service_code || PTX_HCET3T_WRITE_NDEF_SC == service_code)
            {
                status = ptx_HCET3T_ConstructCheckResponse(T3T, rx, rxLen, tx, txLen);
            }
            else
            {
                // Construct error response
                uint8_t index = 0;
                uint8_t length = PTX_HCET3T_CHECK_RES_FIXED_SIZE;
                tx[index] = length;
                index++;

                tx[index] = PTX_HCET3T_CHECK_RES_OPCODE;
                index++;

                memcpy(&tx[index], &T3T->Nfcid[0], sizeof(uint8_t) * PTX_HCET3T_NFCID2_LEN);
                index += PTX_HCET3T_NFCID2_LEN;

                tx[index] = PTX_HCET3T_STATUS_FLAG1_ERROR;
                index++;
                tx[index] = PTX_HCET3T_STATUS_FLAG2_ERROR;
                index++;

                *txLen = length;

                status = PTX_STATUS_SUCCESS;
            }
        }
        else
        {
            status = PTX_STATUS_E_GEN_NOT_AVAILABLE;
        }


        status = PTX_STATUS_SUCCESS;
    }

    return status;
}

static ptx_Status_t ptx_HCET3T_ConstructCheckResponse(ptx_HCET3T_t *T3T, uint8_t *rx, uint32_t rxLen, uint8_t *tx, uint32_t *txLen)
{
    ptx_Status_t status = PTX_STATUS_E_GEN_INVALID_PARAMETER;

    if (PTX_COMP_CHECK(T3T, Comp_HCE) && (NULL != rx) && (0 != rxLen) && (NULL != tx) && (NULL != txLen))
    {
        uint8_t number_blocks = rx[PTX_HCET3T_CHECK_NOB_OFFSET];

        if (((uint32_t)(number_blocks * PTX_HCET3T_BLOCK_SIZE + PTX_HCET3T_CHECK_RES_FIXED_SIZE + PTX_HCET3T_CHECK_RES_NOB_SIZE)) < *txLen)
        {
            uint8_t index = 0;
            tx[index] = ((uint8_t)(number_blocks * PTX_HCET3T_BLOCK_SIZE + PTX_HCET3T_CHECK_RES_FIXED_SIZE + PTX_HCET3T_CHECK_RES_NOB_SIZE));
            index++;

            tx[index] = PTX_HCET3T_CHECK_RES_OPCODE;
            index++;

            memcpy(&tx[index], &T3T->Nfcid[0], sizeof(uint8_t) * PTX_HCET3T_NFCID2_LEN);
            index += PTX_HCET3T_NFCID2_LEN;

            tx[index] = PTX_HCET3T_STATUS_FLAG1_SUCCESS;
            index++;
            tx[index] = PTX_HCET3T_STATUS_FLAG2_SUCCESS;
            index++;

            tx[index] = number_blocks;
            index++;

            for (uint32_t i = 0; i < number_blocks; i++)
            {
                uint8_t block_list_index = (uint8_t)(i * 2 + 1);
                uint8_t block_index = PTX_HCET3T_BLOCK_SIZE * rx[PTX_HCET3T_CHECK_BLOCK_LIST_OFFSET + block_list_index];

                memcpy(&tx[index], &T3TndefFile[block_index], sizeof(uint8_t) * PTX_HCET3T_BLOCK_SIZE);
                index += PTX_HCET3T_BLOCK_SIZE;
            }

            *txLen = index;

            status = PTX_STATUS_SUCCESS;

        }
        else
        {
            status = PTX_STATUS_E_GEN_INSUFFICIENT_RESOURCES;
        }

    }

    return status;

}

static ptx_Status_t ptx_HCET3T_HandleUpdate(ptx_HCET3T_t *T3T, uint8_t *rx, uint32_t rxLen, uint8_t *tx, uint32_t *txLen)
{
    ptx_Status_t status = PTX_STATUS_E_GEN_INVALID_PARAMETER;

    if (PTX_COMP_CHECK(T3T, Comp_HCE) && (NULL != rx) && (0 != rxLen) && (NULL != tx) && (NULL != txLen))
    {

        if (PTX_HCET3T_CHECK_MAX_NOS_SUPPORTED >= rx[PTX_HCET3T_CHECK_NOS_OFFSET])
        {
            // Convert Service code from little endian
            uint16_t service_code = (uint16_t)(((uint16_t)(rx[PTX_HCET3T_CHECK_SERVICE_HI_OFFSET] << 8)) | rx[PTX_HCET3T_CHECK_SERVICE_LO_OFFSET]);

            if (PTX_HCET3T_WRITE_NDEF_SC == service_code)
            {
                uint8_t number_blocks = rx[PTX_HCET3T_CHECK_NOB_OFFSET];

                for (uint32_t i = 0; i < number_blocks; i++)
                {
                    uint8_t block_list_index = (uint8_t)(i * 2 + 1);
                    uint8_t block_index = PTX_HCET3T_BLOCK_SIZE * rx[PTX_HCET3T_CHECK_BLOCK_LIST_OFFSET + block_list_index];

                    uint8_t rx_block_data_idx = (uint8_t)(PTX_HCET3T_CHECK_BLOCK_LIST_OFFSET + (uint8_t)(number_blocks * 2) + (uint8_t)(i * PTX_HCET3T_BLOCK_SIZE));


                    memcpy(&T3TndefFile[block_index], &rx[rx_block_data_idx], sizeof(uint8_t) * PTX_HCET3T_BLOCK_SIZE);
                }

                status = ptx_HCET3T_ConstructUpdateResponse(T3T, rx, rxLen, tx, txLen);
            }
            else
            {
                status = PTX_STATUS_E_GEN_INVALID_STATE;
            }

        }
        else
        {
            status = PTX_STATUS_E_GEN_NOT_AVAILABLE;
        }


        return status;
    }

    return status;
}

static ptx_Status_t ptx_HCET3T_ConstructUpdateResponse(ptx_HCET3T_t *T3T, uint8_t *rx, uint32_t rxLen, uint8_t *tx, uint32_t *txLen)
{
    ptx_Status_t status = PTX_STATUS_E_GEN_INVALID_PARAMETER;

    if (PTX_COMP_CHECK(T3T, Comp_HCE) && (NULL != rx) && (0 != rxLen) && (NULL != tx) && (NULL != txLen))
    {

        if (PTX_HCET3T_UPDATE_RES_SIZE < *txLen)
        {
            uint8_t index = 0;
            tx[index] = PTX_HCET3T_UPDATE_RES_SIZE;
            index++;

            tx[index] = PTX_HCET3T_UPDATE_RES_OPCODE;
            index++;

            memcpy(&tx[index], &T3T->Nfcid[0], sizeof(uint8_t) * PTX_HCET3T_NFCID2_LEN);
            index += PTX_HCET3T_NFCID2_LEN;

            tx[index] = PTX_HCET3T_STATUS_FLAG1_SUCCESS;
            index++;
            tx[index] = PTX_HCET3T_STATUS_FLAG2_SUCCESS;
            index++;

            *txLen = index;

            status = PTX_STATUS_SUCCESS;
        }
        else
        {
            status = PTX_STATUS_E_GEN_INSUFFICIENT_RESOURCES;
        }

    }

    return status;

}

static ptx_Status_t ptx_HCET3T_HandleSensFRequest(ptx_HCET3T_t *T3T, uint8_t *rx, uint32_t rxLen, uint8_t *tx, uint32_t *txLen)
{
    ptx_Status_t status = PTX_STATUS_E_GEN_INVALID_PARAMETER;

    if (PTX_COMP_CHECK(T3T, Comp_HCE) && (NULL != rx) && (0 != rxLen) && (NULL != tx) && (NULL != txLen))
    {
        if (rxLen >= PTX_HCET3T_SENSF_REQ_RC_OFFSET)
        {
            uint8_t index = 0;
            uint8_t length = PTX_HCET3T_SENSF_RES_FIXED_SIZE;

            uint8_t req_rc = rx[PTX_HCET3T_SENSF_REQ_RC_OFFSET];
            if (req_rc != 0)
            {
                /* Additional information requested */
                length += PTX_HCET3T_SENSF_RES_RC_SIZE;
            }

            tx[index] = length;
            index++;

            tx[index] = PTX_HCET3T_SENSF_RES_OPCODE;
            index++;

            memcpy(&tx[index], &T3T->Nfcid[0], sizeof(uint8_t) * PTX_HCET3T_NFCID2_LEN);
            index += PTX_HCET3T_NFCID2_LEN;

            const uint8_t PMm[PTX_HCET3T_PMM_LEN] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00 };
            memcpy(&tx[index], &PMm[0], sizeof(uint8_t) * PTX_HCET3T_PMM_LEN);
            index += PTX_HCET3T_PMM_LEN;

            // Append additional information if requested.
            if (req_rc == PTX_HCET3T_SENSF_REQ_RC_SC)
            {
                tx[index] = (PTX_HCET3T_SC_NDEF >> 8) & 0xFF;
                index++;

                tx[index] = (PTX_HCET3T_SC_NDEF & 0xFF);
                index++;
            }
            else if (req_rc == PTX_HCET3T_SENSF_REQ_RC_AP)
            {
                tx[index] = PTX_HCET3T_SENSF_RES_RC_AP_UNSUPPORTED;
                index++;

                tx[index] = PTX_HCET3T_SENSF_RES_RC_AP_UNSUPPORTED;
                index++;
            }

            *txLen = length;

            status = PTX_STATUS_SUCCESS;
        }
    }

    return status;
}

static ptx_Status_t ptx_HCET3T_HandleUnsupportedCommand(ptx_HCET3T_t *T3T, uint8_t *rx, uint32_t rxLen, uint8_t *tx, uint32_t *txLen)
{
    ptx_Status_t status = PTX_STATUS_E_GEN_INVALID_PARAMETER;

    if (PTX_COMP_CHECK(T3T, Comp_HCE) && (NULL != rx) && (0 != rxLen) && (NULL != tx) && (NULL != txLen))
    {
        uint8_t index = 0;
        uint8_t length = PTX_HCET3T_ERROR_RES_SIZE;

        tx[index] = length;
        index++;

        tx[index] = rx[PTX_HCET3T_OPCODE_OFFSET] + 1;
        index++;
        tx[index] = 0x00;
        index++;
        tx[index] = 0x00;
        index++;

        *txLen = length;
        status = PTX_STATUS_SUCCESS;
    }

    return status;
}
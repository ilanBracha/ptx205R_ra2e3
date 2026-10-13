/**
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
    Module      : SDK Example (ISO Loopback)
    File        : EXAMPLES/ptx_Example_ISOLoopback/ptx_ExampleApplication.h

    Description : This application demonstrates the use of the ISO Loopback
                  application using the Reader Universal Library (RUL) and the HCE add-on API.
                  The application is used for compliance / type-approval testing.
*/

/**
 * \addtogroup SDK_EXAMPLES
 *
 * @{
 */

#if !defined(_PTX_SDK_EXAMPLE_ISO_LOOPBACK_H_)
#define _PTX_SDK_EXAMPLE_ISO_LOOPBACK_H_

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * ####################################################################################################################
 * INCLUDES
 * ####################################################################################################################
 */

/*
 * ####################################################################################################################
 * DEFINES / CONSTANTS
 * ####################################################################################################################
 */

/**
 * \brief Version number of current Demo-Application.
 */
const char *PTX_DEMO_VERSION = "V1.0";

#define PTX_DEMO_APP_BUFFER_SIZE          4096
#define PTX_DEMO_APP_MIN_APDU_LENGTH         5

/*
 * ####################################################################################################################
 * TYPES
 * ####################################################################################################################
 */

typedef enum ptx_DemoAppState
{
    DemoAppState_StartDiscovery,       /**< Start NFC Forum Discovery / Polling-Loop. */
    DemoAppState_WaitForDiscovery,     /**< Wait until Polling-Loop discovered, resolved and potentially also activated available card(s). */
    DemoAppState_DataExchange,         /**< Perform example data exchange for every activated card using standard RUL API Exchange-functions. */
    DemoAppState_Deactivate,           /**< Deactivate Reader i.e. switch RF-Field off. */
    DemoAppState_Exit,                 /**< Stop Demo-Application (no specific reason). */
    DemoAppState_SystemError,          /**< System Error */

    DemoAppState_PollHCEEvent,         /**< Poll for new HCE events */
    DemoAppState_HandleHCEEvent,       /**< Handle received HCE event */
    DemoAppState_HandleHCEData,        /**< Handle HCE Data Event and send according response */
} ptx_DemoAppState_t;

typedef enum ptx_DemoAppISOSequence
{
    ISO_DEFAULT_TEST_COMMAND_SEQUENCE,
    ISO_TEST_COMMAND_SEQUENCE_1,
    ISO_TEST_COMMAND_SEQUENCE_2
} ptx_DemoAppISOSequence_t;

typedef enum ptx_DemoAppISOLoopbackMode
{
    ISO_LOOPBACK_PICC,
    ISO_LOOPBACK_PCD
} ptx_DemoAppISOLoopbackMode_t;

/*
 * ####################################################################################################################
 * API FUNCTIONS
 * ####################################################################################################################
 */

#if defined(PTX_PLATFORM_MCU)
int ptx_App_Entry(void);
#endif

#if defined(__cplusplus)
}
#endif

#endif /* Guard */

/** @} */


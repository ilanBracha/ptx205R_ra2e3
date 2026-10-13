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
    Module      : SDK Example (NFC Forum NDEF API Add-On API)
    File        : EXAMPLES/ptx_Example_NFCForumNDEF/ptx_ExampleApplication.h

    Description : This application example demonstrates how to
                  - setup and start the basic NFC Forum RF-Discovery loop,
                  - retrieve Card/Tag parameters from the Card-registry,
                  - check and read NDEF-messages from the various NFC Forum Type Tags 2 - 5
                  using the NDEF Add-on API. Typical use cases for the usage of NDEF operations are all
                  types NFC Forum applications such as NFC Forum Wireless Charging (WLC), sending Text / URLs,
                  Bluetooth / WiFi pairing and many more.
                  This example shows exclusively how to read NDEF messages from an NFC Forum Type Tag x.
                  The NDEF API also supports writing of NDEF messages and creation of various NDEF records types.
                  How to use both features is shown in the NFC WLC Poller application.

                  The NDEF add-on API supports two types of NDEF operations:
                    1. "General Tag"-based NDEF operations and
                       Abstracts all NDEF operations for every NFC Forum Type Tag and is agnostic to the used NFC
                       RF-technology and RF-protocol. The API automatically detects which NFC Forum Type Tag is
                       currently active and invokes the proper methods from the "Type Tag"-based NDEF operations (see below).
                       The "General Tag"-based NDEF API is targeted for applications where memory is less of a concern and
                       where the application wants to support all NFC Forum Type Tags without having to deal with which
                       Tag and RF-technology/-protocol is currently being used.
                    2. "Type Tag"-based NDEF operations.
                       These operations work directly on a given NFC Forum Type Tag 2/3/4/5 and can be used to
                       exactly tailor an NDEF application using only the NDEF API needed for the intended scenario.
                       For example, an application knows that it only uses NFC Forum Type Tag 2, then the application
                       only needs to include the T2T NDEF API, but can exclude all others including the "General Tag"-based
                       NDEF API to safe memory.
                       The "Type Tag"-based NDEF API provides the same set of functions like the "General Tag"-based API, but
                       is intended for applications where memory is a concern and high level of customization is required with
                       respect to support for Type Tag 2/3/4/5.
*/

/**
 * \addtogroup SDK_EXAMPLES
 *
 * @{
 */

#if !defined(_PTX_SDK_EXAMPLE_NFC_FORUM_NDEF_H_)
#define _PTX_SDK_EXAMPLE_NFC_FORUM_NDEF_H_

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

/*
 * ####################################################################################################################
 * TYPES
 * ####################################################################################################################
 */

/**
 * \brief Demo-Application States.
 */
typedef enum ptx_DemoAppState
{
    DemoAppState_StartDiscovery,       /**< Start NFC Forum Discovery / Polling-Loop. */
    DemoAppState_WaitForDiscovery,     /**< Wait until Polling-Loop discovered, resolved and potentially also activated available card(s). */
    DemoAppState_SelectCard,           /**< Select specific card in case multiple cards were found or one card supporting multiple protocols. */
    DemoAppState_NDEFDataExchange,     /**< Perform example data exchange for every activated Type Tag using the functions from the NDEF Add-on API. */
    DemoAppState_DeactivateReader,     /**< Deactivate Reader i.e. switch RF-Field off. */
    DemoAppState_RestartDiscovery,     /**< Re-Start NFC Forum Discovery / Polling-Loop. */
    DemoAppState_SystemError,          /**< Critical (System-)Error occurred. */
    DemoAppState_Exit,                 /**< Stop Demo-Application (no specific reason)). */
} ptx_DemoAppState_t;


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


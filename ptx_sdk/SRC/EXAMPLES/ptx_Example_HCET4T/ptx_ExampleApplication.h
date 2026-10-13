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
    Module      : SDK Example (Host Card Emulation Typ A - T4T)
    File        : EXAMPLES/ptx_Example_HCET4T/ptx_ExampleApplication.c

    Description : This example shows how to setup Host Card Emulation (HCE) for NFC Typ A emulating a Type 4 Tag (T4T).
                  An exemplary T4T implementation, following the standard can be found in the /HCE_T4T folder. The ptx_RUL_HCE module only handles
                  collecting events and sending data. Which allows more flexibility and modularity in the application.
                  The scenario in this example is as follows:
                  - Initializing all components:
                        * Initialize main Rul component
                        * Initialize Rul hardware
                        * Initialize the T4T implementation
                        * Initialize the RUL_HCE module
                  - Initiate discovery for Type A + Iso Dep
                  - Update emulated ndef memory of T4T to initial value, here "Hello World"
                  - Entering the main loop of the application repeating following steps
                        * Call ptx_RUL_HCE_GetEvent to receive the latest event
                        * If event occurred parse the event accordingly
                  - On application exit, Deinitialize all components in reverse order.
*/


/**
 * \addtogroup SDK_EXAMPLES
 *
 * @{
 */

#if !defined(_PTX_SDK_EXAMPLE_HCE_T4T_H_)
#define _PTX_SDK_EXAMPLE_HCE_T4T_H_

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
    DemoAppState_ProcessEvents,        /**< Process Listen/Card-Emulation Events. */
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


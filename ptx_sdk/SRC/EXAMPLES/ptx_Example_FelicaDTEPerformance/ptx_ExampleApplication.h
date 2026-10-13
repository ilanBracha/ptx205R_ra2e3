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
    Module      : SDK Example (Felica DTE Performance)
    File        : EXAMPLES/ptx_Example_FelicaDTEPerformance/ptx_ExampleApplication.h

    Description : This application executes the FelicaDTE Performance Tests using the
                  FeliCaDTE add-on API. Various test parameters can be changed via defines.
                  Results are printed in the console or via uart logging on an mcu.
*/

/**
 * \addtogroup SDK_EXAMPLES
 *
 * @{
 */

#if !defined(_PTX_SDK_EXAMPLE_FELICA_DTE_PERFORMANCE_H_)
#define _PTX_SDK_EXAMPLE_FELICA_DTE_PERFORMANCE_H_

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
    DemoAppState_SetupTestParams,               /**< Setup the test parameter struct. */
    DemoAppState_EnableFeliCaMode,              /**< Turn on FeliCa DTE mode. */
    DemoAppState_RunPerformanceTest,            /**< Execute the test. */
    DemoAppState_PrintPerformanceTestResults,   /**< Format and print the test results. */
    DemoAppState_DisableFeliCaMode,             /**< Turn off FeliCa DTE mode. */
    DemoAppState_SystemError,                   /**< Critical (System-)Error occurred. */
    DemoAppState_Exit,                          /**< Stop Demo-Application (no specific reason). */

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


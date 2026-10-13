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
    Module      : SDK Example (NFC Forum DTA Basic Pattern-based Execution)
    File        : EXAMPLES/ptx_Example_NFCForumDta/ptx_ExampleApplication.h

    Description : This application example demonstrates how to use the NFC Forum DTA Add-on API module.
                  The NFC Forum DTA API implementation is based on the "Device Test Application"-specification defined
                  by the NFC Forum and can be used for NFC Forum Compliance testing.
                  The NFC Forum DTA works with so called Pattern-numbers where every number specifies (and triggers) a
                  specific scenario how the NFC device shall behave during a test.
                  For more details on Pattern-numbers and the specific test-scenarios, please refer to "Device Test Application
                  Specification"-document Annex C "PATTERN_NUMBER".

                  Attention: The NFC Forum DTA API implements only the Pattern-number bases scenarios, but not the actual
                             test-cases as specified by the NFC Forum.
                             Test-case implementation is highly dependent on the tools/environment of the test-vendor and
                             is therefore out of scope of this NFC Forum DTA API implementation.

                  Note: The NFC Forum DTA API is implemented in a blocking-manner.
                        An optional callback-function is supported upon module initialization which supports cancellation / stopping
                        an ongoing Pattern-/Test-execution.

                  Note: The NFC Forum DTA API is based on the main RUL API, but works completely stand-alone i.e. no
                        pre-allocated/-initialized RUL-instance is necessary here; the NFC Forum DTA API manages all the
                        resource allocation & initialization internally.

                  Note: System-errors (e.g. temperature events) are handled within the NFC Forum DTA module.
*/

/**
 * \addtogroup SDK_EXAMPLES
 *
 * @{
 */

#if !defined(_PTX_SDK_EXAMPLE_NFC_FORUM_DTA_BASIC_PATTERN_H_)
#define _PTX_SDK_EXAMPLE_NFC_FORUM_DTA_BASIC_PATTERN_H_

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


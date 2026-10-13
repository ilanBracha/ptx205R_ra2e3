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
    Module      : SDK Example (Log)
    File        : EXAMPLES/ptx_Example_Log/ptx_ExampleApplication.c

    Description : This application example demonstrates the usage of the Logging-component (LOG).
                  The Logging-component is integrated through the various layers of the Stack but can be
                  used stand-alone and/or can be integrated into the top-level application.
                  The stand-alone usage is demonstrated in this example. In case of RA4M2 usage UART has to be connected
                  in order for this example to work.
*/

/*
 * ####################################################################################################################
 * INCLUDES
 * ####################################################################################################################
 */

#include "ptx_ExampleApplication.h"
#include "ptx_Log.h"
#include "ptx_Utils_Print.h"
#include <stdio.h>

/*
 * ####################################################################################################################
 * DEFINES / TYPES / CONSTANTS
 * ####################################################################################################################
 */

/*
 * ####################################################################################################################
 * INTERNAL FUNCTIONS DECLARATIONS / HELPERS
 * ####################################################################################################################
 */

/*
 * ####################################################################################################################
 * APPLICATION ENTRY / INTERNAL FUNCTIONS / CALLBACKS
 * ####################################################################################################################
 */

#if defined(PTX_PLATFORM_MCU)
    int ptx_App_Entry(void)
#else
    int main(int argc, char **argv)
#endif
{
    ptx_Status_t status;

    /* LOG Main API component */
    ptx_Log_t log = {0};
    ptx_Log_InitParams_t log_init_params = {0};

    /* Test Buffer to log */
    uint8_t TEST_BUFFER[10] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09};

    /* Initialize LOG Main RUL API Component. */
    log_init_params.LogType = ptx_Log_Type_Default;
    log_init_params.LogCtx = &log;

    /* After succesful initialization of the LOG component, the macros defined in ptx_Log.h can be freely used at application level. */
    status = ptx_Log_Init(&log, &log_init_params);

    if (PTX_STATUS_SUCCESS == status)
    {
        /* All Log-messages created wit PTX_LOG_* can be found in file LogOutput.txt (same folder of application executable). */
        PTX_LOG_I(&log, "***********************************************************\n");
        PTX_LOG_I(&log, "****  Logger Demo %s             ****\n", PTX_DEMO_VERSION);
        PTX_LOG_I(&log, "***********************************************************\n");

        PTX_LOG_I(&log, "LOG Component successfully initialized!\n");

        PTX_LOG_I(&log, "Log-Entry: Trace, Log-Level: Information");
        PTX_LOG_D(&log, "Log-Entry: Trace, Log-Level: Debug");
        PTX_LOG_I_MEM(&log, &TEST_BUFFER, sizeof(TEST_BUFFER), "Log-Entry Memory, Log-Level Info..: ");
        PTX_LOG_D_MEM(&log, &TEST_BUFFER, sizeof(TEST_BUFFER), "Log-Entry Memory, Log-Level Debug.: ");
    }
    else
    {
        /* LOG Component initialization failed */
    }

    if (PTX_STATUS_SUCCESS == status)
    {
        status = ptx_Log_DeInit(&log);
    }

    (void)TEST_BUFFER;

#if defined(PTX_PLATFORM_OS)
    (void)argc;
    (void)argv;
#endif

    return 0;
}
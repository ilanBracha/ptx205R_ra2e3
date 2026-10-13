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
    File        : EXAMPLES/ptx_Example_NFCForumDta/ptx_ExampleApplication.c

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

/*
 * ####################################################################################################################
 * INCLUDES
 * ####################################################################################################################
 */

#include "ptx_ExampleApplication.h"
#include "ptx_Status.h"
#include "ptx_RUL.h"
#include "ptx_NFCForumDTA.h"
#include "ptx_Utils_AppParameters.h"
#include "ptx_Utils_Print.h"
#include "ptx_Utils_VersionInfo.h"
#include "ptx_Utils_CardInfo.h"
#include <string.h>
#include <stdio.h>
#if defined(PTX_APP_DIVERSITY_WINDOWS)
    #include <wincon.h>
#endif
#if defined(PTX_APP_DIVERSITY_LINUX)
    #include <stdlib.h>
    #include <signal.h>
#endif

#if defined(PTX_ENABLE_BOARD_EVK)
    #include "ptx_EVK_Peripherals.h"
#endif

/*
 * ####################################################################################################################
 * DEFINES / TYPES / CONSTANTS
 * ####################################################################################################################
 */


/* Device Name used during RUL-initialization. Only used for Desktop-OS versions like Windows or Linux. Can be omitted for MCU platforms. */
static const char *EXAMPLE_DEVICE_NAME = "COM4";

/* Device Name Length. See above. */
static const uint16_t EXAMPLE_DEVICE_NAME_LEN = 4u;

#if defined(PTX_ENABLE_SPI)
    /* Interface Type for the connection to the PTX2xxX chips. */
    static const ptx_HAL_Interface_Type_t EXAMPLE_INTERFACE_TYPE = HAL_Interface_Type_SPI;
#elif PTX_ENABLE_I2C
    /* Interface Type for the connection to the PTX2xxX chips. */
    static const ptx_HAL_Interface_Type_t EXAMPLE_INTERFACE_TYPE = HAL_Interface_Type_I2C;
#elif PTX_ENABLE_UART
    /* Interface Type for the connection to the PTX2xxX chips. */
    static const ptx_HAL_Interface_Type_t EXAMPLE_INTERFACE_TYPE = HAL_Interface_Type_UART;
#else
    /* Interface Type for the connection to the PTX2xxX chips. */
    static const ptx_HAL_Interface_Type_t EXAMPLE_INTERFACE_TYPE = HAL_Interface_Type_SPI;
#endif

#if defined(PTX_ENABLE_BOARD_EVK)
    /* Interface Driver Port implementation type for the connection to the PTX2xxX chips. */
    static const ptx_HAL_DRV_Interface_Port_Type_t EXAMPLE_INTERFACE_PORT_TYPE = HAL_DRV_Interface_Port_Type_Evk;
#else
    /* Interface Driver Port implementation type for the connection to the PTX2xxX chips. */
    static const ptx_HAL_DRV_Interface_Port_Type_t EXAMPLE_INTERFACE_PORT_TYPE = HAL_DRV_Interface_Port_Type_Default;
#endif

/* Interface Driver Port implementation ID for the connection to the PTX2xxX chips. */
static const uint8_t EXAMPLE_INTERFACE_PORT_ID = 0;

/* Requested number of hardware timer instances. */
static const uint8_t EXAMPLE_NUMBER_OF_TIMERS = 2;


/* Application specific Exit-Flag (controllable by the user on Windows-/Linux-platforms via Ctrl-C). */
static volatile int gExit = 0;

#if defined(PTX_ENABLE_BOARD_EVK)
    /* Corresponds to the voltage on the DC/DC converter on the PTX2xxW Boards. E.g. a value of 330 corresponds to 3.3V. */
    static const uint16_t EVK_DEFAULT_VOLTAGE = 500u;
#endif

/* Enable (1) or Disable (0) DDPC (Digital Dynamic Power Control). */
#define PTX_DEMO_APP_ENABLE_DDPC                           (uint8_t)1
#define PTX_DEMO_APP_DISABLE_DDPC                          (uint8_t)0

/*
 * ####################################################################################################################
 * INTERNAL FUNCTIONS DECLARATIONS / HELPERS
 * ####################################################################################################################
 */
static ptx_Status_t ptx_RunDemoApplication(ptx_RUL_t *rul);

/* Optional callback-function to stop an ongoing Pattern(-test) execution. */
static void ptx_Demo_Application_DTA_Callback_Fn(void* ctx);

#if PTX_APP_DIVERSITY_WINDOWS
/*
 * Add a platform dependent handler for a console event.
 * It is used to catch the user key press application-quit event Ctrl-C.
 */
BOOL WINAPI ConsoleHandler(DWORD dwType);
BOOL WINAPI ConsoleHandler(DWORD dwType)
{
    if (dwType == CTRL_C_EVENT)
    {
        ptx_Utils_Print("Application interrupted by user\n");
        gExit = 1;
    }
    return TRUE;
}

#endif

#if defined(PTX_APP_DIVERSITY_LINUX)
/*
 * Add a platform dependent handler for a console event.
 * It is used to catch the user key press application-quit event Ctrl-C.
 */
static void handler(int sig)
{
    if (sig == SIGINT)
    {
        ptx_Utils_Print("Application interrupted by user\n");
        gExit = 1;
    }
    else
    {
        exit(sig);
    }
}
#endif

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

    /*
    * Define platform dependent console handler function to handle application quit event: key press Ctrl-C.
    */
#if defined(PTX_APP_DIVERSITY_WINDOWS)
    SetConsoleCtrlHandler((PHANDLER_ROUTINE)ConsoleHandler, TRUE);
#elif defined(PTX_APP_DIVERSITY_LINUX)
    signal(SIGINT,  handler);
#endif

    ptx_Status_t status;

    /* Demo Application Parameters */
    ptx_Utils_AppParams_t app_params;

    /* Main API component */
    ptx_RUL_t rul = {0};
    ptx_RUL_InitParams_t rul_init_params = {0};

    /* Other components */
    ptx_RUL_DeactivateParams_t deact_params = {0};

    ptx_Utils_Print("***********************************************************\n");
    ptx_Utils_Print("****   NFC Forum DTA Basic Pattern Execution Demo %s             ****\n", PTX_DEMO_VERSION);
    ptx_Utils_Print("***********************************************************\n");


    /* Initialize demo application parameters with default values. */
    (void)memcpy(&app_params.DeviceName[0], EXAMPLE_DEVICE_NAME, EXAMPLE_DEVICE_NAME_LEN);
    app_params.DeviceNameLen = EXAMPLE_DEVICE_NAME_LEN;

#if defined(PTX_PLATFORM_OS)
    /* Load application parameters from command-line (if provided), otherwise use default values. */
    ptx_Utils_AppParameters_Load((uint32_t)argc, argv, &app_params);
#endif


    /* Initialize Main RUL API Component.
     *
     * Note: The used Host-Interface i.e. the HAL-component and the Logging-component (if used) get selected via the Build-process (see User Manual)
     *       and get also initialized during the next call to "ptx_RUL_Init".
     */
    rul_init_params.InterfaceInitParams.InterfaceName = (uint8_t*)&app_params.DeviceName[0];
    rul_init_params.InterfaceInitParams.InterfaceNameLen = app_params.DeviceNameLen;
    rul_init_params.InterfaceInitParams.InterfaceType = EXAMPLE_INTERFACE_TYPE;
    rul_init_params.InterfaceInitParams.InterfacePortType = EXAMPLE_INTERFACE_PORT_TYPE;
    rul_init_params.InterfaceInitParams.InterfacePortID = EXAMPLE_INTERFACE_PORT_ID;
    rul_init_params.InterfaceInitParams.NumberOfTimers = EXAMPLE_NUMBER_OF_TIMERS;

    status = ptx_RUL_Init(&rul, &rul_init_params);

    if (PTX_STATUS_SUCCESS == status)
    {
        ptx_Utils_Print("RUL SW-Stack successfully initialized!\n");

#if defined(PTX_ENABLE_BOARD_EVK)
        /*
        * Allocate the RUL Peripherals data structure.
        * This component is needed for operation of the PTX2xxR/W EVK.
        * It controls additional circuitry on the PCB like a DC/DC converter,
        * an (optional) on-PCB temperature sensor as well as (optional) power sensors.
        */
        ptx_EVK_Peripherals_t peripherals;
        memset(&peripherals, 0, sizeof(ptx_EVK_Peripherals_t));
        peripherals.Log = &rul.Log;

        if (PTX_STATUS_SUCCESS == status)
        {
            /* Initialize the RUL Peripherals component. */
            status = ptx_EVK_Peripherals_Init(&peripherals);
        }

        if (PTX_STATUS_SUCCESS == status)
        {
            /* Set the DC/DC converter to the default voltage of 5.0 Volts. */
            status = ptx_EVK_Peripherals_DCDC_SetVoltage(&peripherals.Dcdc, EVK_DEFAULT_VOLTAGE);
        }
#endif

        /* Initialize the Hardware by
        *  - downloading the Firmware-image and starting execution,
        *  - initializing the hardware / firmware,
        *  - downloading the default RF-configuration,
        *  - changing into READY-state.
        */
        status = ptx_RUL_InitHardware(&rul);

        if (PTX_STATUS_SUCCESS == status)
        {
            ptx_Utils_Print("PTX Hardware-/-Chip successfully initialized!\n");

            /* Print HW-Info and all available Stack-revisions */
            status = ptx_Utils_VersionInfo_Print_Revision(&rul);

            if (PTX_STATUS_SUCCESS == status)
            {
                status = ptx_RunDemoApplication(&rul);

                ptx_Utils_Print("Demo Application completed with Status %04X!\n", status);
            }
            else
            {
                ptx_Utils_Print("Print Status/Revision information failed with Error-Status %04X!\n", status);
            }
        }
        else
        {
            ptx_Utils_Print("Hardware initialization failed with Error-Status %04X!\n", status);
        }

    }
    else
    {
        ptx_Utils_Print("RUL-Stack initialization failed with Error-Status %04X!\n", status);
    }

    /* Switch RF-Field off (independent of previous state) */
    deact_params.Type = ptx_RUL_DeactivateType_Generic;
    deact_params.State = ptx_RUL_DeactivateState_Idle;
    (void)ptx_RUL_Deactivate(&rul, &deact_params);

    /* De-initialize the RUL API Component (independent of previous state) */
    status = ptx_RUL_DeInit(&rul);

    if (PTX_STATUS_SUCCESS == status)
    {
        ptx_Utils_Print("RUL-Stack successfully de-initialized!\n");
    }
    else
    {
        ptx_Utils_Print("RUL-Stack de-initialization failed with Error-Status %04X!\n", status);
    }

#if defined(PTX_PLATFORM_OS)
    (void)argc;
    (void)argv;
#endif

    return 0;
}

static ptx_Status_t ptx_RunDemoApplication(ptx_RUL_t *rul)
{
    ptx_Status_t status = PTX_STATUS_E_GEN_INVALID_PARAMETER;

    /* NFC Forum DTA Add-on API component */
    ptx_NFCForumDTA_t dta = {0};
    ptx_NFCForumDTA_InitParams_t dta_init_params = {0};
    uint8_t dta_version = 0;

    uint8_t dta_NDEF_Message_Buffer[PTX_NFCFORUMDTA_NDEF_MESSAGE_BUFFER_SIZE] = {0};
    uint32_t dta_NDEF_Message_Buffer_Size = PTX_NFCFORUMDTA_NDEF_MESSAGE_BUFFER_SIZE;
    uint32_t dta_NDEF_Message_Buffer_Length = 0;

    /* NFC Forum DTA API Pattern/Test Configuration Parameters */
    ptx_NFCForumDTA_Test_Config_t dta_test_config_params = {0};

    if (NULL != rul)
    {
        /* Initialize NFC Forum DTA Add-on API component.
        *
        * Note:
        *   -- "applicationCallback"    Optional callback that can be used to stop ongoing DTA-execution
        */
        dta_init_params.rul = rul;
        dta_init_params.NDEFMessageBuffer = dta_NDEF_Message_Buffer;
        dta_init_params.NDEFMessageBufferSize = dta_NDEF_Message_Buffer_Size;
        dta_init_params.NDEFMessageBufferLen = &dta_NDEF_Message_Buffer_Length;
        dta_init_params.applicationCallback = &ptx_Demo_Application_DTA_Callback_Fn;

        /* Initialize the NFC Forum DTA API component. */
        status = ptx_NFCForumDTA_Init(&dta, &dta_init_params);

        if (PTX_STATUS_SUCCESS == status)
        {
            ptx_Utils_Print("NFC Forum DTA API successfully initialized!\n");

            status = ptx_NFCForumDTA_GetVersion(&dta_version);

            if (PTX_STATUS_SUCCESS == status)
            {
                ptx_Utils_Print("NFC Forum DTA API Version = %02d\n", dta_version);
            }
            else
            {
                ptx_Utils_Print("Reading Forum DTA API version failed with Error-Status %04X!\n", status);
            }

            ptx_Utils_Print("Running NFC Forum DTA Pattern Sequence: %04X!\n", dta_test_config_params.PATTERN_NUMBER);

            /*
            *  Configure Pattern/Test sequence according to "Device Test Application Specification"-document by NFC Forum.
            *  The configuration parameter structure supports additional, but optional configuration parameters to control
            *  a Pattern/Test sequence.
            */
            if (PTX_STATUS_SUCCESS == status)
            {
                /* DTA Pattern Number. */
                dta_test_config_params.PATTERN_NUMBER = 0x0001;

                /* Max. timeout for RF-discovery - if nothing detected within this time, DTA Pattern execution stops. */
                dta_test_config_params.DiscoveryTimeout = 200000;

                /* Disable DDPC for this Example */
                dta_test_config_params.EnableDDPC = PTX_DEMO_APP_DISABLE_DDPC;

                /* Set reader device type to only poll for expected technologies for the Reader device class*/
                dta_test_config_params.DeviceType = ptx_NFCForumDTA_DevType_Reader;

                do
                {
                    /*
                    *  Execute actual single Pattern/Test scenario.
                    *  The function returns in case
                    *   - the Pattern/Test scenario has finished or
                    *   - an error occurred or
                    *   - the user stopped the execution via the optional callback function.
                    *
                    *  If an application wants to execute a specific amount of Pattern/Test scenarios,
                    *  the application can use API function "ptx_NFCForumDTA_RepeatTest" instead.
                    */
                    status = ptx_NFCForumDTA_StartDTARequest(&dta, &dta_test_config_params);

                    if (PTX_STATUS_SUCCESS == status)
                    {
                        ptx_Utils_Print("NFC Forum DTA Pattern Sequence PASSED!\n");
                        (void)ptx_RUL_Sleep(rul, 2000);

                    }
                    else
                    {
                        ptx_Utils_Print("NFC Forum DTA Pattern Sequence failed with Error-Status %04X!\n", status);
                    }

                }
                while (0 == gExit);
            }

            /* Deinitialize the NFC Forum DTA component. */
            status = ptx_NFCForumDTA_DeInit(&dta);

            if (PTX_STATUS_SUCCESS == status)
            {
                ptx_Utils_Print("NFC Forum DTA API successfully uninitialized!\n");
            }
            else
            {
                ptx_Utils_Print("De-initializing Forum DTA API failed with Error-Status %04X!\n", status);
            }
        }
    }

    return status;

}

static void ptx_Demo_Application_DTA_Callback_Fn(void* ctx)
{
    ptx_NFCForumDTA_t *dta;

    if (NULL != ctx)
    {
        dta = (ptx_NFCForumDTA_t*)ctx;

        if (0 != gExit)
        {
            dta->quitDTA = 1U;
        }

    }
}

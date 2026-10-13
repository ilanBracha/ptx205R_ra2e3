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
    Module      : SDK Example (Felica DTE Protocol Tests)
    File        : EXAMPLES/ptx_Example_FelicaDTEProtocolTests/ptx_ExampleApplication.c

    Description : This application executes a specific FelicaDTE Digital Protocol Test
                  using the FeliCaDTE add-on API. The subtest id is selected via defines.
                  Only one may be active at a time. Results are printed in the console or
                  via uart logging on an mcu.
*/

/*
 * ####################################################################################################################
 * INCLUDES
 * ####################################################################################################################
 */

#include "ptx_ExampleApplication.h"
#include "ptx_Status.h"
#include "ptx_RUL.h"
#include "ptx_Utils_AppParameters.h"
#include "ptx_Utils_Print.h"
#include "ptx_Utils_VersionInfo.h"
#include "ptx_Utils_CardInfo.h"
#include "ptx_FeliCa_DTE.h"
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

/* Timeout in MS for protocol tests. */
#define PTX_DEMO_APP_MS_TIMEOUT_PROTOCOL_TESTS 500u

/* Command Buffer for Generic Protocol Test */
static const uint8_t PTX_DEMO_APP_GENERIC_CMD_BUFFER[PTX_DEMO_APP_GENERIC_CMD_BUFFER_SIZE] = {0x06, 0x00, 0xFF, 0xFF, 0x00, 0x00};

/* Selected Protocol Test to execute. Only one may be active at a time! */
#define PTX_DEMO_APP_FELICA_DTE_TEST_ID FELICA_DTE_SubTestID_RWDigProt_eMoney_Variant1
// #define PTX_DEMO_APP_FELICA_DTE_TEST_ID FELICA_DTE_SubTestID_RWDigProt_eMoney_Variant2
// #define PTX_DEMO_APP_FELICA_DTE_TEST_ID FELICA_DTE_SubTestID_RWDigProt_eMoney_NFCDEP
// #define PTX_DEMO_APP_FELICA_DTE_TEST_ID FELICA_DTE_SubTestID_RWDigProt_FrameStructure
// #define PTX_DEMO_APP_FELICA_DTE_TEST_ID FELICA_DTE_SubTestID_RWDigProt_FrameStructure1
// #define PTX_DEMO_APP_FELICA_DTE_TEST_ID FELICA_DTE_SubTestID_RWDigProt_FrameStructure2
// #define PTX_DEMO_APP_FELICA_DTE_TEST_ID FELICA_DTE_SubTestID_RWDigProt_FrameStructure3
// #define PTX_DEMO_APP_FELICA_DTE_TEST_ID FELICA_DTE_SubTestID_RWDigProt_FrameStructure4
// #define PTX_DEMO_APP_FELICA_DTE_TEST_ID FELICA_DTE_SubTestID_RWDigProt_FrameStructure5
// #define PTX_DEMO_APP_FELICA_DTE_TEST_ID FELICA_DTE_SubTestID_RWDigProt_FrameStructure6
// #define PTX_DEMO_APP_FELICA_DTE_TEST_ID FELICA_DTE_SubTestID_RWDigProt_Setup_DP_POS
// #define PTX_DEMO_APP_FELICA_DTE_TEST_ID FELICA_DTE_SubTestID_RWDigProt_Setup_DP_NEG
// #define PTX_DEMO_APP_FELICA_DTE_TEST_ID FELICA_DTE_SubTestID_RWDigProt_Generic

/*
 * ####################################################################################################################
 * INTERNAL FUNCTIONS DECLARATIONS / HELPERS
 * ####################################################################################################################
 */
static ptx_Status_t ptx_RunDemoApplication(ptx_RUL_t *rul);

static ptx_Status_t ptx_ProcessState_SetupTestParams(ptx_DemoAppState_t *appState, ptx_FeliCaDTE_TestParams_t *testParams, uint8_t *resultBuffer, uint8_t* cmdBuffer);
static ptx_Status_t ptx_ProcessState_EnableFelicaMode(ptx_FeliCaDTE_t *felica, ptx_DemoAppState_t *appState, ptx_FeliCaDTE_TestParams_t *testParams);
static ptx_Status_t ptx_ProcessState_RunProtocolTest(ptx_FeliCaDTE_t *felica, ptx_DemoAppState_t *appState, ptx_FeliCaDTE_TestParams_t *testParams);
static ptx_Status_t ptx_ProcessState_PrintTestResults(ptx_DemoAppState_t *appState, ptx_FeliCaDTE_TestParams_t *testParams);
static ptx_Status_t ptx_ProcessState_DisableFelicaMode(ptx_FeliCaDTE_t *felica, ptx_DemoAppState_t *appState);
static ptx_Status_t ptx_ProcessState_SystemError(ptx_RUL_t *rul, ptx_DemoAppState_t *appState, ptx_RUL_Status_Params_t *statusInfoParam);
static ptx_Status_t ptx_ProcessState_RestartSystem(ptx_RUL_t *rul, ptx_DemoAppState_t *appState);

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
    ptx_Utils_Print("****   Felica DTE Protocol Test Demo %s              ****\n", PTX_DEMO_VERSION);
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

        /* Initialize the Hardware */
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

    ptx_DemoAppState_t demo_state = DemoAppState_SetupTestParams;
    ptx_RUL_Status_Params_t status_info = {0};

    /* FeliCa library component */
    ptx_FeliCaDTE_t felica_comp = {0};
    ptx_FeliCaDTE_InitParams_t felica_init_params = {0};
    ptx_FeliCaDTE_TestParams_t felica_test_params = {0};

    uint8_t result_buffer[PTX_DEMO_APP_EXAMPLE_RESULT_BUFFER_SIZE] = {0};
    uint8_t cmd_buffer[PTX_DEMO_APP_GENERIC_CMD_BUFFER_SIZE] = {0};
    memcpy(&cmd_buffer[0], &PTX_DEMO_APP_GENERIC_CMD_BUFFER[0], PTX_DEMO_APP_GENERIC_CMD_BUFFER_SIZE);

    if (NULL != rul)
    {
        /* Initialize FeliCa-DTE Add-on API component. */
        felica_init_params.Rul = rul;
        status = ptx_FeliCaDTE_Open(&felica_comp, &felica_init_params);

        if (PTX_STATUS_SUCCESS == status)
        {
            ptx_Utils_Print("FeliCa-DTE API successfully initialized!\n");

            do
            {
                /* Check regularly for System-state/-health (e.g. temperature/over-current errors etc.) */
                status_info.Type = ptx_RUL_StatusType_System;

                status = ptx_RUL_GetStatusInfo(rul, &status_info);

                if (PTX_STATUS_SUCCESS == status)
                {
                    if (ptx_RUL_StatusInfoSystem_OK != status_info.Info.System)
                    {
                        demo_state = DemoAppState_SystemError;
                    }
                }

                if (PTX_STATUS_SUCCESS == status)
                {
                    switch (demo_state)
                    {
                        case DemoAppState_SetupTestParams:
                        {
                            status = ptx_ProcessState_SetupTestParams(&demo_state, &felica_test_params, &result_buffer[0], &cmd_buffer[0]);
                            break;
                        }

                        case DemoAppState_EnableFeliCaMode:
                        {
                            status = ptx_ProcessState_EnableFelicaMode(&felica_comp, &demo_state, &felica_test_params);
                            break;
                        }

                        case DemoAppState_RunProtocolTest:
                        {
                            status = ptx_ProcessState_RunProtocolTest(&felica_comp, &demo_state, &felica_test_params);
                            break;
                        }

                        case DemoAppState_PrintTestResults:
                        {
                            status = ptx_ProcessState_PrintTestResults(&demo_state, &felica_test_params);
                            break;
                        }

                        case DemoAppState_DisableFeliCaMode:
                        {
                            status = ptx_ProcessState_DisableFelicaMode(&felica_comp, &demo_state);
                            break;
                        }

                        case DemoAppState_SystemError:
                        {
                            status = ptx_ProcessState_SystemError(rul, &demo_state, &status_info);
                            break;
                        }

                        case DemoAppState_Exit:
                        {
                            /* Nothing to do here */
                            break;
                        }

                        default:
                        {
                            ptx_Utils_Print("Invalid Application-State detected!\n");
                            demo_state = DemoAppState_Exit;
                            break;
                        }
                    }
                }

            }
            while ((PTX_STATUS_SUCCESS == status) && (DemoAppState_Exit != demo_state) && (0 == gExit));
            /* De-initialize the FeliCa-DTE API Component. */
            status = ptx_FeliCaDTE_Close(&felica_comp);
        }
        else
        {
            ptx_Utils_Print("FeliCa-DTE API initialization failed with Error-Status %04X!\n", status);
        }

        ptx_Utils_Print("Exiting Demo-Application...\n");
    }

    return status;
}

static ptx_Status_t ptx_ProcessState_SetupTestParams(ptx_DemoAppState_t *appState, ptx_FeliCaDTE_TestParams_t *testParams, uint8_t *resultBuffer, uint8_t* cmdBuffer)
{
    ptx_Status_t status = PTX_STATUS_E_GEN_INVALID_PARAMETER;

    if ((NULL != appState) && (NULL != testParams) && (NULL != resultBuffer) && (NULL != cmdBuffer))
    {
        testParams->ID = FELICA_DTE_TestID_RWDigProtTest;
        testParams->Params.RWDigProtTest.SubTestID = PTX_DEMO_APP_FELICA_DTE_TEST_ID;
        testParams->Params.RWDigProtTest.ResultBufferSize = PTX_DEMO_APP_EXAMPLE_RESULT_BUFFER_SIZE;
        testParams->Params.RWDigProtTest.ResultBuffer = resultBuffer;
        testParams->Params.RWDigProtTest.GenericCmdBuffer = cmdBuffer;
        testParams->Params.RWDigProtTest.GenericCmdBufferLen = PTX_DEMO_APP_GENERIC_CMD_BUFFER_SIZE;
        testParams->Params.RWDigProtTest.TimeOutMS = PTX_DEMO_APP_MS_TIMEOUT_PROTOCOL_TESTS;

        testParams->ProgressCB = NULL;

        *appState = DemoAppState_EnableFeliCaMode;
        status = PTX_STATUS_SUCCESS;
    }

    return status;
}

static ptx_Status_t ptx_ProcessState_EnableFelicaMode(ptx_FeliCaDTE_t *felica, ptx_DemoAppState_t *appState, ptx_FeliCaDTE_TestParams_t *testParams)
{
    ptx_Status_t status = PTX_STATUS_E_GEN_INVALID_PARAMETER;

    if ((NULL != felica) && (NULL != appState) && (NULL != testParams))
    {
        status = ptx_FeliCaDTE_EnableMode(felica, (uint8_t)1, testParams);

        if (PTX_STATUS_SUCCESS == status)
        {
            *appState = DemoAppState_RunProtocolTest;
            ptx_Utils_Print("FeliCa-DTE Mode successfully enabled!\n");
        }
        else
        {
            *appState = DemoAppState_Exit;
            ptx_Utils_Print("Enabling FeliCa-DTE Mode failed with Error-Status %04X!\n", status);
        }
    }

    return status;
}

static ptx_Status_t ptx_ProcessState_RunProtocolTest(ptx_FeliCaDTE_t *felica, ptx_DemoAppState_t *appState, ptx_FeliCaDTE_TestParams_t *testParams)
{
    ptx_Status_t status = PTX_STATUS_E_GEN_INVALID_PARAMETER;

    if ((NULL != felica) && (NULL != appState) && (NULL != testParams))
    {
        status = ptx_FeliCaDTE_RunTest(felica, testParams);

        if (PTX_STATUS_SUCCESS == status)
        {
            *appState = DemoAppState_PrintTestResults;
            ptx_Utils_Print("FeliCa-DTE protocol test was successfully run!\n");
        }
        else
        {
            *appState = DemoAppState_DisableFeliCaMode;
            ptx_Utils_Print("Running FeliCa-DTE protocol test failed with Error-Status %04X!\n", status);
        }
    }

    return status;
}

static ptx_Status_t ptx_ProcessState_PrintTestResults(ptx_DemoAppState_t *appState, ptx_FeliCaDTE_TestParams_t *testParams)
{
    ptx_Status_t status = PTX_STATUS_E_GEN_INVALID_PARAMETER;

    uint32_t index = 0;
    uint8_t quit_processing = 0;
    uint16_t sequence_counter = 1;
    uint16_t sequence_len;


    if ((NULL != testParams) && (NULL != appState))
    {
        ptx_Utils_Print("================================== RESULT BUFFER START ==================================");
        if ((NULL != testParams->Params.RWDigProtTest.ResultBuffer) && (0 != testParams->Params.RWDigProtTest.ResultBufferSize))
        {
            uint8_t *result_buffer = testParams->Params.RWDigProtTest.ResultBuffer;
            uint32_t result_buffer_size = testParams->Params.RWDigProtTest.ResultBufferSize;
            /* Note:
             * "result_buffer" structured as follows: Command-0 (+ prepended length), Response-0 (+ prepended length), Command-1, ...
             */
            while (0 == quit_processing)
            {
                if ((index + 1) <= result_buffer_size)
                {
                    if (0 != result_buffer[index])
                    {
                        sequence_len = (int)result_buffer[index];

                        if ((index + sequence_len) <= result_buffer_size)
                        {
                            if (0 != (sequence_counter % 2))
                            {
                                ptx_Utils_Print(sequence_counter + " TX Sequence = ");

                            }
                            else
                            {
                                ptx_Utils_Print(sequence_counter + " RX Sequence = ");
                            }

                            ptx_Utils_Print_Buffer(&result_buffer[index], sequence_len, 1, 0);

                            index += sequence_len;
                            sequence_counter++;

                        }
                        else
                        {
                            quit_processing = 1;
                        }
                    }
                    else
                    {
                        /* no more sequences stored - processing done */
                        quit_processing = 1;
                    }
                }
                else
                {
                    /* no more sequences stored - processing done */
                    quit_processing = 1;
                }
            }
        }
        else
        {
            ptx_Utils_Print("Error - Invalid Result-Parameters used;");
        }
        ptx_Utils_Print("================================== RESULT BUFFER END ==================================");

        *appState = DemoAppState_DisableFeliCaMode;
    }

    return status;
}

static ptx_Status_t ptx_ProcessState_DisableFelicaMode(ptx_FeliCaDTE_t *felica, ptx_DemoAppState_t *appState)
{
    ptx_Status_t status = PTX_STATUS_E_GEN_INVALID_PARAMETER;

    if ((NULL != felica) && (NULL != appState))
    {
        status = ptx_FeliCaDTE_EnableMode(felica, (uint8_t)0, NULL);

        if (PTX_STATUS_SUCCESS == status)
        {
            *appState = DemoAppState_Exit;
            ptx_Utils_Print("Successfully disabled FeliCa-DTE mode!\n");
        }
        else
        {
            ptx_Utils_Print("Disabling FeliCa-DTE mode failed with Error-Status %04X!\n", status);
        }
    }

    return status;
}

static ptx_Status_t ptx_ProcessState_SystemError(ptx_RUL_t *rul, ptx_DemoAppState_t *appState, ptx_RUL_Status_Params_t *statusInfoParam)
{
    ptx_Status_t status = PTX_STATUS_E_GEN_INVALID_PARAMETER;

    if ((NULL != rul) && (NULL != appState) && (NULL != statusInfoParam))
    {
        /* System-Errors-/-Warnings are critical and needs to be resolved at integration-/application-level.
         * There can be multiple root causes those errors/warnings, but in most cases they are related
         * the Hardware-environment (e.g. PCB, component issues etc.).
         */
        if (ptx_RUL_StatusInfoSystem_ErrorOvercurrent == statusInfoParam->Info.System)
        {
            ptx_Utils_Print("WARNING - Overcurrent Limiter activated!\n");
            *appState = DemoAppState_Exit;
        }

        if (ptx_RUL_StatusInfoSystem_ErrorTemperature == statusInfoParam->Info.System)
        {
            ptx_Utils_Print("WARNING - Temperature Limiter activated!\n");
            status = ptx_ProcessState_RestartSystem(rul, appState);
        }

        if (ptx_RUL_StatusInfoSystem_ErrorOutOfMemory == statusInfoParam->Info.System)
        {
            ptx_Utils_Print("WARNING - Out of Memory!\n");
            gExit = 1u;
        }


        if (ptx_RUL_StatusInfoSystem_NSCReset == statusInfoParam->Info.System)
        {
            ptx_Utils_Print("WARNING - Unexpected System Reset - exiting Application!\n");
            *appState = DemoAppState_Exit;
        }
    }

    return status;
}

static ptx_Status_t ptx_ProcessState_RestartSystem(ptx_RUL_t *rul, ptx_DemoAppState_t *appState)
{
    ptx_Status_t status = PTX_STATUS_E_GEN_INVALID_PARAMETER;

    if ((NULL != rul) && (NULL != appState))
    {
        do
        {
            /* Wait for 1s to give the chip time to cool down, after that time try to initialize the system again */
            status = ptx_RUL_Sleep(rul, 1000u);

            if (PTX_STATUS_SUCCESS == status)
            {
                status = ptx_RUL_InitHardware(rul);
            }

        }
        while (status != PTX_STATUS_SUCCESS);

        if (PTX_STATUS_SUCCESS == status)
        {
            *appState = DemoAppState_SetupTestParams;
        }
    }

    return status;
}
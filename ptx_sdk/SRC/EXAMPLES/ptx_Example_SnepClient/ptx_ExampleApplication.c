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
    Module      : SDK Example (Simple NDEF Exchange Protocol)
    File        : EXAMPLES/ptx_Example_SnepClient/ptx_ExampleApplication.c

    Description : This SDK application demonstrates the implementation of a Simple NDEF Exchange Protocol (SNEP) client
                  that performs a PUT request to a SNEP server.
*/

/*
 * ####################################################################################################################
 * INCLUDES
 * ####################################################################################################################
 */
#include "ptx_ExampleApplication.h"
#include "ptx_RUL.h"
#include "ptx_Utils_AppParameters.h"
#include "ptx_Utils_VersionInfo.h"
#include "ptx_Utils_Print.h"
#include "ptx_RUL_LLCP.h"
#include "ptx_RUL_SnepClient.h"
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
/* RF Polling interval set to 10ms. */
#define PTX_RF_POLLING_INTERVAL 10u

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

/* NDEF Text record for demonstration puporses. */
const uint8_t normal_record[37] = { 0xC1, 0x01, 0x00, 0x00, 0x00, 0x1E, 0x54, 0x02, 0x6C, 0x61, 0x4C, 0x6F, 0x72, 0x65,
                                    0x6D, 0x20, 0x69, 0x70, 0x73, 0x75, 0x6D, 0x20, 0x64, 0x6F, 0x6C, 0x6F, 0x72, 0x20,
                                    0x73, 0x69, 0x74, 0x20, 0x61, 0x6D, 0x65, 0x74, 0x2E
                                  };
/*
 * ####################################################################################################################
 * INTERNAL FUNCTIONS DECLARATIONS / HELPERS
 * ####################################################################################################################
 */

static ptx_Status_t ptx_RunDemoApplication(ptx_RUL_t *rul);
static ptx_Status_t ptx_ProcessState_Initialize(ptx_RUL_t *rul, ptx_RUL_LLCP_t *llcp, ptx_RUL_SnepClient_t *snep, ptx_DemoAppState_t *appState);
static ptx_Status_t ptx_ProcessState_InitiateDiscovery(ptx_RUL_SnepClient_t *ctx, ptx_DemoAppState_t *appState);
static ptx_Status_t ptx_ProcessState_WaitForActivation(ptx_RUL_SnepClient_t *ctx, ptx_DemoAppState_t *appState);
static ptx_Status_t ptx_ProcessState_InitiatePutRequest(ptx_RUL_SnepClient_t *ctx, ptx_DemoAppState_t *appState);
static ptx_Status_t ptx_ProcessState_InitiateGetRequest(ptx_RUL_SnepClient_t *ctx, ptx_DemoAppState_t *appState);
static ptx_Status_t ptx_ProcessState_ProcessRequest(ptx_RUL_SnepClient_t *ctx, ptx_DemoAppState_t *appState);
static ptx_Status_t ptx_ProcessState_Deactivate(ptx_RUL_SnepClient_t *ctx, ptx_DemoAppState_t *appState);
static ptx_Status_t ptx_ProcessState_SystemError(ptx_RUL_SnepClient_t *ctx, ptx_DemoAppState_t *appState);
static ptx_Status_t ptx_ProcessState_RestartSystem(ptx_RUL_t *rul, ptx_RUL_LLCP_t *llcp, ptx_RUL_SnepClient_t *snep, ptx_DemoAppState_t *appState);

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
    ptx_Utils_Print("****        NFC Forum SNEP Client API Demo %s          ****\n", PTX_DEMO_VERSION);
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
    (void) ptx_RUL_Deactivate(&rul, &deact_params);

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

    ptx_DemoAppState_t demo_state = DemoAppState_Initialize;
    ptx_RUL_Status_Params_t status_info = {0};

    if (NULL != rul)
    {
        ptx_RUL_LLCP_t llcp;
        memset(&llcp, 0, sizeof(ptx_RUL_LLCP_t));

        ptx_RUL_SnepClient_t snep;
        memset(&snep, 0, sizeof(ptx_RUL_SnepClient_t));

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
                    case DemoAppState_Initialize:
                    {
                        status = ptx_ProcessState_Initialize(rul, &llcp, &snep, &demo_state);
                        break;
                    }

                    case DemoAppState_StartDiscovery:
                    {
                        status = ptx_ProcessState_InitiateDiscovery(&snep, &demo_state);
                        break;
                    }

                    case DemoAppState_WaitForActivation:
                    {
                        status = ptx_ProcessState_WaitForActivation(&snep, &demo_state);
                        break;
                    }

                    case DemoAppState_PutRequest:
                    {
                        status = ptx_ProcessState_InitiatePutRequest(&snep, &demo_state);
                        break;
                    }

                    case DemoAppState_GetRequest:
                    {
                        status = ptx_ProcessState_InitiateGetRequest(&snep, &demo_state);
                        break;
                    }

                    case DemoAppState_Process:
                    {
                        status = ptx_ProcessState_ProcessRequest(&snep, &demo_state);
                        break;
                    }

                    case DemoAppState_Deactivate:
                    {
                        status = ptx_ProcessState_Deactivate(&snep, &demo_state);
                        break;
                    }

                    case DemoAppState_SystemError:
                    {
                        status = ptx_ProcessState_SystemError(&snep, &demo_state);
                        break;
                    }

                    case DemoAppState_RestartSystem:
                    {
                        status = ptx_ProcessState_RestartSystem(rul, &llcp, &snep, &demo_state);
                        break;
                    }

                    case DemoAppState_Exit:
                    {
                        /* Nothing to do here. */
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

            if (PTX_STATUS_SUCCESS != status)
            {
                ptx_Utils_Print("SNEP Loop status: %04X!\n", status);
            }

            /*
                * NOTE: This required for to this demo application only! Please use your own
                *       error handling for your application!
                *
                * In case of an PTX_STATUS_E_NSC_SYSTEM error, we can safely reset the status
                * and set it to PTX_STATUS_SUCCESS. We also need to set the demo_state to the
                * DemoAppState_SystemError to check for the error during the next iteration of
                * the program loop.
                */
            if (PTX_STATUS_E_NSC_SYSTEM == status)
            {
                demo_state = DemoAppState_SystemError;
                status = PTX_STATUS_SUCCESS;
            }

        }
        while ((PTX_STATUS_SUCCESS == status) && (DemoAppState_Exit != demo_state) && (0 == gExit));

        /* De-initialize the SNEP API Component. */
        status = ptx_RUL_SnepClient_DeInit(&snep);

        ptx_Utils_Print("Exiting Demo-Application...\n");
    }

    return status;
}

static ptx_Status_t ptx_ProcessState_Initialize(ptx_RUL_t *rul, ptx_RUL_LLCP_t *llcp, ptx_RUL_SnepClient_t *snep, ptx_DemoAppState_t *appState)
{
    ptx_Status_t status = PTX_STATUS_E_GEN_INVALID_PARAMETER;

    if ((NULL != rul) && (NULL != llcp) && (NULL != snep))
    {
        /* Start from a clean state. */
        memset(llcp, 0, sizeof(ptx_RUL_LLCP_t));
        memset(snep, 0, sizeof(ptx_RUL_SnepClient_t));

        /* Initialize the LLCP add-on API component. */
        ptx_RUL_LLCP_InitParameters_t init_params;
        memset(&init_params, 0, sizeof(ptx_RUL_LLCP_InitParameters_t));

        init_params.Rul = rul;
        status = ptx_RUL_LLCP_Init(llcp, &init_params);

        if (PTX_STATUS_SUCCESS == status)
        {
            status = ptx_RUL_SnepClient_Init(snep, llcp, 0u);
        }

        if (PTX_STATUS_SUCCESS == status)
        {
            *appState = DemoAppState_StartDiscovery;
            ptx_Utils_Print("SNEP API initialized successfully.");
        }
        else
        {
            ptx_Utils_Print("SNEP API initialization failed with Error-Status %04X!\n", status);
        }
    }

    return status;
}

static ptx_Status_t ptx_ProcessState_InitiateDiscovery(ptx_RUL_SnepClient_t *ctx, ptx_DemoAppState_t *appState)
{
    ptx_Status_t status = PTX_STATUS_E_GEN_INVALID_PARAMETER;

    if ((NULL != ctx) && (NULL != appState))
    {
        status = ptx_RUL_SnepClient_StartDefaultDiscovery(ctx, PTX_RF_POLLING_INTERVAL);

        if (PTX_STATUS_SUCCESS == status)
        {
            ptx_Utils_Print("RF-Discovery successfully started!\n");
            *appState = DemoAppState_WaitForActivation;
        }
        else
        {
            *appState = DemoAppState_RestartSystem;
        }
    }

    return status;
}

static ptx_Status_t ptx_ProcessState_WaitForActivation(ptx_RUL_SnepClient_t *ctx, ptx_DemoAppState_t *appState)
{
    ptx_Status_t status = PTX_STATUS_E_GEN_INVALID_PARAMETER;

    if ((NULL != ctx) && (NULL != appState))
    {
        ptx_RUL_Status_Params_t status_params;
        status_params.Type = ptx_RUL_StatusType_Discover;
        status = ptx_RUL_GetStatusInfo(ctx->Llcp->Rul, &status_params);

        /* Check LLCP support by interpreting the general bytes. */
        if (PTX_STATUS_SUCCESS == status)
        {
            uint8_t llcp_activated = 0;

            status = ptx_RUL_LLCP_IsLlcpSupported(ctx->Llcp, &llcp_activated);

            if (0 != llcp_activated)
            {
                ptx_Utils_Print("LLCP is supported by the connected peer!\n");

                status = ptx_RUL_SnepClient_ConnectToServer(ctx);

                if (PTX_STATUS_SUCCESS == status)
                {
                    ptx_Utils_Print("Successfully connected to the SNEP server!\n");
                    *appState = DemoAppState_PutRequest;
                }
                else
                {
                    *appState = DemoAppState_RestartSystem;
                }
            }
        }
    }

    return status;
}

static ptx_Status_t ptx_ProcessState_ProcessRequest(ptx_RUL_SnepClient_t *ctx, ptx_DemoAppState_t *appState)
{
    ptx_Status_t status = PTX_STATUS_E_GEN_INVALID_PARAMETER;

    if ((NULL != ctx) && (NULL != appState))
    {
        status = ptx_RUL_SnepClient_Handle(ctx);

        if (status == PTX_STATUS_SUCCESS)
        {
            if (ctx->ClientState == ClientState_Idle)
            {
                ptx_Utils_Print("Successfully executed the request!\n");
                *appState = DemoAppState_Deactivate;
            }
        }
    }

    return status;
}

static ptx_Status_t ptx_ProcessState_Deactivate(ptx_RUL_SnepClient_t *ctx, ptx_DemoAppState_t *appState)
{
    ptx_Status_t status = PTX_STATUS_E_GEN_INVALID_PARAMETER;

    if ((NULL != ctx) && (NULL != appState))
    {
        ptx_RUL_DeactivateParams_t deact_params;
        deact_params.Type = ptx_RUL_DeactivateType_Generic;
        deact_params.State = ptx_RUL_DeactivateState_Idle;
        status = ptx_RUL_Deactivate(ctx->Llcp->Rul, &deact_params);

        if (status == PTX_STATUS_SUCCESS)
        {
            *appState = DemoAppState_RestartSystem;
        }
    }

    return status;
}

static ptx_Status_t ptx_ProcessState_InitiatePutRequest(ptx_RUL_SnepClient_t *ctx, ptx_DemoAppState_t *appState)
{
    ptx_Status_t status = PTX_STATUS_E_GEN_INVALID_PARAMETER;

    if ((NULL != ctx) && (NULL != appState))
    {
        status = ptx_RUL_SnepClient_PutMessage(ctx, (uint8_t*) normal_record, sizeof(normal_record));

        if (status == PTX_STATUS_SUCCESS)
        {
            ptx_Utils_Print("Successfully initiated a PUT request!\n");
            *appState = DemoAppState_Process;
        }
    }

    return status;
}

static ptx_Status_t ptx_ProcessState_InitiateGetRequest(ptx_RUL_SnepClient_t *ctx, ptx_DemoAppState_t *appState)
{
    ptx_Status_t status = PTX_STATUS_E_GEN_INVALID_PARAMETER;

    if ((NULL != ctx) && (NULL != appState))
    {
        status = ptx_RUL_SnepClient_GetMessage(ctx, (uint8_t*) normal_record, sizeof(normal_record));

        if (status == PTX_STATUS_SUCCESS)
        {
            ptx_Utils_Print("Successfully initiated a GET request!\n");
            *appState = DemoAppState_Process;
        }
    }

    return status;
}

static ptx_Status_t ptx_ProcessState_SystemError(ptx_RUL_SnepClient_t *ctx, ptx_DemoAppState_t *appState)
{
    ptx_Status_t status = PTX_STATUS_E_GEN_INVALID_PARAMETER;

    if ((NULL != ctx) && (NULL != appState))
    {
        ptx_RUL_Status_Params_t status_params = {0};
        status_params.Type = ptx_RUL_StatusType_System;
        if (NULL != ctx->Llcp)
        {
            status = ptx_RUL_GetStatusInfo(ctx->Llcp->Rul, &status_params);
        }

        /* System-Errors-/-Warnings are critical and needs to be resolved at integration-/application-level.
         * There can be multiple root causes those errors/warnings, but in most cases they are related
         * the Hardware-environment (e.g. PCB, component issues etc.).
         */
        if (ptx_RUL_StatusInfoSystem_ErrorOvercurrent == status_params.Info.System)
        {
            ptx_Utils_Print("WARNING - Overcurrent Limiter activated!\n");
        }

        if (ptx_RUL_StatusInfoSystem_ErrorTemperature == status_params.Info.System)
        {
            ptx_Utils_Print("WARNING - Temperature Limiter activated!\n");
        }

        if (ptx_RUL_StatusInfoSystem_ErrorOutOfMemory == status_params.Info.System)
        {
            ptx_Utils_Print("WARNING - Out of Memory!\n");
            gExit = 1u;
        }

        if (ptx_RUL_StatusInfoSystem_NSCReset == status_params.Info.System)
        {
            ptx_Utils_Print("WARNING - Unexpected System Reset - exiting Application!\n");
        }

        *appState = DemoAppState_RestartSystem;
    }

    return status;
}

static ptx_Status_t ptx_ProcessState_RestartSystem(ptx_RUL_t *rul, ptx_RUL_LLCP_t *llcp, ptx_RUL_SnepClient_t *snep, ptx_DemoAppState_t *appState)
{
    ptx_Status_t status = PTX_STATUS_E_GEN_INVALID_PARAMETER;

    if ((NULL != rul) && (NULL != llcp) && (NULL != snep) && (NULL != appState))
    {
        status = ptx_RUL_SnepClient_DeInit(snep);

        if (PTX_STATUS_SUCCESS == status)
        {
            status = ptx_RUL_LLCP_DeInit(llcp);
        }

        if (PTX_STATUS_SUCCESS == status)
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
        }

        if (PTX_STATUS_SUCCESS == status)
        {
            *appState = DemoAppState_Initialize;
        }
    }

    return status;
}

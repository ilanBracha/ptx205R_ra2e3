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
    Module      : SDK Example (EMV POS-DTE Loopback Application)
    File        : EXAMPLES/ptx_Example_EMVPosDteLoopback/ptx_ExampleApplication.c

    Description : This application example demonstrates how to use / execute the EMV Loopback application
                  which is specified in the document "EMV Contactless Terminal Level 1 Type Approval -
                  Device Test Environment" from EMVCo using the "POS-DTE" Add-on API on top of the RUL-API.
                  The EMV Loopback application is used for for EMV L1 digital certification / compliance / type-approval testing.
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
#include "ptx_POS_DTE.h"
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

/* Enable (1) or Disable (0) EMV Interoperability (IOP) Loopback mode.
 *
 * The EMV IOP Loopback mode is the same as the standard EMV Loopback mode with the exception that
 * the IOP implementation may call additional Callback-functions to inform the application about certain execution states such as
 * - Loopback Test PASS,
 * - Loopback Test Error / Fail,
 * - Received APDU.
 *
 * Which mode to be used is defined by the Laboratory during Type-Approval / Compliance / Certification testing.
 * Under normal circumstances, the EMV Loopback application under test shall execute as fast as possible and therefore the internal
 * POS-DTE Loopback function does not perform an print-operations (or similar) unless the LOG is enabled.
 * For this demo application, IOP mode is used to have more application progress printed.
 */
#define PTX_DEMO_APP_USE_EMV_IOP_MODE                   (uint8_t)1

/* Enable (1) or Disable (0) DDPC (Digital Dynamic Power Control). */
#define PTX_DEMO_APP_ENABLE_DDPC                            (uint8_t)1
#define PTX_DEMO_APP_DISABLE_DDPC                           (uint8_t)0

/* Compile switch to enable or disable print-operations in demo- or compliance-testing mode.
 * It is recommended to disable / deactivate the switch when in compliance-testing mode because the printf-statements might impact timings.
 */
#define PTX_DEMO_APP_COMPLIANCE_TESTING

/* Application specific Exit-Flag (controllable by the user on Windows-/Linux-platforms via Ctrl-C). */
static volatile int gExit = 0;

#if defined(PTX_ENABLE_BOARD_EVK)
    /* Corresponds to the voltage on the DC/DC converter on the PTX2xxW Boards. E.g. a value of 330 corresponds to 3.3V. */
    static const uint16_t EVK_DEFAULT_VOLTAGE = 500u;
#endif

/* Print current EMV Loopback state via Console/UART.
 *
 * Attention: If MCU-target and UART is used - this produces a lot of traffic on the serial line.
 *            Please comment line if not needed.
 */
// #define PTX_DEMO_APP_PRINT_LOOPBACK_STATE

/*
 * ####################################################################################################################
 * INTERNAL FUNCTIONS DECLARATIONS / HELPERS
 * ####################################################################################################################
 */
static ptx_Status_t ptx_RunDemoApplication(ptx_RUL_t *rul);
static void ptx_EMVLoopbackDemo_CbSuccess(void);
static void ptx_EMVLoopbackDemo_CbError(void);
static void ptx_EMVLoopbackDemo_CbRxAPDU(uint8_t* data, uint32_t dataLength);
#if defined(PTX_DEMO_APP_PRINT_LOOPBACK_STATE)
    static void ptx_EMVLoopbackDemo_PrintState(ptx_POS_DTE_LoopbackState_t lbState);
#endif
static ptx_Status_t ptx_ProcessState_SystemError(ptx_POS_DTE_t *posDTE, ptx_RUL_Status_Params_t *statusInfoParam);
static ptx_Status_t ptx_ProcessState_RestartSystem(ptx_POS_DTE_t *posDTE);


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
    ptx_Utils_Print("****   EMV POS-DTE EMV (IOP) Loopback Demo %s              ****\n", PTX_DEMO_VERSION);
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

    ptx_RUL_Status_Params_t status_info = {0};

    /* POS-DTE Add-on API component */
    ptx_POS_DTE_t pos_dte = {0};
    ptx_POS_DTE_InitParams_t init_params = {0};

#if defined(PTX_DEMO_APP_PRINT_LOOPBACK_STATE)
    /* POS-DTE Support parameters. */
    ptx_POS_DTE_LoopbackState_t lb_state;
#endif

    if (NULL != rul)
    {
        /* Initialize POS-DTE Add-on API component.
        *
        * Note:
        *   -- "IgnoreRFError":     Can be used to filter RF-errors during testing which can happen if the system is close to it's operating volume.
        *                           If filtered, RF-errors are still detected internally, but are not forwarded to the Application.
        *   -- "IopLoopback"        True/False -> see description of "PTX_DEMO_APP_USE_EMV_IOP_MODE" above.
        *   -- "CbSuccess"          Optional callback to inform application whether current EMV Loopback run passed. Only active if "PTX_DEMO_APP_USE_EMV_IOP_MODE" is set.
        *   -- "CbError"            Optional callback to inform application whether current EMV Loopback run failed. Only active if "PTX_DEMO_APP_USE_EMV_IOP_MODE" is set.
        *   -- "CbApdu"             Optional callback to forward received APDU data from the card/tester to the Application. Only active if "PTX_DEMO_APP_USE_EMV_IOP_MODE" is set.
        */
        init_params.Rul = rul;
        init_params.IgnoreRFError = 1u;
        init_params.IopLoopback = PTX_DEMO_APP_USE_EMV_IOP_MODE;
        init_params.EnableDDPC = PTX_DEMO_APP_ENABLE_DDPC;
        init_params.CbSuccess = &ptx_EMVLoopbackDemo_CbSuccess;
        init_params.CbError = &ptx_EMVLoopbackDemo_CbError;
        init_params.CbApdu = &ptx_EMVLoopbackDemo_CbRxAPDU;

        status = ptx_POS_DTE_Init(&pos_dte, &init_params);

        if (PTX_STATUS_SUCCESS == status)
        {
            ptx_Utils_Print("POS-DTE API successfully initialized!\n");

            do
            {
                /* Check regularly for System-state/-health (e.g. temperature/over-current errors etc.) */
                status_info.Type = ptx_RUL_StatusType_System;

                status = ptx_RUL_GetStatusInfo(rul, &status_info);

                if (PTX_STATUS_SUCCESS == status)
                {
                    if (ptx_RUL_StatusInfoSystem_OK == status_info.Info.System)
                    {
                        /* Call the EMV Loopback function.
                        *
                        * Attention:
                        *   The EMV Loopback function is implemented in a reentrant manner and is based on an internal state-machine.
                        *   It needs to be called as often as needed by the Application (typically in an endless-loop during testing)
                        *   and gives the Application full control.
                        *
                        *                       If filtered, RF-errors are still detected internally, but are not forwarded to the Application.
                        *   -- "IopLoopback"    True/False -> see description of "PTX_DEMO_APP_USE_EMV_IOP_MODE" above.
                        *   -- "CbSuccess"      Optional callback to inform application whether current EMV Loopback run passed. Only active if "PTX_DEMO_APP_USE_EMV_IOP_MODE" is set.
                        *   -- "CbError"        Optional callback to inform application whether current EMV Loopback run failed. Only active if "PTX_DEMO_APP_USE_EMV_IOP_MODE" is set.
                        *   -- "CbApdu"         Optional callback to forward received APDU data from the card/tester to the Application. Only active if "PTX_DEMO_APP_USE_EMV_IOP_MODE" is set.
                        */
                        status = ptx_POS_DTE_Loopback(&pos_dte, ptx_POS_DTE_TargetRFTech_A_B);

                        /* The call to get the current state is optional and may be called by the Application to get the current internal state. */
                        if (PTX_STATUS_SUCCESS == status)
                        {
#if defined(PTX_DEMO_APP_PRINT_LOOPBACK_STATE)
                            status = ptx_POS_DTE_GetLoopbackState(&pos_dte, &lb_state);

                            if (PTX_STATUS_SUCCESS == status)
                            {
                                ptx_EMVLoopbackDemo_PrintState(lb_state);
                            }
#endif
                        }
                    }
                    else
                    {
                        status = ptx_ProcessState_SystemError(&pos_dte, &status_info);
                    }
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
                    status = ptx_ProcessState_SystemError(&pos_dte, &status_info);
                }

            }
            while ((PTX_STATUS_SUCCESS == status) && (0 == gExit));


            /* Stop the EMV Loopback Application. */
            (void)ptx_POS_DTE_Loopback_Stop(&pos_dte);

            /* De-initialize the POS-DTE API Component. */
            status = ptx_POS_DTE_DeInit(&pos_dte);
        }
        else
        {
            ptx_Utils_Print("POS-DTE API initialization failed with Error-Status %04X!\n", status);
        }

        ptx_Utils_Print("Exiting Demo-Application...\n");

    }

    return status;
}

static void ptx_EMVLoopbackDemo_CbSuccess(void)
{
#if !defined(PTX_DEMO_APP_COMPLIANCE_TESTING)
    ptx_Utils_Print("EMV Loopback Application Run PASSED\n");
#endif
}

static void ptx_EMVLoopbackDemo_CbError(void)
{
#if !defined(PTX_DEMO_APP_COMPLIANCE_TESTING)
    ptx_Utils_Print("EMV Loopback Application Run FAILED with Errors.\n");
#endif
}

static void ptx_EMVLoopbackDemo_CbRxAPDU(uint8_t* data, uint32_t dataLength)
{
#if !defined(PTX_DEMO_APP_COMPLIANCE_TESTING)
    ptx_Utils_Print_Buffer(data, (uint16_t)dataLength, 1u, 0);
#endif

    (void)data;
    (void)dataLength;
}

#if defined(PTX_DEMO_APP_PRINT_LOOPBACK_STATE)
static void ptx_EMVLoopbackDemo_PrintState(ptx_POS_DTE_LoopbackState_t lbState)
{
    switch (lbState)
    {
        case ptx_POS_DTE_State_StartPolling:
            ptx_Utils_Print("ptx_POS_DTE_State_StartPolling\n");
            break;
        case ptx_POS_DTE_State_WaitForActivation:
            ptx_Utils_Print("ptx_POS_DTE_State_WaitForActivation\n");
            break;
        case ptx_POS_DTE_State_ExchangePPSE:
            ptx_Utils_Print("ptx_POS_DTE_State_ExchangePPSE\n");
            break;
        case ptx_POS_DTE_State_ExchangeAPDU:
            ptx_Utils_Print("ptx_POS_DTE_State_ExchangeAPDU\n");
            break;
        case ptx_POS_DTE_State_StartCardRemoval:
            ptx_Utils_Print("ptx_POS_DTE_State_StartCardRemoval\n");
            break;
        case ptx_POS_DTE_State_WaitCardRemoval:
            ptx_Utils_Print("ptx_POS_DTE_State_WaitCardRemoval\n");
            break;
        case ptx_POS_DTE_State_RestartPolling:
            ptx_Utils_Print("ptx_POS_DTE_State_RestartPolling\n");
            break;
        case ptx_POS_DTE_State_DeactivateReader:
            ptx_Utils_Print("ptx_POS_DTE_State_DeactivateReader\n");
            break;
        case ptx_POS_DTE_State_SystemError:
            ptx_Utils_Print("ptx_POS_DTE_State_SystemError\n");
            break;
        case ptx_POS_DTE_State_Exit:
            ptx_Utils_Print("ptx_POS_DTE_State_Exit\n");
            break;
        default:
            ptx_Utils_Print("UNKNOWN STATE\n");
            break;   /* SHALL NEVER HAPPEN! */
    }
}
#endif

static ptx_Status_t ptx_ProcessState_SystemError(ptx_POS_DTE_t *posDTE, ptx_RUL_Status_Params_t *statusInfoParam)
{
    ptx_Status_t status = PTX_STATUS_E_GEN_INVALID_PARAMETER;

    if ((NULL != posDTE) && (NULL != statusInfoParam))
    {
        status = PTX_STATUS_SUCCESS;

        /* System-Errors-/-Warnings are critical and needs to be resolved at integration-/application-level.
         * There can be multiple root causes those errors/warnings, but in most cases they are related
         * the Hardware-environment (e.g. PCB, component issues etc.).
         */
        if (ptx_RUL_StatusInfoSystem_ErrorOvercurrent == statusInfoParam->Info.System)
        {
            ptx_Utils_Print("WARNING - Overcurrent Limiter activated!\n");
            gExit = 1u;
        }

        if (ptx_RUL_StatusInfoSystem_ErrorTemperature == statusInfoParam->Info.System)
        {
            ptx_Utils_Print("WARNING - Temperature Limiter activated!\n");
            status = ptx_ProcessState_RestartSystem(posDTE);
        }

        if (ptx_RUL_StatusInfoSystem_ErrorOutOfMemory == statusInfoParam->Info.System)
        {
            ptx_Utils_Print("WARNING - Out of Memory!\n");
            gExit = 1u;
        }

        if (ptx_RUL_StatusInfoSystem_NSCReset == statusInfoParam->Info.System)
        {
            ptx_Utils_Print("WARNING - Unexpected System Reset - exiting Application!\n");
            gExit = 1u;
        }
    }

    return status;
}

static ptx_Status_t ptx_ProcessState_RestartSystem(ptx_POS_DTE_t *posDTE)
{
    ptx_Status_t status = PTX_STATUS_E_GEN_INVALID_PARAMETER;

    if ((NULL != posDTE))
    {
        do
        {
            /* Wait for 1s to give the chip time to cool down, after that time try to initialize the system again */
            status = ptx_RUL_Sleep(posDTE->Rul, 1000u);

            if (PTX_STATUS_SUCCESS == status)
            {
                status = ptx_RUL_InitHardware(posDTE->Rul);
            }

        }
        while (status != PTX_STATUS_SUCCESS);

        if (PTX_STATUS_SUCCESS == status)
        {
            status = ptx_POS_DTE_ResetState(posDTE);
        }
    }

    return status;
}
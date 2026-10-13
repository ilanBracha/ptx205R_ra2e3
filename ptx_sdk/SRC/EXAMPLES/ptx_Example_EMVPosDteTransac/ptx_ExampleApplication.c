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
    Module      : SDK Example (EMV POS-DTE TRANSAC_A/_B)
    File        : EXAMPLES/ptx_Example_EMVPosDteTransac/ptx_ExampleApplication.c

    Description : This application example demonstrates how to use / execute the EMV TRANSAC_A and/or _B application
                  which is specified in the document "EMV Contactless Terminal Level 1 Type Approval -
                  Device Test Environment" from EMVCo using the "POS-DTE" Add-on API on top of the RUL-API.
                  The EMV TRANSAC_A/_B application is used for for EMV L1 analog certification / compliance / type-approval testing.
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

/* Select TRANSAC-Mode for current demo application.
 * If #define is set, TRANSAC_A will be used - if it's not set / undefined, TRANSAC_B will be used.
 *
 * Which mode to be used is defined by the actual EMV testcase.
 */
#define PTX_DEMO_APP_USE_EMV_TRANSAC_A

/* Application specific Exit-Flag (controllable by the user on Windows-/Linux-platforms via Ctrl-C). */
static volatile int gExit = 0;

#if defined(PTX_ENABLE_BOARD_EVK)
    /* Corresponds to the voltage on the DC/DC converter on the PTX2xxW Boards. E.g. a value of 330 corresponds to 3.3V. */
    static const uint16_t EVK_DEFAULT_VOLTAGE = 500u;
#endif

/* Print current EMV TRANSAC_x state via Console/UART.
 *
 * Attention: If MCU-target and UART is used - this produces a lot of traffic on the serial line.
 *            Please comment line if not needed.
 */
#define PTX_DEMO_APP_PRINT_TRANSAC_STATE

/* Enable (1) or Disable (0) DDPC (Digital Dynamic Power Control). */
#define PTX_DEMO_APP_ENABLE_DDPC                            (uint8_t)1
#define PTX_DEMO_APP_DISABLE_DDPC                           (uint8_t)0

/*
 * ####################################################################################################################
 * INTERNAL FUNCTIONS DECLARATIONS / HELPERS
 * ####################################################################################################################
 */
static ptx_Status_t ptx_RunDemoApplication(ptx_RUL_t *rul);
#if defined(PTX_DEMO_APP_PRINT_TRANSAC_STATE)
    static void ptx_EMVTransacDemo_PrintState(ptx_POS_DTE_TransacA_State_t taaState, ptx_POS_DTE_TransacB_State_t tabState);
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
    ptx_Utils_Print("****   EMV POS-DTE EMV TRANSAC_A/_B Demo %s              ****\n", PTX_DEMO_VERSION);
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

#if defined(PTX_DEMO_APP_PRINT_TRANSAC_STATE)
    /* POS-DTE Support parameters. */
    ptx_POS_DTE_TransacA_State_t taa_state = {0};
    ptx_POS_DTE_TransacB_State_t tab_state = {0};
#endif

    if (NULL != rul)
    {
        /* Initialize POS-DTE Add-on API component.
        *
        * Note: The initialization parameter structure contains a lot more parameters which could be used, but they are not needed
        *       for the low-level POS-DTE commands - they are only needed when the EMV Loopback-function is used/called
        *       (available as separate demo application).
        *       Only a pointer to a RUL reference instance is needed to use the TRANSAC_A/_B POS-DTE commands.
        */
        init_params.Rul = rul;
        init_params.EnableDDPC = PTX_DEMO_APP_DISABLE_DDPC;

        status = ptx_POS_DTE_Init(&pos_dte, &init_params);

        if (PTX_STATUS_SUCCESS == status)
        {
            ptx_Utils_Print("POS-DTE API successfully initialized!\n");

            /* Turn RF-field / Carrier on */
            status = ptx_POS_DTE_Carrier(&pos_dte, ptx_POS_DTE_Carrier_ON);

            if (PTX_STATUS_SUCCESS == status)
            {
                do
                {
                    /* Check regularly for System-state/-health (e.g. temperature/over-current errors etc.) */
                    status_info.Type = ptx_RUL_StatusType_System;

                    status = ptx_RUL_GetStatusInfo(rul, &status_info);

                    if (PTX_STATUS_SUCCESS == status)
                    {
                        if (ptx_RUL_StatusInfoSystem_OK == status_info.Info.System)
                        {
                            /* Call the EMV TRANSAC_A or _B function.
                            *
                            * Attention:
                            *   The EMV TRANSAC_x functions are implemented in a reentrant manner and both are based on an internal state-machine.
                            *   It needs to be called as often as needed by the Application (typically in an endless-loop during testing)
                            *   and gives the Application full control.
                            */
#if defined(PTX_DEMO_APP_USE_EMV_TRANSAC_A)
                            status = ptx_POS_DTE_TransacA(&pos_dte);

                            /* The call to get the current state is optional and may be called by the Application to get the current internal state. */
                            if (PTX_STATUS_SUCCESS == status)
                            {
                                status = ptx_POS_DTE_GetTransacAState(&pos_dte, &taa_state);
                            }
#else
                            status = ptx_POS_DTE_TransacB(&pos_dte);

                            /* The call to get the current state is optional and may be called by the Application to get the current internal state. */
                            if (PTX_STATUS_SUCCESS == status)
                            {
                                status = ptx_POS_DTE_GetTransacBState(&pos_dte, &tab_state);
                            }
#endif

#if defined(PTX_DEMO_APP_PRINT_TRANSAC_STATE)
                            ptx_EMVTransacDemo_PrintState(taa_state, tab_state);
#endif
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
            }

            /* Stop the EMV TRANSAC_A/_B Application. */
            (void)ptx_POS_DTE_Transac_Stop(&pos_dte);

            /* Turn RF-field / Carrier off */
            (void)ptx_POS_DTE_Carrier(&pos_dte, ptx_POS_DTE_Carrier_OFF);

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

static void ptx_EMVTransacDemo_PrintState(ptx_POS_DTE_TransacA_State_t taaState, ptx_POS_DTE_TransacB_State_t tabState)
{
#if defined(PTX_DEMO_APP_USE_EMV_TRANSAC_A)
    switch (taaState)
    {
        case ptx_POS_DTE_TransacA_Init:
            ptx_Utils_Print("ptx_POS_DTE_TransacA_Init\n");
            break;
        case ptx_POS_DTE_TransacA_CMD_WUPA_1:
            ptx_Utils_Print("ptx_POS_DTE_TransacA_CMD_WUPA_1\n");
            break;
        case ptx_POS_DTE_TransacA_CMD_HLTA:
            ptx_Utils_Print("ptx_POS_DTE_TransacA_CMD_HLTA\n");
            break;
        case ptx_POS_DTE_TransacA_CMD_WUPB:
            ptx_Utils_Print("ptx_POS_DTE_TransacA_CMD_WUPB\n");
            break;
        case ptx_POS_DTE_TransacA_CMD_WUPA_2:
            ptx_Utils_Print("ptx_POS_DTE_TransacA_CMD_WUPA_2\n");
            break;
        case ptx_POS_DTE_TransacA_CMD_ANTICOLLISION:
            ptx_Utils_Print("ptx_POS_DTE_TransacA_CMD_ANTICOLLISION\n");
            break;
        case ptx_POS_DTE_TransacA_CMD_SELECT:
            ptx_Utils_Print("ptx_POS_DTE_TransacA_CMD_SELECT\n");
            break;
        case ptx_POS_DTE_TransacA_CMD_RATS:
            ptx_Utils_Print("ptx_POS_DTE_TransacA_CMD_RATS\n");
            break;
        case ptx_POS_DTE_TransacA_CMD_IBLOCK_1:
            ptx_Utils_Print("ptx_POS_DTE_TransacA_CMD_IBLOCK_1\n");
            break;
        case ptx_POS_DTE_TransacA_CMD_IBLOCK_2:
            ptx_Utils_Print("ptx_POS_DTE_TransacA_CMD_IBLOCK_2\n");
            break;
        case ptx_POS_DTE_TransacA_END:
            ptx_Utils_Print("ptx_POS_DTE_TransacA_END\n");
            break;
        default:
            ptx_Utils_Print("UNKNOWN STATE\n");
            break;   /* SHALL NEVER HAPPEN! */
    }
#else
    switch (tabState)
    {
        case ptx_POS_DTE_TransacB_Init:
            ptx_Utils_Print("ptx_POS_DTE_TransacB_Init\n");
            break;
        case ptx_POS_DTE_TransacB_CMD_WUPB_1:
            ptx_Utils_Print("ptx_POS_DTE_TransacB_CMD_WUPB_1\n");
            break;
        case ptx_POS_DTE_TransacB_CMD_WUPA:
            ptx_Utils_Print("ptx_POS_DTE_TransacB_CMD_WUPA\n");
            break;
        case ptx_POS_DTE_TransacB_CMD_WUPB_2:
            ptx_Utils_Print("ptx_POS_DTE_TransacB_CMD_WUPB_2\n");
            break;
        case ptx_POS_DTE_TransacB_CMD_ATTRIB:
            ptx_Utils_Print("ptx_POS_DTE_TransacB_CMD_ATTRIB\n");
            break;
        case ptx_POS_DTE_TransacB_CMD_IBLOCK_1:
            ptx_Utils_Print("ptx_POS_DTE_TransacB_CMD_IBLOCK_1\n");
            break;
        case ptx_POS_DTE_TransacB_CMD_IBLOCK_2:
            ptx_Utils_Print("ptx_POS_DTE_TransacB_CMD_IBLOCK_2\n");
            break;
        case ptx_POS_DTE_TransacB_END:
            ptx_Utils_Print("ptx_POS_DTE_TransacB_END\n");
            break;
        default:
            ptx_Utils_Print("UNKNOWN STATE\n");
            break;   /* SHALL NEVER HAPPEN! */
    }
#endif

    PTX_UNUSED_PARAMETER(taaState);
    PTX_UNUSED_PARAMETER(tabState);
}

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
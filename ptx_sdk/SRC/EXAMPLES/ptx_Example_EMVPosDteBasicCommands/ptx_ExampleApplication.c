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
    Module      : SDK Example (EMV POS-DTE Basic Commands)
    File        : EXAMPLES/ptx_Example_EMVPosDteBasicCommands/ptx_ExampleApplication.c

    Description : This application example demonstrates how to use the low-level POS-DTE commands
                  which are specified in the document "EMV Contactless Terminal Level 1 Type Approval -
                  Device Test Environment" from EMVCo using the "POS-DTE" Add-on API on top of the RUL-API.
                  The example code demonstrates the usage of both contactless RF-technologies A and B.
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

/* Work-buffer max. sizes for current demo example. */
#define PTX_DEMO_APP_EXAMPLE_WORK_BUFFER_SIZE       128

/* Application specific Exit-Flag (controllable by the user on Windows-/Linux-platforms via Ctrl-C). */
static volatile int gExit = 0;

#if defined(PTX_ENABLE_BOARD_EVK)
    /* Corresponds to the voltage on the DC/DC converter on the PTX2xxW Boards. E.g. a value of 330 corresponds to 3.3V. */
    static const uint16_t EVK_DEFAULT_VOLTAGE = 500u;
#endif

/* Delay/Sleep after RF-Field == On. */
static const uint32_t PTX_DEMO_APP_FIELD_ON_DELAY = 10u;

/* Artificial Application Delay between states to get a human-readable i.e. slower output on the console. NOT NEEDED in real Application! */
static const uint32_t PTX_DEMO_APP_STATE_DELAY = 100u;

/* Enable (1) or Disable (0) DDPC (Digital Dynamic Power Control). */
#define PTX_DEMO_APP_ENABLE_DDPC                            (uint8_t)1
#define PTX_DEMO_APP_DISABLE_DDPC                           (uint8_t)0

/*
 * ####################################################################################################################
 * INTERNAL FUNCTIONS DECLARATIONS / HELPERS
 * ####################################################################################################################
 */
static ptx_Status_t ptx_RunDemoApplication(ptx_RUL_t *rul);
static ptx_Status_t ptx_ProcessState_FieldOn(ptx_POS_DTE_t *posDTE, ptx_DemoAppState_t *appState);
static ptx_Status_t ptx_ProcessState_FieldReset(ptx_POS_DTE_t *posDTE, ptx_DemoAppState_t *appState);
static ptx_Status_t ptx_ProcessState_ExchangeWUPA(ptx_POS_DTE_t *posDTE, ptx_DemoAppState_t *appState);
static ptx_Status_t ptx_ProcessState_ExchangeRATS(ptx_POS_DTE_t *posDTE, ptx_DemoAppState_t *appState);
static ptx_Status_t ptx_ProcessState_ExchangeWUPB(ptx_POS_DTE_t *posDTE, ptx_DemoAppState_t *appState);
static ptx_Status_t ptx_ProcessState_ExchangeATTRIB(ptx_POS_DTE_t *posDTE, ptx_DemoAppState_t *appState);
static ptx_Status_t ptx_ProcessState_SystemError(ptx_POS_DTE_t *posDTE, ptx_DemoAppState_t *appState, ptx_RUL_Status_Params_t *statusInfoParam);
static ptx_Status_t ptx_ProcessState_RestartSystem(ptx_POS_DTE_t *posDTE, ptx_DemoAppState_t *appState);

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
    ptx_Utils_Print("****   EMV POS-DTE Basic Commands Demo %s              ****\n", PTX_DEMO_VERSION);
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

    ptx_DemoAppState_t demo_state = DemoAppState_SwitchOnField;
    ptx_RUL_Status_Params_t status_info = {0};

    /* POS-DTE Add-on API component */
    ptx_POS_DTE_t pos_dte = {0};
    ptx_POS_DTE_InitParams_t init_params = {0};

    if (NULL != rul)
    {
        /* Initialize POS-DTE Add-on API component.
        *
        * Note: The initialization parameter structure contains a lot more parameters which could be used, but they are not needed
        *       for the low-level POS-DTE commands - they are only needed when the EMV Loopback-function is used/called
        *       (available as separate demo application).
        *       Only a pointer to a RUL reference instance is needed to use the low-level POS-DTE commands.
        */
        init_params.Rul = rul;
        init_params.EnableDDPC = PTX_DEMO_APP_DISABLE_DDPC;

        status = ptx_POS_DTE_Init(&pos_dte, &init_params);

        if (PTX_STATUS_SUCCESS == status)
        {
            ptx_Utils_Print("POS-DTE API successfully initialized!\n");

            do
            {
                /* Artificial delay to slow down console output and make output readable. */
                (void)ptx_RUL_Sleep(rul, PTX_DEMO_APP_STATE_DELAY);

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
                        case DemoAppState_SwitchOnField:
                        {
                            status = ptx_ProcessState_FieldOn(&pos_dte, &demo_state);
                            break;
                        }

                        case DemoAppState_ExchangeWUPA:
                        {
                            status = ptx_ProcessState_ExchangeWUPA(&pos_dte, &demo_state);
                            break;
                        }

                        case DemoAppState_ExchangeRATS:
                        {
                            status = ptx_ProcessState_ExchangeRATS(&pos_dte, &demo_state);
                            break;
                        }

                        case DemoAppState_ExchangeWUPB:
                        {
                            status = ptx_ProcessState_ExchangeWUPB(&pos_dte, &demo_state);
                            break;
                        }

                        case DemoAppState_ExchangeATTRIB:
                        {
                            status = ptx_ProcessState_ExchangeATTRIB(&pos_dte, &demo_state);
                            break;
                        }

                        case DemoAppState_ResetField:
                        {
                            status = ptx_ProcessState_FieldReset(&pos_dte, &demo_state);
                            break;
                        }

                        case DemoAppState_SystemError:
                        {
                            status = ptx_ProcessState_SystemError(&pos_dte, &demo_state, &status_info);
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

                /*
                 * NOTE: The following error handling is required for this demo application only! Please use your own
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


            /* Turn Carrier-RF-field off regardless of it's internal state */
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

static ptx_Status_t ptx_ProcessState_FieldOn(ptx_POS_DTE_t *posDTE, ptx_DemoAppState_t *appState)
{
    ptx_Status_t status = PTX_STATUS_E_GEN_INVALID_PARAMETER;

    if ((NULL != posDTE) && (NULL != appState))
    {
        /* Independent of RF-technology A and B, the Carrier i.e. the RF-field must be turned on as a first step. */
        status = ptx_POS_DTE_Carrier(posDTE, ptx_POS_DTE_Carrier_ON);

        if (PTX_STATUS_SUCCESS == status)
        {
            ptx_Utils_Print("RF-Field / Carrier switched on successfully!\n");
            *appState = DemoAppState_ExchangeWUPA;
        }
        else
        {
            ptx_Utils_Print("Switching RF-Field / Carrier on failed with Error-Status %04X!\n", status);
        }
    }

    return status;
}

static ptx_Status_t ptx_ProcessState_FieldReset(ptx_POS_DTE_t *posDTE, ptx_DemoAppState_t *appState)
{
    ptx_Status_t status = PTX_STATUS_E_GEN_INVALID_PARAMETER;

    if ((NULL != posDTE) && (NULL != appState))
    {
        /* Reset Carrier/RF-field for next loop run. */
        status = ptx_POS_DTE_Reset(posDTE);

        if (PTX_STATUS_SUCCESS == status)
        {
            ptx_Utils_Print("RF-Field / Carrier switched on successfully!\n");
            *appState = DemoAppState_ExchangeWUPA;
        }
        else
        {
            ptx_Utils_Print("Reseting RF-Field / Carrier on failed with Error-Status %04X!\n", status);
        }
    }

    return status;
}

static ptx_Status_t ptx_ProcessState_ExchangeWUPA(ptx_POS_DTE_t *posDTE, ptx_DemoAppState_t *appState)
{
    ptx_Status_t status = PTX_STATUS_E_GEN_INVALID_PARAMETER;

    uint8_t rx[PTX_DEMO_APP_EXAMPLE_WORK_BUFFER_SIZE] = {0};
    uint32_t rx_len = 0;

    if ((NULL != posDTE) && (NULL != appState))
    {
        /* RF-technology A and B require a Card to wait at least 5 ms (we use 10 in this example) until
         * - the first command can be sent after the field was turned on or
         + - between the consecutive calls of different RF-technologies (e.g. switch from A to B).
        */
        status = ptx_RUL_Sleep(posDTE->Rul, PTX_DEMO_APP_FIELD_ON_DELAY);

        if (PTX_STATUS_SUCCESS == status)
        {
            /*
            * Send a WUP-A command according to ISO 14443-3A.
            *
            * Note: Providing parameters "rx" and "rx_len" is optional.
            *       They might be useful if the application is interested in the actual response data of the card; can be omitted if not needed.
            */
            status = ptx_POS_DTE_WupA(posDTE, &rx[0], &rx_len);

            if (PTX_STATUS_SUCCESS == status)
            {
                ptx_Utils_Print("Type-A Card found! ATQA/SENS_RES: ");
                ptx_Utils_Print_Buffer(&rx[0], (uint16_t)rx_len, 1u, 0);

                /*
                * Send another WUP-A to reset the card's internal state (see ISO/IEC 14443-3A).
                * This is only needed here for the demo application to make the following RATS-command working because the RATS-command
                * itself performs another WUP-A and this would fail because the card changed it's internal state.
                */
                (void)ptx_POS_DTE_WupA(posDTE, NULL, NULL);

                *appState = DemoAppState_ExchangeRATS;
            }
            else
            {
                ptx_Utils_Print("Executing WUP-A failed with Error-Status %04X!\n", status);

                /*
                 * Timeouts are expected if there is no card present or RF-transmissions errors can also happen - that's OK;
                 * Just reset the status and look out for Type-B.
                 */
                status = ptx_Utils_Check_Rf_Error(status);

                if (PTX_STATUS_SUCCESS == status)
                {
                    *appState = DemoAppState_ExchangeWUPB;
                }
            }
        }
    }

    return status;
}

static ptx_Status_t ptx_ProcessState_ExchangeRATS(ptx_POS_DTE_t *posDTE, ptx_DemoAppState_t *appState)
{
    ptx_Status_t status = PTX_STATUS_E_GEN_INVALID_PARAMETER;

    uint8_t rx[PTX_DEMO_APP_EXAMPLE_WORK_BUFFER_SIZE] = {0};
    uint32_t rx_len = 0;

    if ((NULL != posDTE) && (NULL != appState))
    {
        /*
         * Perform a full Type-A card activation up to and including RATS.
         *
         * Note: A full Type-A card activation consists of the following Type-A NFC-/RF-commands:
         *       - WUPA/SENS_REQ (Wake-up A)
         *       - AntiCollision
         *       - Select
         *       - RATS (Request Answer To Select)
         *
         * Note: Providing parameters "rx" and "rx_len" is optional.
         *       They might be useful if the application is interested in the actual response data of the card; can be omitted if not needed.
         */
        status = ptx_POS_DTE_RATS(posDTE, &rx[0], &rx_len);

        if (PTX_STATUS_SUCCESS == status)
        {
            ptx_Utils_Print("Activation Parameters: ");
            ptx_Utils_Print_Buffer(&rx[0], (uint16_t)rx_len, 1u, 0);

            *appState = DemoAppState_ExchangeWUPB;
        }
        else
        {
            ptx_Utils_Print("Executing RATS failed with Error-Status %04X!\n", status);

            /*
            * Timeouts are expected if there is no card present or RF-transmissions errors can also happen - that's OK;
            * Just reset the status and look out for Type-B.
            */
            status = ptx_Utils_Check_Rf_Error(status);

            if (PTX_STATUS_SUCCESS == status)
            {
                *appState = DemoAppState_ExchangeWUPB;
            }

        }
    }

    return status;
}

static ptx_Status_t ptx_ProcessState_ExchangeWUPB(ptx_POS_DTE_t *posDTE, ptx_DemoAppState_t *appState)
{
    ptx_Status_t status = PTX_STATUS_E_GEN_INVALID_PARAMETER;

    uint8_t rx[PTX_DEMO_APP_EXAMPLE_WORK_BUFFER_SIZE] = {0};
    uint32_t rx_len = 0;

    if ((NULL != posDTE) && (NULL != appState))
    {
        /* RF-technology A and B require a Card to wait at least 5 ms (we use 10 in this example) until
         * - the first command can be sent after the field was turned on or
         + - between the consecutive calls of different RF-technologies (e.g. switch from A to B).
        */
        status = ptx_RUL_Sleep(posDTE->Rul, PTX_DEMO_APP_FIELD_ON_DELAY);

        if (PTX_STATUS_SUCCESS == status)
        {
            /*
            * Send a WUP-B command according to ISO 14443-3B.
            *
            * Note: Providing parameters "rx" and "rx_len" is optional.
            *       They might be useful if the application is interested in the actual response data of the card; can be omitted if not needed.
            *
            * Note: Providing parameters "rx" and "rx_len" is optional.
            *       They might be useful if the application is interested in the actual response data of the card; can be omitted if not needed.
            */
            status = ptx_POS_DTE_WupB(posDTE, &rx[0], &rx_len);

            if (PTX_STATUS_SUCCESS == status)
            {
                ptx_Utils_Print("Type-B Card found! ATQB/SENSB_RES: ");
                ptx_Utils_Print_Buffer(&rx[0], (uint16_t)rx_len, 1u, 0);

                /*
                * Send another WUP-A to reset the card's internal state (see ISO/IEC 14443-3B).
                * This is only needed here for the demo application to make the following Attrib-command working because the Attrib-command
                * itself performs another WUP-B and this would fail because the card changed it's internal state.
                */
                (void)ptx_POS_DTE_WupB(posDTE, NULL, NULL);

                *appState = DemoAppState_ExchangeATTRIB;
            }
            else
            {
                ptx_Utils_Print("Executing WUP-B failed with Error-Status %04X!\n", status);

                /*
                 * Timeouts are expected if there is no card present or RF-transmissions errors can also happen - that's OK;
                 * Just reset the status and start over.
                 */
                status = ptx_Utils_Check_Rf_Error(status);

                if (PTX_STATUS_SUCCESS == status)
                {
                    *appState = DemoAppState_ResetField;
                }
            }
        }
    }

    return status;
}

static ptx_Status_t ptx_ProcessState_ExchangeATTRIB(ptx_POS_DTE_t *posDTE, ptx_DemoAppState_t *appState)
{
    ptx_Status_t status = PTX_STATUS_E_GEN_INVALID_PARAMETER;

    uint8_t rx[PTX_DEMO_APP_EXAMPLE_WORK_BUFFER_SIZE] = {0};
    uint32_t rx_len = 0;

    if ((NULL != posDTE) && (NULL != appState))
    {
        /*
         * Perform a full Type-B card activation up to and including RATS.
         *
         * Note: A full Type-B card activation consists of the following Type-A NFC-/RF-commands:
         *       - WUPB/SENSB_REQ (Wake-up B)
         *       - Attrib
         *
         * Note: Providing parameters "rx" and "rx_len" is optional.
         *       They might be useful if the application is interested in the actual response data of the card; can be omitted if not needed.
         */
        status = ptx_POS_DTE_ATTRIB(posDTE, &rx[0], &rx_len);

        if (PTX_STATUS_SUCCESS == status)
        {
            ptx_Utils_Print("Activation Parameters: ");
            ptx_Utils_Print_Buffer(&rx[0], (uint16_t)rx_len, 1u, 0);

            *appState = DemoAppState_ExchangeWUPB;
        }
        else
        {
            ptx_Utils_Print("Executing ATTRIB failed with Error-Status %04X!\n", status);

            /*
                * Timeouts are expected if there is no card present or RF-transmissions errors can also happen - that's OK;
                * Just reset the status and start over.
                */
            status = ptx_Utils_Check_Rf_Error(status);

            if (PTX_STATUS_SUCCESS == status)
            {
                *appState = DemoAppState_ResetField;
            }
        }
    }

    return status;
}

static ptx_Status_t ptx_ProcessState_SystemError(ptx_POS_DTE_t *posDTE, ptx_DemoAppState_t *appState, ptx_RUL_Status_Params_t *statusInfoParam)
{
    ptx_Status_t status = PTX_STATUS_E_GEN_INVALID_PARAMETER;

    if ((NULL != posDTE) && (NULL != appState) && (NULL != statusInfoParam))
    {
        status = PTX_STATUS_SUCCESS;

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
            status = ptx_ProcessState_RestartSystem(posDTE, appState);
        }

        if (ptx_RUL_StatusInfoSystem_ErrorOutOfMemory == statusInfoParam->Info.System)
        {
            ptx_Utils_Print("WARNING - Out of Memory!\n");
            *appState = DemoAppState_Exit;
        }

        if (ptx_RUL_StatusInfoSystem_NSCReset == statusInfoParam->Info.System)
        {
            ptx_Utils_Print("WARNING - Unexpected System Reset - exiting Application!\n");
            *appState = DemoAppState_Exit;
        }
    }

    return status;
}

static ptx_Status_t ptx_ProcessState_RestartSystem(ptx_POS_DTE_t *posDTE, ptx_DemoAppState_t *appState)
{
    ptx_Status_t status = PTX_STATUS_E_GEN_INVALID_PARAMETER;

    if ((NULL != posDTE) && (NULL != appState))
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

        if (PTX_STATUS_SUCCESS == status)
        {
            *appState = DemoAppState_SwitchOnField;
        }
    }

    return status;
}
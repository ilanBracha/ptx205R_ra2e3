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
    Module      : SDK Example (Transparent Mode)
    File        : EXAMPLES/ptx_Example_TransparentMode/ptx_Example_TransparentMode.c

    Description : This application example demonstrates how to use the Transparent-Mode API.
                  It performs standard activation procedures for each technology type (A, B, F, V)
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
#include "ptx_TransparentMode.h"
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

/* Waiting time after a loop of the example. */
static const uint32_t PTX_EXAMPLE_TRANSPARENT_MODE_LOOP_SLEEP_MS = 1500u;

/* Maximum RF timeout. */
static const uint32_t PTX_EXAMPLE_TRANSPARENT_MODE_RF_TIMEOUT_MS = 100u;

/* Sleep time after turning field on (can be adjusted - depending on used RF-technology after field on). */
static const uint16_t PTX_EXAMPLE_TRANSPARENT_MODE_FIELD_ON_SLEEP_MS = 50u;

/* Required minimum sleep time after turning field off. */
static const uint16_t PTX_EXAMPLE_TRANSPARENT_MODE_FIELD_OFF_SLEEP_MS = 10u;

/* Maximum buffer size. */
#define PTX_EXAMPLE_TRANSPARENT_MODE_MAX_BUFFER_SIZE                    255u

/*
 * ####################################################################################################################
 * INTERNAL FUNCTIONS DECLARATIONS / HELPERS
 * ####################################################################################################################
 */

static ptx_Status_t ptx_RunDemoApplication(ptx_RUL_t *rul);
static ptx_Status_t ptx_Example_TransparentMode_SetStateOn(ptx_TransparentMode_t *tm, ptx_DemoAppState_t *appState);
static ptx_Status_t ptx_Example_TransparentMode_SetStateOff(ptx_TransparentMode_t *tm, ptx_DemoAppState_t *appState);
static ptx_Status_t ptx_Example_TransparentMode_DataExchangeTypeA(ptx_TransparentMode_t *tm, ptx_DemoAppState_t *appState);
static ptx_Status_t ptx_Example_TransparentMode_DataExchangeTypeB(ptx_TransparentMode_t *tm, ptx_DemoAppState_t *appState);
static ptx_Status_t ptx_Example_TransparentMode_DataExchangeTypeF(ptx_TransparentMode_t *tm, ptx_DemoAppState_t *appState);
static ptx_Status_t ptx_Example_TransparentMode_DataExchangeTypeV(ptx_TransparentMode_t *tm, ptx_DemoAppState_t *appState);
static ptx_Status_t ptx_ProcessState_SystemError(ptx_TransparentMode_t *tm, ptx_DemoAppState_t *appState, ptx_RUL_Status_Params_t *statusInfoParam);
static ptx_Status_t ptx_ProcessState_RestartSystem(ptx_TransparentMode_t *tm, ptx_DemoAppState_t *appState);

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
    ptx_Utils_Print("****   Transparent Mode Example %s             ****\n", PTX_DEMO_VERSION);
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

    ptx_DemoAppState_t demo_state = DemoAppState_SetStateOn;
    ptx_RUL_Status_Params_t status_info = {0};

    /* Transparent Mode Add-on API component */
    ptx_TransparentMode_t tm = {0};
    ptx_TransparentMode_InitParams_t init_params = {0};

    if (PTX_COMP_CHECK(rul, Comp_RUL))
    {
        init_params.Rul = rul;
        init_params.UseDefaultConfigForFieldOffState = 0;

        status = ptx_TransparentMode_Open(&tm, &init_params);
        ptx_Utils_Print("Opening Transparent Mode completed with Status %04X!\n", status);

        if (PTX_STATUS_SUCCESS == status)
        {
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
                        case DemoAppState_SetStateOn:
                        {
                            status = ptx_Example_TransparentMode_SetStateOn(&tm, &demo_state);
                            ptx_Utils_Print("Turning Field ON completed with Status %04X!\n", status);
                            (void)ptx_RUL_Sleep(rul, PTX_EXAMPLE_TRANSPARENT_MODE_FIELD_ON_SLEEP_MS);
                            break;
                        }

                        case DemoAppState_ExchangeA:
                        {
                            status = ptx_Example_TransparentMode_DataExchangeTypeA(&tm, &demo_state);
                            ptx_Utils_Print("Data Exchange Type A completed with Status %04X!\n", status);
                            break;
                        }

                        case DemoAppState_ExchangeB:
                        {
                            status = ptx_Example_TransparentMode_DataExchangeTypeB(&tm, &demo_state);
                            ptx_Utils_Print("Data Exchange Type B completed with Status %04X!\n", status);
                            break;
                        }

                        case DemoAppState_ExchangeF:
                        {
                            status = ptx_Example_TransparentMode_DataExchangeTypeF(&tm, &demo_state);
                            ptx_Utils_Print("Data Exchange Type F completed with Status %04X!\n", status);
                            break;
                        }

                        case DemoAppState_ExchangeV:
                        {
                            status = ptx_Example_TransparentMode_DataExchangeTypeV(&tm, &demo_state);
                            ptx_Utils_Print("Data Exchange Type V completed with Status %04X!\n", status);
                            break;
                        }

                        case DemoAppState_SetStateOff:
                        {
                            status = ptx_Example_TransparentMode_SetStateOff(&tm, &demo_state);
                            ptx_Utils_Print("Turning Field OFF completed with Status %04X!\n", status);
                            (void)ptx_RUL_Sleep(rul, PTX_EXAMPLE_TRANSPARENT_MODE_FIELD_OFF_SLEEP_MS);

                            ptx_Utils_Print("Sequence over, waiting %lu ms.\n", PTX_EXAMPLE_TRANSPARENT_MODE_LOOP_SLEEP_MS);
                            (void)ptx_RUL_Sleep(rul, PTX_EXAMPLE_TRANSPARENT_MODE_LOOP_SLEEP_MS);
                            break;
                        }

                        case DemoAppState_SystemError:
                        {
                            status = ptx_ProcessState_SystemError(&tm, &demo_state, &status_info);
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

                    /* RF-Errors can happen in this demo application (e.g. desired card not present / used) - expected, just reset status */
                    status = ptx_Utils_Check_Rf_Error(status);
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

            (void)ptx_TransparentMode_Close(&tm);
        }

        ptx_Utils_Print("Exiting Demo-Application...\n");
    }

    return status;
}

static ptx_Status_t ptx_Example_TransparentMode_SetStateOn(ptx_TransparentMode_t *tm, ptx_DemoAppState_t *appState)
{
    ptx_Status_t status = PTX_STATUS_E_GEN_INVALID_PARAMETER;

    if ((NULL != tm) && (NULL != appState))
    {
        status = ptx_TransparentMode_SetState(tm, ptx_TransparentMode_State_RF_On);
        *appState = DemoAppState_ExchangeA;
    }

    return status;
}

static ptx_Status_t ptx_Example_TransparentMode_SetStateOff(ptx_TransparentMode_t *tm, ptx_DemoAppState_t *appState)
{
    ptx_Status_t status = PTX_STATUS_E_GEN_INVALID_PARAMETER;

    if ((NULL != tm) && (NULL != appState))
    {
        status = ptx_TransparentMode_SetState(tm, ptx_TransparentMode_State_RF_Off);
        *appState = DemoAppState_SetStateOn;
    }

    return status;
}

static ptx_Status_t ptx_Example_TransparentMode_DataExchangeTypeA(ptx_TransparentMode_t *tm, ptx_DemoAppState_t *appState)
{
    ptx_Status_t status = PTX_STATUS_E_GEN_INVALID_PARAMETER;

    if ((NULL != tm) && (NULL != appState))
    {
        /* Prepare RFParams and exchange parameters required for a successful data exchange. */
        ptx_TransparentMode_RFParams_t rfParams;
        rfParams.Tech = ptx_TransparentMode_RF_Tech_A;
        rfParams.ResLimit = 1u;
        rfParams.Flags = (uint8_t)(0x00 | PTX_TRANSPARENT_MODE_FLAGS_TX_PARITY | PTX_TRANSPARENT_MODE_FLAGS_RX_PARITY);
        rfParams.RxRate = ptx_TransparentMode_RF_Bitrate_106;
        rfParams.TxRate = ptx_TransparentMode_RF_Bitrate_106;
        rfParams.RxBitOffset = 0;

        ptx_TransparentMode_ExchangeParams_t exchangeParams;
        uint8_t rx[PTX_EXAMPLE_TRANSPARENT_MODE_MAX_BUFFER_SIZE];
        uint8_t tx[PTX_EXAMPLE_TRANSPARENT_MODE_MAX_BUFFER_SIZE];
        exchangeParams.TxData = tx;
        exchangeParams.TxLength = PTX_EXAMPLE_TRANSPARENT_MODE_MAX_BUFFER_SIZE;
        memset(exchangeParams.TxData, 0, sizeof(uint8_t)*exchangeParams.TxLength);
        exchangeParams.NrTxBits = 0;
        exchangeParams.RxData = rx;
        exchangeParams.RxLength = PTX_EXAMPLE_TRANSPARENT_MODE_MAX_BUFFER_SIZE;
        memset(exchangeParams.RxData, 0, sizeof(uint8_t)*exchangeParams.RxLength);
        exchangeParams.NrResidualRxBits = 0;
        exchangeParams.TimeoutMS = PTX_EXAMPLE_TRANSPARENT_MODE_RF_TIMEOUT_MS;

        /* Start standard Type A activation procedure. */
        /* WUP-A */
        exchangeParams.TxData[0] = 0x52;
        exchangeParams.TxLength = 1;
        exchangeParams.NrTxBits = 7;

        ptx_Utils_Print("WUP-A: ");
        ptx_Utils_Print_Buffer(&exchangeParams.TxData[0], (uint16_t)exchangeParams.TxLength, 1u, 0);

        status = ptx_TransparentMode_Exchange(tm, &rfParams, &exchangeParams);

        if (PTX_STATUS_SUCCESS == status)
        {
            ptx_Utils_Print("ATQA: ");
            ptx_Utils_Print_Buffer(&exchangeParams.RxData[0], (uint16_t)exchangeParams.RxLength, 1u, 0);

            if (2 != exchangeParams.RxLength)
            {
                status = PTX_STATUS_E_GEN_INTERNAL;
            }
        }
        else
        {
            ptx_Utils_Print("NFC Data-Exchange failed with Error-Status %04X!\n", status);
        }

        /* AntiCollision */
        if (PTX_STATUS_SUCCESS == status)
        {
            exchangeParams.NrTxBits = 0;
            exchangeParams.TxData[0] = 0x93;
            exchangeParams.TxData[1] = 0x20;
            exchangeParams.TxLength = 2;
            exchangeParams.RxLength = PTX_EXAMPLE_TRANSPARENT_MODE_MAX_BUFFER_SIZE;
            memset(exchangeParams.RxData, 0, sizeof(uint8_t)*exchangeParams.RxLength);

            ptx_Utils_Print("Anticollision: ");
            ptx_Utils_Print_Buffer(&exchangeParams.TxData[0], (uint16_t)exchangeParams.TxLength, 1u, 0);

            status = ptx_TransparentMode_Exchange(tm, &rfParams, &exchangeParams);

            if (PTX_STATUS_SUCCESS == status)
            {
                ptx_Utils_Print("UID: ");
                ptx_Utils_Print_Buffer(&exchangeParams.RxData[0], (uint16_t)exchangeParams.RxLength, 1u, 0);

                if (5 != exchangeParams.RxLength)
                {
                    status = PTX_STATUS_E_GEN_INTERNAL;
                }
            }
            else
            {
                ptx_Utils_Print("NFC Data-Exchange failed with Error-Status %04X!\n", status);
            }
        }

        /* Select Command. */
        if (PTX_STATUS_SUCCESS == status)
        {
            rfParams.Flags = (uint8_t)(0x00 | PTX_TRANSPARENT_MODE_FLAGS_TX_CRC | PTX_TRANSPARENT_MODE_FLAGS_RX_CRC | PTX_TRANSPARENT_MODE_FLAGS_TX_PARITY | PTX_TRANSPARENT_MODE_FLAGS_RX_PARITY);
            exchangeParams.TxData[0] = 0x93;
            exchangeParams.TxData[1] = 0x70;
            memcpy(&exchangeParams.TxData[2], &exchangeParams.RxData[0], 5);
            exchangeParams.TxLength = 7;
            exchangeParams.RxLength = PTX_EXAMPLE_TRANSPARENT_MODE_MAX_BUFFER_SIZE;
            memset(exchangeParams.RxData, 0, sizeof(uint8_t)*exchangeParams.RxLength);

            ptx_Utils_Print("Select: ");
            ptx_Utils_Print_Buffer(&exchangeParams.TxData[0], (uint16_t)exchangeParams.TxLength, 1u, 0);

            status = ptx_TransparentMode_Exchange(tm, &rfParams, &exchangeParams);

            if (PTX_STATUS_SUCCESS == status)
            {
                ptx_Utils_Print("SAK: ");
                ptx_Utils_Print_Buffer(&exchangeParams.RxData[0], (uint16_t)exchangeParams.RxLength, 1u, 0);

                if (1 != exchangeParams.RxLength)
                {
                    status = PTX_STATUS_E_GEN_INTERNAL;
                }
            }
            else
            {
                ptx_Utils_Print("NFC Data-Exchange failed with Error-Status %04X!\n", status);
            }
        }

        if ((PTX_STATUS_SUCCESS == status) && (0 != (exchangeParams.RxData[0] & 0x04)))
        {
            /* AntiCollision CL-2 */
            rfParams.Flags = (uint8_t)(0x00 | PTX_TRANSPARENT_MODE_FLAGS_TX_PARITY | PTX_TRANSPARENT_MODE_FLAGS_RX_PARITY);
            exchangeParams.TxData[0] = 0x95;
            exchangeParams.TxData[1] = 0x20;
            exchangeParams.TxLength = 2;
            exchangeParams.RxLength = PTX_EXAMPLE_TRANSPARENT_MODE_MAX_BUFFER_SIZE;
            memset(exchangeParams.RxData, 0, sizeof(uint8_t)*exchangeParams.RxLength);

            ptx_Utils_Print("Anticollision CL-2: ");
            ptx_Utils_Print_Buffer(&exchangeParams.TxData[0], (uint16_t)exchangeParams.TxLength, 1u, 0);

            status = ptx_TransparentMode_Exchange(tm, &rfParams, &exchangeParams);

            if (PTX_STATUS_SUCCESS == status)
            {
                ptx_Utils_Print("UID: ");
                ptx_Utils_Print_Buffer(&exchangeParams.RxData[0], (uint16_t)exchangeParams.RxLength, 1u, 0);

                if (5 != exchangeParams.RxLength)
                {
                    status = PTX_STATUS_E_GEN_INTERNAL;
                }
            }
            else
            {
                ptx_Utils_Print("NFC Data-Exchange failed with Error-Status %04X!\n", status);
            }

            /* Select Command CL-2 */
            if (PTX_STATUS_SUCCESS == status)
            {
                rfParams.Flags = (uint8_t)(0x00 | PTX_TRANSPARENT_MODE_FLAGS_TX_CRC | PTX_TRANSPARENT_MODE_FLAGS_RX_CRC | PTX_TRANSPARENT_MODE_FLAGS_TX_PARITY | PTX_TRANSPARENT_MODE_FLAGS_RX_PARITY);
                exchangeParams.TxData[0] = 0x95;
                exchangeParams.TxData[1] = 0x70;
                memcpy(&exchangeParams.TxData[2], &exchangeParams.RxData[0], 5);
                exchangeParams.TxLength = 7;
                exchangeParams.RxLength = PTX_EXAMPLE_TRANSPARENT_MODE_MAX_BUFFER_SIZE;
                memset(exchangeParams.RxData, 0, sizeof(uint8_t)*exchangeParams.RxLength);

                ptx_Utils_Print("Select CL-2: ");
                ptx_Utils_Print_Buffer(&exchangeParams.TxData[0], (uint16_t)exchangeParams.TxLength, 1u, 0);

                status = ptx_TransparentMode_Exchange(tm, &rfParams, &exchangeParams);

                if (PTX_STATUS_SUCCESS == status)
                {
                    ptx_Utils_Print("SAK: ");
                    ptx_Utils_Print_Buffer(&exchangeParams.RxData[0], (uint16_t)exchangeParams.RxLength, 1u, 0);

                    if (1 != exchangeParams.RxLength)
                    {
                        status = PTX_STATUS_E_GEN_INTERNAL;
                    }
                }
                else
                {
                    ptx_Utils_Print("NFC Data-Exchange failed with Error-Status %04X!\n", status);
                }
            }
        }

        *appState = DemoAppState_ExchangeB;
    }

    return status;
}

static ptx_Status_t ptx_Example_TransparentMode_DataExchangeTypeB(ptx_TransparentMode_t *tm, ptx_DemoAppState_t *appState)
{
    ptx_Status_t status = PTX_STATUS_E_GEN_INVALID_PARAMETER;

    if ((NULL != tm) && (NULL != appState))
    {
        /* Prepare RFParams and exchange parameters required for a successful data exchange. */
        ptx_TransparentMode_RFParams_t rfParams;
        rfParams.Tech = ptx_TransparentMode_RF_Tech_B;
        rfParams.ResLimit = 1u;
        rfParams.Flags = (uint8_t)(0x00 | PTX_TRANSPARENT_MODE_FLAGS_TX_CRC | PTX_TRANSPARENT_MODE_FLAGS_RX_CRC);
        rfParams.RxRate = ptx_TransparentMode_RF_Bitrate_106;
        rfParams.TxRate = ptx_TransparentMode_RF_Bitrate_106;

        ptx_TransparentMode_ExchangeParams_t exchangeParams;
        uint8_t rx[PTX_EXAMPLE_TRANSPARENT_MODE_MAX_BUFFER_SIZE];
        uint8_t tx[PTX_EXAMPLE_TRANSPARENT_MODE_MAX_BUFFER_SIZE];
        exchangeParams.TxData = tx;
        exchangeParams.TxLength = PTX_EXAMPLE_TRANSPARENT_MODE_MAX_BUFFER_SIZE;
        memset(exchangeParams.TxData, 0, sizeof(uint8_t)*exchangeParams.TxLength);
        exchangeParams.NrTxBits = 0;
        exchangeParams.RxData = rx;
        exchangeParams.RxLength = PTX_EXAMPLE_TRANSPARENT_MODE_MAX_BUFFER_SIZE;
        memset(exchangeParams.RxData, 0, sizeof(uint8_t)*exchangeParams.RxLength);
        exchangeParams.NrResidualRxBits = 0;
        exchangeParams.TimeoutMS = PTX_EXAMPLE_TRANSPARENT_MODE_RF_TIMEOUT_MS;

        /* Start standard Type B activation procedure. */
        /* REQ-B */
        exchangeParams.TxData[0] = 0x05;
        exchangeParams.TxData[1] = 0x00;
        exchangeParams.TxData[2] = 0x00;
        exchangeParams.TxLength = 3;

        ptx_Utils_Print("REQ-B: ");
        ptx_Utils_Print_Buffer(&exchangeParams.TxData[0], (uint16_t)exchangeParams.TxLength, 1u, 0);

        status = ptx_TransparentMode_Exchange(tm, &rfParams, &exchangeParams);

        if (PTX_STATUS_SUCCESS == status)
        {
            ptx_Utils_Print("ATQB: ");
            ptx_Utils_Print_Buffer(&exchangeParams.RxData[0], (uint16_t)exchangeParams.RxLength, 1u, 0);

            if ((12 != exchangeParams.RxLength) && (13 != exchangeParams.RxLength))
            {
                status = PTX_STATUS_E_GEN_INTERNAL;
            }
        }
        else
        {
            ptx_Utils_Print("NFC Data-Exchange failed with Error-Status %04X!\n", status);
        }

        *appState = DemoAppState_ExchangeF;
    }

    return status;
}

static ptx_Status_t ptx_Example_TransparentMode_DataExchangeTypeF(ptx_TransparentMode_t *tm, ptx_DemoAppState_t *appState)
{
    ptx_Status_t status = PTX_STATUS_E_GEN_INVALID_PARAMETER;

    if ((NULL != tm) && (NULL != appState))
    {
        /* Prepare RFParams and exchange parameters required for a successful data exchange. */
        ptx_TransparentMode_RFParams_t rfParams;
        rfParams.Tech = ptx_TransparentMode_RF_Tech_F;
        rfParams.ResLimit = 0u;
        rfParams.Flags = (uint8_t)(0x00 | PTX_TRANSPARENT_MODE_FLAGS_TX_CRC | PTX_TRANSPARENT_MODE_FLAGS_RX_CRC);
        rfParams.RxRate = ptx_TransparentMode_RF_Bitrate_212;
        rfParams.TxRate = ptx_TransparentMode_RF_Bitrate_212;
        rfParams.RxBitOffset = 0;

        ptx_TransparentMode_ExchangeParams_t exchangeParams;
        uint8_t rx[PTX_EXAMPLE_TRANSPARENT_MODE_MAX_BUFFER_SIZE];
        uint8_t tx[PTX_EXAMPLE_TRANSPARENT_MODE_MAX_BUFFER_SIZE];
        exchangeParams.TxData = tx;
        exchangeParams.TxLength = PTX_EXAMPLE_TRANSPARENT_MODE_MAX_BUFFER_SIZE;
        memset(exchangeParams.TxData, 0, sizeof(uint8_t)*exchangeParams.TxLength);
        exchangeParams.NrTxBits = 0;
        exchangeParams.RxData = rx;
        exchangeParams.RxLength = PTX_EXAMPLE_TRANSPARENT_MODE_MAX_BUFFER_SIZE;
        memset(exchangeParams.RxData, 0, sizeof(uint8_t)*exchangeParams.RxLength);
        exchangeParams.NrResidualRxBits = 0;
        exchangeParams.TimeoutMS = PTX_EXAMPLE_TRANSPARENT_MODE_RF_TIMEOUT_MS;

        /* Start standard Type F activation procedure. */
        /* SENSF_REQ */
        exchangeParams.TxData[0] = 0x06;
        exchangeParams.TxData[1] = 0x00;
        exchangeParams.TxData[2] = 0xFF;
        exchangeParams.TxData[3] = 0xFF;
        exchangeParams.TxData[4] = 0x00;
        exchangeParams.TxData[5] = 0x0F;
        exchangeParams.TxLength = 6;

        ptx_Utils_Print("SENSF_REQ: ");
        ptx_Utils_Print_Buffer(&exchangeParams.TxData[0], (uint16_t)exchangeParams.TxLength, 1u, 0);

        status = ptx_TransparentMode_Exchange(tm, &rfParams, &exchangeParams);

        if (PTX_STATUS_SUCCESS == status)
        {
            ptx_Utils_Print("SENSF_RES: ");
            ptx_Utils_Print_Buffer(&exchangeParams.RxData[0], (uint16_t)exchangeParams.RxLength, 1u, 0);

            if (18 != exchangeParams.RxLength)
            {
                status = PTX_STATUS_E_GEN_INTERNAL;
            }
        }
        else
        {
            ptx_Utils_Print("NFC Data-Exchange failed with Error-Status %04X!\n", status);
        }

        *appState = DemoAppState_ExchangeV;
    }

    return status;
}

static ptx_Status_t ptx_Example_TransparentMode_DataExchangeTypeV(ptx_TransparentMode_t *tm, ptx_DemoAppState_t *appState)
{
    ptx_Status_t status = PTX_STATUS_E_GEN_INVALID_PARAMETER;

    if ((NULL != tm) && (NULL != appState))
    {
        /* Prepare RFParams and exchange parameters required for a successful data exchange. */
        ptx_TransparentMode_RFParams_t rfParams;
        rfParams.Tech = ptx_TransparentMode_RF_Tech_V;
        rfParams.ResLimit = 1u;
        rfParams.Flags = (uint8_t)(0x00 | PTX_TRANSPARENT_MODE_FLAGS_TX_CRC | PTX_TRANSPARENT_MODE_FLAGS_RX_CRC);
        rfParams.RxRate = ptx_TransparentMode_RF_Bitrate_26;
        rfParams.TxRate = ptx_TransparentMode_RF_Bitrate_26;
        rfParams.RxBitOffset = 0;

        ptx_TransparentMode_ExchangeParams_t exchangeParams;
        uint8_t rx[PTX_EXAMPLE_TRANSPARENT_MODE_MAX_BUFFER_SIZE];
        uint8_t tx[PTX_EXAMPLE_TRANSPARENT_MODE_MAX_BUFFER_SIZE];
        exchangeParams.TxData = tx;
        exchangeParams.TxLength = PTX_EXAMPLE_TRANSPARENT_MODE_MAX_BUFFER_SIZE;
        memset(exchangeParams.TxData, 0, sizeof(uint8_t)*exchangeParams.TxLength);
        exchangeParams.NrTxBits = 0;
        exchangeParams.RxData = rx;
        exchangeParams.RxLength = PTX_EXAMPLE_TRANSPARENT_MODE_MAX_BUFFER_SIZE;
        memset(exchangeParams.RxData, 0, sizeof(uint8_t)*exchangeParams.RxLength);
        exchangeParams.NrResidualRxBits = 0;
        exchangeParams.TimeoutMS = PTX_EXAMPLE_TRANSPARENT_MODE_RF_TIMEOUT_MS;

        /* Start standard Type V activation procedure. */
        /* Inventory-Request */
        exchangeParams.TxData[0] = 0x26;
        exchangeParams.TxData[1] = 0x01;
        exchangeParams.TxData[2] = 0x00;
        exchangeParams.TxLength = 3;

        ptx_Utils_Print("Inventory-Request: ");
        ptx_Utils_Print_Buffer(&exchangeParams.TxData[0], (uint16_t)exchangeParams.TxLength, 1u, 0);

        status = ptx_TransparentMode_Exchange(tm, &rfParams, &exchangeParams);

        if (PTX_STATUS_SUCCESS == status)
        {
            ptx_Utils_Print("Inventory-Response: ");
            ptx_Utils_Print_Buffer(&exchangeParams.RxData[0], (uint16_t)exchangeParams.RxLength, 1u, 0);

            if (10 != exchangeParams.RxLength)
            {
                status = PTX_STATUS_E_GEN_INTERNAL;
            }
        }
        else
        {
            ptx_Utils_Print("NFC Data-Exchange failed with Error-Status %04X!\n", status);
        }

        *appState = DemoAppState_SetStateOff;
    }

    return status;
}

static ptx_Status_t ptx_ProcessState_SystemError(ptx_TransparentMode_t *tm, ptx_DemoAppState_t *appState, ptx_RUL_Status_Params_t *statusInfoParam)
{
    ptx_Status_t status = PTX_STATUS_E_GEN_INVALID_PARAMETER;

    if ((NULL != tm) && (NULL != appState) && (NULL != statusInfoParam))
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
            status = ptx_ProcessState_RestartSystem(tm, appState);
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

static ptx_Status_t ptx_ProcessState_RestartSystem(ptx_TransparentMode_t *tm, ptx_DemoAppState_t *appState)
{
    ptx_Status_t status = PTX_STATUS_E_GEN_INVALID_PARAMETER;

    if ((NULL != tm) && (NULL != appState))
    {
        do
        {
            /* Wait for 1s to give the chip time to cool down, after that time try to initialize the system again */
            status = ptx_RUL_Sleep(tm->Rul, 1000u);

            if (PTX_STATUS_SUCCESS == status)
            {
                status = ptx_RUL_InitHardware(tm->Rul);
            }

        }
        while (status != PTX_STATUS_SUCCESS);

        if (PTX_STATUS_SUCCESS == status)
        {
            status = ptx_TransparentMode_ResetState(tm);
        }

        if (PTX_STATUS_SUCCESS == status)
        {
            *appState = DemoAppState_SetStateOn;
        }
    }

    return status;
}
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
    Module      : SDK Example (NFC Forum Native Tag API Add-On API)
    File        : EXAMPLES/ptx_Example_NFCForumNativeTag/ptx_ExampleApplication.c

    Description : This application example demonstrates how to
                  - setup and start the basic NFC Forum RF-Discovery loop,
                  - retrieve Card/Tag parameters from the Card-registry,
                  - perform NFC data exchanges with various NFC Forum Type Tags 2 - 5
                  using the Native Tag Add-on API. Typical use cases for the NFC Forum RF-Discovery
                  and data exchanges are IoT applications.
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
#include "ptx_NativeTag_T2T.h"
#include "ptx_NativeTag_T3T.h"
#include "ptx_NativeTag_T4T.h"
#include "ptx_NativeTag_T5T.h"
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
/* Application specific Exit-Flag (controllable by the user on Windows-/Linux-platforms via Ctrl-C). */
static volatile int gExit = 0;

#if defined(PTX_ENABLE_BOARD_EVK)
    /* Corresponds to the voltage on the DC/DC converter on the PTX2xxW Boards. E.g. a value of 330 corresponds to 3.3V. */
    static const uint16_t EVK_DEFAULT_VOLTAGE = 500u;
#endif

/* Delay/Sleep between checking RF-Discovery state and no device was found. */
static const uint32_t PTX_DEMO_APP_NO_CARD_SLEEP_MS = 10u;

/* Delay/Sleep between checking RF-Discovery state and Discovery-procedure is ongoing. */
static const uint32_t PTX_DEMO_APP_CARD_RESOVLVE_SLEEP_MS = 5u;

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
#define PTX_DEMO_APP_EXAMPLE_WORK_BUFFER_SIZE       266u

/*
 * ####################################################################################################################
 * INTERNAL FUNCTIONS DECLARATIONS / HELPERS
 * ####################################################################################################################
 */
static ptx_Status_t ptx_RunDemoApplication(ptx_RUL_t *rul);
static ptx_Status_t ptx_ProcessState_StartDiscovery(ptx_RUL_t *rul, ptx_DemoAppState_t *appState);
static ptx_Status_t ptx_ProcessState_WaitForDiscovery(ptx_RUL_t *rul, ptx_DemoAppState_t *appState, ptx_RUL_CardRegistry_t *cardRegistry);
static ptx_Status_t ptx_ProcessState_SelectCard(ptx_RUL_t *rul, ptx_DemoAppState_t *appState, ptx_RUL_CardRegistry_t *cardRegistry);
static ptx_Status_t ptx_ProcessState_NativeTagDataExchange(ptx_RUL_t *rul,
                                                           ptx_DemoAppState_t *appState,
                                                           ptx_RUL_CardRegistry_t *cardRegistry,
                                                           ptx_NativeTag_T2T_t *t2t,
                                                           ptx_NativeTag_T3T_t *t3t,
                                                           ptx_NativeTag_T4T_t *t4t,
                                                           ptx_NativeTag_T5T_t *t5t);
static ptx_Status_t ptx_ProcessState_DeactivateReader(ptx_RUL_t *rul, ptx_DemoAppState_t *appState);
static ptx_Status_t ptx_ProcessState_RestartDiscovery(ptx_RUL_t *rul, ptx_DemoAppState_t *appState);
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
    ptx_Utils_Print("****   NFC Forum Native Tag API Demo %s             ****\n", PTX_DEMO_VERSION);
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

    ptx_DemoAppState_t demo_state = DemoAppState_StartDiscovery;
    ptx_RUL_Status_Params_t status_info = {0};
    ptx_RUL_CardRegistry_t *card_registry = NULL;

    /* Data Exchange (work) buffers. */
    uint8_t tx_data[PTX_DEMO_APP_EXAMPLE_WORK_BUFFER_SIZE];

    /* Native Tag Add-on API components */
    ptx_NativeTag_T2T_t t2t = {0};
    ptx_NativeTag_T2T_InitParams_t t2t_init_params = {0};

    ptx_NativeTag_T3T_t t3t = {0};
    ptx_NativeTag_T3T_InitParams_t t3t_init_params = {0};

    ptx_NativeTag_T4T_t t4t = {0};
    ptx_NativeTag_T4T_InitParams_t t4t_init_params = {0};

    ptx_NativeTag_T5T_t t5t = {0};
    ptx_NativeTag_T5T_InitParams_t t5t_init_params = {0};

    if (NULL != rul)
    {
        /* Get and store a pointer to the RUL card registry */
        status = ptx_RUL_GetCardRegistry(rul, &card_registry);

        if (PTX_STATUS_SUCCESS == status)
        {
            /* Initialize T2T Native Tag API. */
            t2t_init_params.rul = rul;
            t2t_init_params.TxBuffer = &tx_data[0];
            t2t_init_params.TxBufferSize = (uint32_t)PTX_DEMO_APP_EXAMPLE_WORK_BUFFER_SIZE;

            status = ptx_NativeTag_T2T_Open(&t2t, &t2t_init_params);

            if (PTX_STATUS_SUCCESS == status)
            {
                ptx_Utils_Print("T2T Native Tag API successfully initialized!\n");
            }
            else
            {
                ptx_Utils_Print("T2T Native Tag API initialization failed with Error-Status %04X!\n", status);
            }
        }

        if (PTX_STATUS_SUCCESS == status)
        {
            /* Initialize T3T Native Tag API. */
            t3t_init_params.rul = rul;
            t3t_init_params.TxBuffer = &tx_data[0];
            t3t_init_params.TxBufferSize = (uint32_t)PTX_DEMO_APP_EXAMPLE_WORK_BUFFER_SIZE;

            /* Attention:
            *  At this stage of the program flow, no card was activated yet and thus, the following activation parameters are unknown.
            *  Just assume NULL / 0 for those parameters; they can be overwritten later on via a dedicated call to "ptx_NativeTag_T3T_SetTagParams()"
            *  before any T3T Native Tag API function gets called.
            *  Another opportunity is to call this Init-/-Open-function once a T3T card was activated because then all parameters are available in the card registry.
            */
            t3t_init_params.NFCID2Len = 0;
            t3t_init_params.MRTI_Check = 0;
            t3t_init_params.MRTI_Update = 0;

            status = ptx_NativeTag_T3T_Open(&t3t, &t3t_init_params);

            if (PTX_STATUS_SUCCESS == status)
            {
                ptx_Utils_Print("T3T Native Tag API successfully initialized!\n");
            }
            else
            {
                ptx_Utils_Print("T3T Native Tag API initialization failed with Error-Status %04X!\n", status);
            }
        }

        if (PTX_STATUS_SUCCESS == status)
        {
            /* Initialize T4T Native Tag API. */
            t4t_init_params.rul = rul;
            t4t_init_params.TxBuffer = &tx_data[0];
            t4t_init_params.TxBufferSize = (uint32_t)PTX_DEMO_APP_EXAMPLE_WORK_BUFFER_SIZE;

            status = ptx_NativeTag_T4T_Open(&t4t, &t4t_init_params);

            if (PTX_STATUS_SUCCESS == status)
            {
                ptx_Utils_Print("T4T Native Tag API successfully initialized!\n");
            }
            else
            {
                ptx_Utils_Print("T4T Native Tag API initialization failed with Error-Status %04X!\n", status);
            }
        }

        if (PTX_STATUS_SUCCESS == status)
        {
            /* Initialize T5T Native Tag API (use selected/addressed mode i.e. every command conveys the UID of the T5T card). */
            t5t_init_params.rul = rul;
            t5t_init_params.TxBuffer = &tx_data[0];
            t5t_init_params.TxBufferSize = (uint32_t)PTX_DEMO_APP_EXAMPLE_WORK_BUFFER_SIZE;
            t5t_init_params.isSelected = 1u;

            /* Attention:
            *  At this stage of the program flow, no card was activated yet and thus, the following activation parameters are unknown.
            *  Just assume NULL / 0 for those parameters; they can be overwritten later on via a dedicated call to "ptx_NativeTag_T5T_SetUID()"
            *  before any T5T Native Tag API function gets called.
            *  Another opportunity is to call this Init-/-Open-function once a T5T card was activated because then all parameters are available in the card registry.
            *
            *  If Init-parameter "isSelected" is set to 0, then no UID would be needed at all.
            */
            status = ptx_NativeTag_T5T_Open(&t5t, &t5t_init_params);

            if (PTX_STATUS_SUCCESS == status)
            {
                ptx_Utils_Print("T5T Native Tag API successfully initialized!\n");
            }
            else
            {
                ptx_Utils_Print("T5T Native Tag API initialization failed with Error-Status %04X!\n", status);
            }
        }

        if (PTX_STATUS_SUCCESS == status)
        {
            do
            {
                /* Check regularly for System-state/-health (e.g. temperature/over-current errors etc.) */
                status_info.Type = ptx_RUL_StatusType_System;

                status = ptx_RUL_GetStatusInfo(rul, &status_info);

                if (PTX_STATUS_SUCCESS == status)
                {
                    if (ptx_RUL_StatusInfoSystem_ErrorStandby == status_info.Info.System)
                    {
                        demo_state = DemoAppState_StartDiscovery;
                    }
                    else if (ptx_RUL_StatusInfoSystem_OK != status_info.Info.System)
                    {
                        demo_state = DemoAppState_SystemError;
                    }
                    else
                    {
                        /* Current system status is OK. Just continue. */
                    }

                }

                if (PTX_STATUS_SUCCESS == status)
                {
                    switch (demo_state)
                    {
                        case DemoAppState_StartDiscovery:
                        {
                            status = ptx_ProcessState_StartDiscovery(rul, &demo_state);
                            break;
                        }

                        case DemoAppState_WaitForDiscovery:
                        {
                            status = ptx_ProcessState_WaitForDiscovery(rul, &demo_state, card_registry);
                            break;
                        }

                        case DemoAppState_SelectCard:
                        {
                            status = ptx_ProcessState_SelectCard(rul, &demo_state, card_registry);
                            break;
                        }

                        case DemoAppState_NativeTagDataExchange:
                        {
                            status = ptx_ProcessState_NativeTagDataExchange(rul, &demo_state, card_registry, &t2t, &t3t, &t4t, &t5t);
                            break;
                        }

                        case DemoAppState_DeactivateReader:
                        {
                            status = ptx_ProcessState_DeactivateReader(rul, &demo_state);
                            break;
                        }

                        case DemoAppState_RestartDiscovery:
                        {
                            status = ptx_ProcessState_RestartDiscovery(rul, &demo_state);
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
        }

        /* De-initialize / Close the Native Tag API components (regardless of previous status). */
        (void)ptx_NativeTag_T2T_Close(&t2t);
        (void)ptx_NativeTag_T3T_Close(&t3t);
        (void)ptx_NativeTag_T4T_Close(&t4t);
        (void)ptx_NativeTag_T5T_Close(&t5t);

        ptx_Utils_Print("Exiting Demo-Application...\n");
    }

    return status;
}

static ptx_Status_t ptx_ProcessState_StartDiscovery(ptx_RUL_t *rul, ptx_DemoAppState_t *appState)
{
    ptx_Status_t status = PTX_STATUS_E_GEN_INVALID_PARAMETER;

    ptx_RUL_DiscoverConfig_t disc_params = {0};

    if ((NULL != rul) && (NULL != appState))
    {
        /* Initialize RF-Discovery / Polling Loop in NFC Forum Poll-Mode with standard parameters.*/
        disc_params.PollMode = ptx_RUL_PollMode_NfcForum;   /* Enable PolL-Mode only in this example (Listen-mode can be optionally enabled). */
        disc_params.EnablePolling = 1u;                     /* Enable Polling only (Listen-mode can be optionally enabled). */
        disc_params.IdleTime = 100u;                        /* System pauses / idles for 100ms after every full polling loop-cycle. */
        disc_params.PollTypeA = 1u;                         /* Poll for NFC-A Technology. */
        disc_params.PollTypeAPollFrequency = 1u;            /* Poll for NFC-A Technology during every Polling-Loop cycle (otherwise every n-th cycle). */
        disc_params.PollTypeADeviceLimit = 5u;              /* Limit the number of NFC-A devices / cards to max. 5 devices (application can define max. value). */
        disc_params.PollTypeB = 1u;                         /* Poll for NFC-B Technology. */
        disc_params.PollTypeBPollFrequency = 1u;            /* Poll for NFC-B Technology during every Polling-Loop cycle (otherwise every n-th cycle). */
        disc_params.PollTypeBDeviceLimit = 5u;              /* Limit the number of NFC-B devices / cards to max. 5 devices (application can define max. value). */
        disc_params.PollTypeF212 = 1u;                      /* Poll for NFC-F 212 kBit/s Technology. */
        disc_params.PollTypeFPollFrequency = 1u;            /* Poll for NFC-F Technology during every Polling-Loop cycle (otherwise every n-th cycle). */
        disc_params.PollTypeFDeviceLimit = 5u;              /* Limit the number of NFC-F devices / cards to max. 5 devices (application can define max. value). */
        disc_params.PollTypeV = 1u;                         /* Poll for NFC-V Technology. */
        disc_params.PollTypeVPollFrequency = 1u;            /* Poll for NFC-V Technology during every Polling-Loop cycle (otherwise every n-th cycle). */
        disc_params.PollTypeVDeviceLimit = 5u;              /* Limit the number of NFC-V devices / cards to max. 5 devices (application can define max. value). */
        disc_params.EnableLPCD = 0;                         /* Don't use low-power-card-detection (LPCD) in this demo. */
#if !defined(PTX_DISABLE_LISTEN_MODE)
        disc_params.ListenMode = 0;                         /* Don't use Listen-mode (Card Emulation) in this demo. */
#endif /* PTX_DISABLE_LISTEN_MODE */

        status = ptx_RUL_InitiateDiscovery(rul, &disc_params);

        if (PTX_STATUS_SUCCESS == status)
        {
            *appState = DemoAppState_WaitForDiscovery;
            ptx_Utils_Print("RF-Discovery successfully started - waiting for Cards ... \n");
        }
        else
        {
            *appState = DemoAppState_Exit;
            ptx_Utils_Print("Starting RF-Discovery failed with Error-Status %04X!\n", status);
        }
    }

    return status;
}

static ptx_Status_t ptx_ProcessState_WaitForDiscovery(ptx_RUL_t *rul, ptx_DemoAppState_t *appState, ptx_RUL_CardRegistry_t *cardRegistry)
{
    ptx_Status_t status = PTX_STATUS_E_GEN_INVALID_PARAMETER;

    ptx_RUL_Status_Params_t discover_status;

    if ((NULL != rul) && (NULL != appState) && (NULL != cardRegistry))
    {
        /* Check the status of the Discovery-Loop */
        memset(&discover_status, 0x00, sizeof(ptx_RUL_Status_Params_t));
        discover_status.Type = ptx_RUL_StatusType_Discover;
        status = ptx_RUL_GetStatusInfo(rul, &discover_status);

        if (PTX_STATUS_SUCCESS == status)
        {
            switch (discover_status.Info.Discover)
            {
                case ptx_RUL_StatusInfoDiscover_NoCard:
                {
                    /* No card found so far, just wait for a certain amount of time to unblock/off-load CPU / Thread etc. */
                    (void)ptx_RUL_Sleep(rul, PTX_DEMO_APP_NO_CARD_SLEEP_MS);
                    break;
                }

                case ptx_RUL_StatusInfoDiscover_CardDetectionOngoing:
                {
                    /* Multiple cards present, the internal Discovery-procedure is still busy resolving all cards. */
                    ptx_Utils_Print("Multiple Cards found - trying to resolve all Cards ... \n");
                    (void)ptx_RUL_Sleep(rul, PTX_DEMO_APP_CARD_RESOVLVE_SLEEP_MS);
                    break;
                }

                case ptx_RUL_StatusInfoDiscover_CardsAvailable:
                {
                    /*
                    * Multiple cards present, the internal Discovery-procedure has resolved all cards.
                    * In this demo, the very first card available in the registry will be selected, but any
                    * card in the registry can be activated in a real application.
                    */
                    ptx_Utils_Print("Multiple Cards found - all Cards resolved.\n");

                    /* Print all available Card-Infos as a List. */
                    for (uint8_t i = 0; i < cardRegistry->CardsCnt; i++)
                    {
                        (void)ptx_Utils_CardInfo_Print_Card_Parameters(rul, i);
                    }

                    *appState = DemoAppState_SelectCard;
                    break;
                }

                case ptx_RUL_StatusInfoDiscover_MultiProtocol:
                {
                    /*
                     * Single device supporting multiple protocols found.
                     * Such devices can typically be
                     *  - NFC Mobile phones supporting ISO-DEP and P2P or
                     *  - cards supporting additional emulation modes.
                     *
                     * In this demo, the first suitable protocol supported will be used for the example data exchange, but
                     * specific protocol needed by the application can be selected as well.
                     */
                    ptx_Utils_Print("Single Card supporting multiple protocols found - trying to activate first protocol found.\n");

                    /* Print all available Card-Infos as a List. */
                    (void)ptx_Utils_CardInfo_Print_Card_Parameters(rul, 0);

                    *appState = DemoAppState_SelectCard;
                    break;
                }

                case ptx_RUL_StatusInfoDiscover_Active:
                {
                    /*
                     * An NFC-device has been activated i.e. selected and is ready for (user) data exchange.
                     * Activation / Selection takes directly place if single device / card supporting only one protocol was detected.
                     */
                    ptx_Utils_Print("Card / Device activated -> ready for NFC data exchange.\n");

                    /* Print all available Card-Infos as a List. */
                    (void)ptx_Utils_CardInfo_Print_Card_Parameters(rul, cardRegistry->ActiveCardId);

                    /* Card / Device is ready for (user) data exchange based on Native Tag API. */
                    *appState = DemoAppState_NativeTagDataExchange;
                    break;
                }

                default:
                {
                    *appState = DemoAppState_Exit;
                    ptx_Utils_Print("Invalid/Unsupported Application-State detected!\n");
                    break;
                }
            }
        }
        else
        {
            *appState = DemoAppState_Exit;
            ptx_Utils_Print("Check status of RF-Discovery failed with Error-Status %04X!\n", status);
        }
    }

    return status;
}

static ptx_Status_t ptx_ProcessState_SelectCard(ptx_RUL_t *rul, ptx_DemoAppState_t *appState, ptx_RUL_CardRegistry_t *cardRegistry)
{
    ptx_Status_t status = PTX_STATUS_E_GEN_INVALID_PARAMETER;

    ptx_RUL_CardProtocol_t protocol_params = {0};
    ptx_RUL_CardParams_t *card;

    if ((NULL != rul) && (NULL != appState) && (NULL != cardRegistry))
    {
        /* For demo-purposes, the very first device in the registry is used - try to activate / select the first supported protocol found. */
        card = &cardRegistry->Cards[0];

        protocol_params.Type = ptx_RUL_CardProtocolType_Undefined;

        if (0 != (ptx_RUL_SupportedProtocols_T2T & card->Protocols))
        {
            protocol_params.Type = ptx_RUL_CardProtocolType_T2T;
        }

        if (0 != (ptx_RUL_SupportedProtocols_T3T & card->Protocols))
        {
            protocol_params.Type = ptx_RUL_CardProtocolType_T3T;
        }

        if (0 != (ptx_RUL_SupportedProtocols_T4T & card->Protocols))
        {
            /* Additional parameters required for ISO-DEP: Higher Bitrate is valid for Type-A and -B. */
            protocol_params.Param.IsoDep.HbrEnable = 0;

            if (ptx_RUL_CardRfTechType_B == card->TechType)
            {
                /* ATTRIB_INFO and ATTRIB_INFO-length only valid for Type-B - not used in the Demo. */
                protocol_params.Param.IsoDep.AttribInfLen = 0;
            }

            protocol_params.Type = ptx_RUL_CardProtocolType_T4T;
        }

        if (0 != (ptx_RUL_SupportedProtocols_T5T & card->Protocols))
        {
            protocol_params.Type = ptx_RUL_CardProtocolType_T5T;
        }

        if (0 != (ptx_RUL_SupportedProtocols_NfcDep & card->Protocols))
        {
            protocol_params.Type = ptx_RUL_CardProtocolType_NfcDep;
        }

        status = ptx_RUL_SelectCard(rul, 0, protocol_params);

        if (PTX_STATUS_SUCCESS == status)
        {
            ptx_Utils_Print("Card Activation/Selection successful!");

            /* Activation / Selection successfully - print Card-infos and move on to exchange example data. */
            (void)ptx_Utils_CardInfo_Print_Card_Parameters(rul, 0);

            *appState = DemoAppState_NativeTagDataExchange;
        }
        else
        {
            ptx_Utils_Print("Card Activation/Selection failed with Error-Status %04X!\n", status);

            /* It might be the case that the card was moved in the meanwhile - can happen, reset status and restart RF-Discovery-*/
            status = ptx_Utils_Check_Rf_Error(status);

            if (PTX_STATUS_SUCCESS == status)
            {
                *appState = DemoAppState_RestartDiscovery;
            }

        }
    }

    return status;
}

static ptx_Status_t ptx_ProcessState_NativeTagDataExchange(ptx_RUL_t *rul,
                                                           ptx_DemoAppState_t *appState,
                                                           ptx_RUL_CardRegistry_t *cardRegistry,
                                                           ptx_NativeTag_T2T_t *t2t,
                                                           ptx_NativeTag_T3T_t *t3t,
                                                           ptx_NativeTag_T4T_t *t4t,
                                                           ptx_NativeTag_T5T_t *t5t)
{
    ptx_Status_t status = PTX_STATUS_E_GEN_INVALID_PARAMETER;

    /* Data Exchange (work) buffers. */
    uint8_t rx_data[PTX_DEMO_APP_EXAMPLE_WORK_BUFFER_SIZE];
    uint32_t rx_data_len = 0;
    uint8_t do_receive = 1u;

    if ((NULL != rul) && (NULL != appState) && (NULL != cardRegistry) && (NULL != t2t) && (NULL != t3t) && (NULL != t4t) && (NULL != t5t))
    {
        switch (cardRegistry->ActiveProtocol)
        {
            case ptx_RUL_CardProtocolType_T2T:
            {
                ptx_Utils_Print("Exchange T2T \"READ BLOCK (0)\"-Command \n");

                rx_data_len = (uint32_t)PTX_DEMO_APP_EXAMPLE_WORK_BUFFER_SIZE;
                status = ptx_NativeTag_T2T_Read(t2t, 0, &rx_data[0], &rx_data_len, 100u);
                break;
            }

            case ptx_RUL_CardProtocolType_T3T:
            {
                ptx_Utils_Print("Exchange T3T \"CHECK BLOCK (0)\"-Command \n");

                ptx_NativeTag_T3T_MRTI_t t3t_mrti_info = {0};
                uint8_t t3t_block_nr_read[]      = {0x80, 0x00};  /* Block number in NDEF service. */
                uint8_t t3t_service_code_read[]  = {0x0B, 0x00};  /* Service Code Read NDEF. */
                ptx_NativeTag_T3T_Services_t t3t_service_info = {0};
                ptx_NativeTag_T3T_Blocks_t t3t_block_info = {0};;

                t3t_mrti_info.MRTICheck = cardRegistry->ActiveCard->TechParam.TypeF.SensFRes[PTX_T3T_SENSFRES_OFFSET_MRTI_CHECK];
                t3t_mrti_info.MRTIUpdate = cardRegistry->ActiveCard->TechParam.TypeF.SensFRes[PTX_T3T_SENSFRES_OFFSET_MRTI_UPDATE];

                /* Update Card-parameters upfront. */
                status = ptx_NativeTag_T3T_SetTagParams (t3t,
                                                         &cardRegistry->ActiveCard->TechParam.TypeF.SensFRes[PTX_T3T_SENSFRES_OFFSET_NFCID2],
                                                         PTX_T3T_NFCID2_SIZE,
                                                         1u,    // Number of Blocks to Read (NBr) at once - assume 1 for this demo, can be adapted.
                                                         1u,    // Number of Blocks to Write (NBw) at once - assume 1 for this demo, can be adapted.
                                                         t3t_mrti_info);

                if (PTX_STATUS_SUCCESS == status)
                {
                    /* Fill info parameters to read ("CHECK") Block 0 (usually NDEF Attribute-Info block). */
                    t3t_block_info.NOB = 1u;
                    t3t_block_info.BlockList = &t3t_block_nr_read[0];
                    t3t_block_info.BlockListLen = 2u;
                    t3t_service_info.NOS = 1u;
                    t3t_service_info.ServiceCodeList = &t3t_service_code_read[0];
                    t3t_service_info.ServiceCodeListLen = 2u;

                    rx_data_len = (uint32_t)PTX_DEMO_APP_EXAMPLE_WORK_BUFFER_SIZE;
                    status = ptx_NativeTag_T3T_Check(t3t, t3t_service_info, t3t_block_info, &rx_data[0], &rx_data_len, 100u);
                }
                break;
            }

            case ptx_RUL_CardProtocolType_T4T:
            {
                ptx_Utils_Print("Exchange T4T \"SELECT NFC Forum Application\"-APDU\n");

                rx_data_len = (uint32_t)PTX_DEMO_APP_EXAMPLE_WORK_BUFFER_SIZE;
                status = ptx_NativeTag_T4T_Select_NDEF_Application(t4t, &rx_data[0], &rx_data_len, 100u);
                break;
            }

            case ptx_RUL_CardProtocolType_T5T:
            {
                ptx_Utils_Print("Exchange T5T \"READ_SINGLE_BLOCK (0)\"-Command \n");

                /* Update Card-parameters upfront. */
                status = ptx_NativeTag_T5T_SetUID (t5t, &cardRegistry->ActiveCard->TechParam.TypeV.InventoryRes[PTX_T5T_UID_INVENTORY_OFFSET], PTX_T5T_UID_SIZE);

                if (PTX_STATUS_SUCCESS == status)
                {
                    rx_data_len = (uint32_t)PTX_DEMO_APP_EXAMPLE_WORK_BUFFER_SIZE;
                    status = ptx_NativeTag_T5T_ReadSingleBlock (t5t, 0x22u, 0, &rx_data[0], &rx_data_len, 100u);
                }
                break;
            }

            default:
            {
                status = PTX_STATUS_SUCCESS;
                do_receive = 0;
                ptx_Utils_Print("Unknown / Unsupported protocol detected - restarting RF-Discovery\n");
                break;
            }

        }

        if (PTX_STATUS_SUCCESS == status)
        {
            if (0 != do_receive)
            {
                ptx_Utils_Print("SUCCESS - RX Data: ");
                ptx_Utils_Print_Buffer(&rx_data[0], (uint16_t)rx_data_len, 1u, 0);
            }
        }
        else
        {
            ptx_Utils_Print("NFC Data-Exchange failed with Error-Status %04X!\n", status);

            /* It might be the case that the card was moved in the meanwhile - can happen, reset status and restart RF-Discovery-*/
            status = ptx_Utils_Check_Rf_Error(status);
        }

        /* In any case, restart RF-Discovery */
        *appState = DemoAppState_RestartDiscovery;
    }

    return status;
}

static ptx_Status_t ptx_ProcessState_DeactivateReader(ptx_RUL_t *rul, ptx_DemoAppState_t *appState)
{
    ptx_Status_t status = PTX_STATUS_E_GEN_INVALID_PARAMETER;

    ptx_RUL_DeactivateParams_t deact_params = {0};

    if ((NULL != rul) && (NULL != appState))
    {
        /* Generic deactivation -> switch RF-Field off and switch to IDLE-state. */
        deact_params.Type = ptx_RUL_DeactivateType_Generic;
        deact_params.State = ptx_RUL_DeactivateState_Idle;
        status = ptx_RUL_Deactivate(rul, &deact_params);

        if (PTX_STATUS_SUCCESS == status)
        {
            ptx_Utils_Print("RF-Field switched off successfully.\n");
        }
        else
        {
            ptx_Utils_Print("Switching RF-Field off failed with Error-Status %04X!\n", status);
        }

        /* Quit application in any case. */
        *appState = DemoAppState_Exit;
    }

    return status;
}

static ptx_Status_t ptx_ProcessState_RestartDiscovery(ptx_RUL_t *rul, ptx_DemoAppState_t *appState)
{
    ptx_Status_t status = PTX_STATUS_E_GEN_INVALID_PARAMETER;

    ptx_RUL_DeactivateParams_t deact_params = {0};

    if ((NULL != rul) && (NULL != appState))
    {
        /* Perform short reset of RF-Field and restart Polling-Loop again. */
        deact_params.Type = ptx_RUL_DeactivateType_Generic;
        deact_params.State = ptx_RUL_DeactivateState_DiscoveryRfReset;
        status = ptx_RUL_Deactivate(rul, &deact_params);

        if (PTX_STATUS_SUCCESS == status)
        {
            ptx_Utils_Print("Reset of RF-Field successful - waiting for Cards ... \n");
            *appState = DemoAppState_WaitForDiscovery;
        }
        else
        {
            ptx_Utils_Print("Reseting RF-Field failed with Error-Status %04X!\n", status);
            *appState = DemoAppState_Exit;
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
            *appState = DemoAppState_StartDiscovery;
        }
    }

    return status;
}

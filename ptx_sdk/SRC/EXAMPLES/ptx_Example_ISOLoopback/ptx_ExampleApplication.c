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
    Module      : SDK Example (ISO Loopback)
    File        : EXAMPLES/ptx_Example_ISOLoopback/ptx_ExampleApplication.c

    Description : This application demonstrates the use of the ISO Loopback
                  application using the Reader Universal Library (RUL) and the HCE add-on API.
                  The application is used for compliance / type-approval testing.
*/

/*
 * ####################################################################################################################
 * INCLUDES
 * ####################################################################################################################
 */

#include "ptx_ExampleApplication.h"
#include "ptx_Status.h"
#include "ptx_RUL.h"
#if !defined(PTX_DISABLE_LISTEN_MODE)
    #include "ptx_RUL_HCE.h"
#endif /* PTX_DISABLE_LISTEN_MODE */
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

/* Select ISO PICC or PCD Loopback mode. Only one may be active at a time! */
#define PTX_DEMO_APP_LOOPBACK_MODE       ISO_LOOPBACK_PICC
// #define PTX_DEMO_APP_LOOPBACK_MODE       ISO_LOOPBACK_PCD

/* Timeout in MS for card data exchange in PCD Loopback Mode */
#define PTX_DEMO_APP_PCD_DATA_EXCHANGE_TIMEOUT_MS 15000

/* Select ISO PCD Loopback Test Sequence. Only one may be active at a time! */
#define PTX_DEMO_APP_PCD_ISO_SEQUENCE ISO_DEFAULT_TEST_COMMAND_SEQUENCE
// #define PTX_DEMO_APP_PCD_ISO_SEQUENCE ISO_TEST_COMMAND_SEQUENCE_1
// #define PTX_DEMO_APP_PCD_ISO_SEQUENCE ISO_TEST_COMMAND_SEQUENCE_2

/* PICC Loopback Mode commands and responses */
#define PTX_DEMO_APP_PICC_PRECONDITION_LEN                  12
#define PTX_DEMO_APP_PICC_TEST_CMD_LEN                      7
#define PTX_DEMO_APP_PICC_TEST_CMD_RES_DATA_LEN_1           10
#define PTX_DEMO_APP_PICC_TEST_CMD_RES_DATA_LEN_2           256
#define PTX_DEMO_APP_PICC_TEST_CMD_RES_DATA_LEN_3           8
#define WAITING_TIME_TEST_COMMAND_3                         38
const uint8_t PTX_DEMO_APP_PICC_PRECONDITION[PTX_DEMO_APP_PICC_PRECONDITION_LEN] = {0x00, 0xA4, 0x04, 0x00, 0x07, 0xD2, 0x76, 0x00, 0x00, 0x85, 0x01, 0x00};
const uint8_t PTX_DEMO_APP_PICC_TEST_CMD_1[PTX_DEMO_APP_PICC_TEST_CMD_LEN] = {0x00, 0xA4, 0x00, 0x00, 0x02, 0x3F, 0x00};
const uint8_t PTX_DEMO_APP_PICC_TEST_CMD_2[PTX_DEMO_APP_PICC_TEST_CMD_LEN] = {0x00, 0xA4, 0x00, 0x00, 0x02, 0x3F, 0x01};
const uint8_t PTX_DEMO_APP_PICC_TEST_CMD_3[PTX_DEMO_APP_PICC_TEST_CMD_LEN] = {0x00, 0xA4, 0x00, 0x00, 0x02, 0x3F, 0x02};

/* PCD Loopback Mode commands and responses */
#define PTX_DEMO_APP_PCD_CMD_1_LEN 19
#define PTX_DEMO_APP_PCD_CMD_1_RES_LEN 15
#define PTX_DEMO_APP_PCD_CMD_2_PRECON_LEN 39
#define PTX_DEMO_APP_PCD_CMD_2_START_LEN 5
#define PTX_DEMO_APP_PCD_CMD_2_RES_LEN 32

#define PTX_DEMO_APP_PCD_CMD_2_DATA_LEN 256

const uint8_t PTX_DEMO_APP_PCD_CMD_1[PTX_DEMO_APP_PCD_CMD_1_LEN] =
{
    0x00, 0xDA, 0x00, 0x00, 0x0E, 0xAA, 0xAA, 0xF0,
    0xFF, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A,
    0x0B, 0x0C, 0x0D
};
const uint8_t PTX_DEMO_APP_PCD_CMD_1_RES[PTX_DEMO_APP_PCD_CMD_1_RES_LEN] =
{
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
    0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x90, 0x00
};
const uint8_t PTX_DEMO_APP_PCD_CMD_2_PRECONDITION[PTX_DEMO_APP_PCD_CMD_2_PRECON_LEN] =
{
    0x00, 0xA4, 0x04, 0x0C, 0x20, 0x00, 0x01, 0x02,
    0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A,
    0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12,
    0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A,
    0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x90, 0x00
};
const uint8_t PTX_DEMO_APP_PCD_CMD_2_START[PTX_DEMO_APP_PCD_CMD_2_START_LEN] =
{
    0x00, 0xDA, 0x00, 0x00, 0xFF
};
const uint8_t PTX_DEMO_APP_PCD_CMD_2_RES[PTX_DEMO_APP_PCD_CMD_2_RES_LEN] =
{
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
    0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x11,
    0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19,
    0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,
    0x90, 0x00
};

/* Status Word Defines */
#define PTX_DEMO_APP_SW_LEN 2

const uint8_t PTX_DEMO_APP_SW_SUCCESS[PTX_DEMO_APP_SW_LEN] = {0x90, 0x00};

/*
 * ####################################################################################################################
 * INTERNAL FUNCTIONS DECLARATIONS / HELPERS
 * ####################################################################################################################
 */

/*
 * Functions to run demo application in PCD/PICC Loopback mode.
 */
#if !defined(PTX_DISABLE_LISTEN_MODE)
    static ptx_Status_t ptx_RunDemoApplicationPICC(ptx_RUL_t *rul);
#endif /* PTX_DISABLE_LISTEN_MODE */
static ptx_Status_t ptx_RunDemoApplicationPCD(ptx_RUL_t *rul);

#if !defined(PTX_DISABLE_LISTEN_MODE)
    /*
    * State handling functions for PICC Loopback mode.
    */
    static ptx_Status_t ptx_ProcessState_PollHCEEvent(ptx_RUL_HCE_t* hce, ptx_RUL_HCE_EventRecord_t *record, ptx_DemoAppState_t *demoState);
    static ptx_Status_t ptx_ProcessState_HandleHCEEvent(ptx_RUL_HCE_t* hce, ptx_RUL_HCE_EventRecord_t *record, ptx_DemoAppState_t *demoState);
    static ptx_Status_t ptx_ProcessState_HandleHCEData(ptx_RUL_HCE_t* hce, ptx_RUL_HCE_EventRecord_t *record, uint8_t *tx, ptx_DemoAppState_t *demoState);

    /*
    * System Error and Restart functions for PICC Loopback mode.
    */
    static ptx_Status_t ptx_PICC_ProcessState_SystemError(ptx_RUL_HCE_t *hce, ptx_DemoAppState_t *appState, ptx_RUL_Status_Params_t *statusInfoParam);
    static ptx_Status_t ptx_PICC_ProcessState_RestartSystem(ptx_RUL_HCE_t* hce, ptx_DemoAppState_t *appState);
#endif /* PTX_DISABLE_LISTEN_MODE */

/**
 * State handling function for PCD Loopback mode.
 */
static ptx_Status_t ptx_ProcessState_StartDiscovery(ptx_RUL_t *rul, ptx_DemoAppState_t *appState);
static ptx_Status_t ptx_ProcessState_WaitForDiscovery(ptx_RUL_t *rul, ptx_DemoAppState_t *appState, ptx_RUL_CardRegistry_t *cardRegistry);
static ptx_Status_t ptx_ProcessState_DataExchange(ptx_RUL_t *rul, ptx_DemoAppState_t *appState, uint8_t *tx, uint8_t *rx);

/*
 * System Error and Restart functions for PCD Loopback mode.
 */
static ptx_Status_t ptx_PCD_ProcessState_SystemError(ptx_RUL_t *rul, ptx_DemoAppState_t *appState, ptx_RUL_Status_Params_t *statusInfoParam);
static ptx_Status_t ptx_PCD_ProcessState_RestartSystem(ptx_RUL_t *rul, ptx_DemoAppState_t *appState);

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

    ptx_Utils_Print("***********************************************************\n");
    ptx_Utils_Print("****              ISO Loopback Demo %s               ****\n", PTX_DEMO_VERSION);
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
                if (ISO_LOOPBACK_PICC == PTX_DEMO_APP_LOOPBACK_MODE)
                {
#if !defined(PTX_DISABLE_LISTEN_MODE)
                    status = ptx_RunDemoApplicationPICC(&rul);
#endif /* PTX_DISABLE_LISTEN_MODE */
                }
                else
                {
                    status = ptx_RunDemoApplicationPCD(&rul);
                }

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

#if !defined(PTX_DISABLE_LISTEN_MODE)
/*
 * PICC Loopback Application
 */
static ptx_Status_t ptx_RunDemoApplicationPICC(ptx_RUL_t *rul)
{
    ptx_Status_t status = PTX_STATUS_E_GEN_INVALID_PARAMETER;

    ptx_DemoAppState_t demo_state = DemoAppState_PollHCEEvent;
    ptx_RUL_Status_Params_t status_info = {0};

    /* HCE component */
    ptx_RUL_HCE_t hce = {0};
    ptx_RUL_HCE_InitParams_t init_params = {0};
    ptx_RUL_HCE_EventRecord_t record = {0};

    if (PTX_COMP_CHECK(rul, Comp_RUL))
    {
        /* Buffers for data */
        uint8_t rx[PTX_DEMO_APP_BUFFER_SIZE];
        uint32_t rx_len = PTX_DEMO_APP_BUFFER_SIZE;
        uint8_t tx[PTX_DEMO_APP_BUFFER_SIZE];

        init_params.Rul = rul;
        init_params.AppRxBuffer = rx;
        init_params.AppRxBufferSize = rx_len;
        init_params.Log = &rul->Log;

        /* Initialize HCE component with buffers and rul context */
        status = ptx_RUL_HCE_Init(&hce, &init_params);

        if (PTX_STATUS_SUCCESS == status)
        {
            /* Intialize discovery for Listen Mode*/
            ptx_RUL_DiscoverConfig_t disc_config = {0};

            /*Initiate ISO DEP ATS*/
            uint8_t ListenIsoDepAts[PTX_NSC_RF_DISCOVER_ATS_LEN] = {0x05, 0x78, 0x00, 0x71, 0x02};
            uint8_t ListenIsoDepAtsLen = 5;

            disc_config.ListenMode   = 3u;
            disc_config.ListenTypeA  = 1u;
            disc_config.ListenTypeB  = 1u;
            disc_config.ListenIsoDep = 3u;
            disc_config.ListenIsoDepAtsLen = ListenIsoDepAtsLen;
            memcpy(disc_config.ListenIsoDepAts, ListenIsoDepAts, ListenIsoDepAtsLen);

            status = ptx_RUL_InitiateDiscovery(rul, &disc_config);
            if (PTX_STATUS_SUCCESS != status)
            {
                ptx_Utils_Print("RUL-Initiate Discovery failed with Error-Status %04X!\n", status);
            }
            else
            {
                ptx_Utils_Print("RUL-Initiate Discovery successful!\n");
            }
        }

        if (PTX_STATUS_SUCCESS == status)
        {
            ptx_Utils_Print("HCE successfully initialized!\n");
            ptx_Utils_Print("Started ISO PICC Loopback Example!\n");

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

                    switch (demo_state)
                    {
                        case DemoAppState_PollHCEEvent:
                        {
                            status = ptx_ProcessState_PollHCEEvent(&hce, &record, &demo_state);
                            break;
                        }

                        case DemoAppState_HandleHCEEvent:
                        {
                            status = ptx_ProcessState_HandleHCEEvent(&hce, &record, &demo_state);
                            break;
                        }

                        case DemoAppState_HandleHCEData:
                        {
                            status = ptx_ProcessState_HandleHCEData(&hce, &record, &tx[0], &demo_state);
                            break;
                        }

                        case DemoAppState_SystemError:
                        {
                            status = ptx_PICC_ProcessState_SystemError(&hce, &demo_state, &status_info);
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
            while ((PTX_STATUS_SUCCESS == status) && (0 == gExit));
        }
        else
        {
            ptx_Utils_Print("HCE initialization failed with Error-Status %04X!\n", status);
        }

        /* Stop the ISO PICC Loopback Application. */
        (void)ptx_RUL_HCE_DeInit(&hce);

        ptx_Utils_Print("Exiting Demo-Application...\n");
    }

    return status;
}

static ptx_Status_t ptx_PICC_ProcessState_SystemError(ptx_RUL_HCE_t *hce, ptx_DemoAppState_t *appState, ptx_RUL_Status_Params_t *statusInfoParam)
{
    ptx_Status_t status = PTX_STATUS_E_GEN_INVALID_PARAMETER;

    if ((NULL != hce) && (NULL != appState) && (NULL != statusInfoParam))
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
            status = ptx_PICC_ProcessState_RestartSystem(hce, appState);
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

static ptx_Status_t ptx_PICC_ProcessState_RestartSystem(ptx_RUL_HCE_t* hce, ptx_DemoAppState_t *appState)
{
    ptx_Status_t status = PTX_STATUS_E_GEN_INVALID_PARAMETER;

    if ((NULL != hce) && (NULL != appState))
    {
        do
        {
            /* Wait for 1s to give the chip time to cool down, after that time try to initialize the system again */
            status = ptx_RUL_Sleep(hce->Rul, 1000);
            if (PTX_STATUS_SUCCESS == status)
            {
                status = ptx_RUL_InitHardware(hce->Rul);
            }
        }
        while (status != PTX_STATUS_SUCCESS);

        if (PTX_STATUS_SUCCESS == status)
        {
            status = ptx_RUL_HCE_ResetState(hce);
        }

        if (PTX_STATUS_SUCCESS == status)
        {
            *appState = DemoAppState_StartDiscovery;
        }
    }


    return status;
}

static ptx_Status_t ptx_ProcessState_PollHCEEvent(ptx_RUL_HCE_t* hce, ptx_RUL_HCE_EventRecord_t *record, ptx_DemoAppState_t *demoState)
{
    ptx_Status_t status = PTX_STATUS_E_GEN_INVALID_PARAMETER;
    ptx_RUL_HCE_EventRecord_t *tempRecord;

    /* Get HCE Event if available */
    status = ptx_RUL_HCE_GetEvent(hce, &tempRecord);
    if (PTX_STATUS_SUCCESS == status)
    {
        memcpy(record, tempRecord, sizeof(ptx_RUL_HCE_EventRecord_t));
    }

    if (PTX_STATUS_SUCCESS != status)
    {
        if (PTX_STATUS_E_GEN_NOT_FOUND == status)
        {
            /* There is no HCE event available. Set ID to RUL_HCE_EventType_NoEvent and continue. */
            status = PTX_STATUS_SUCCESS;
            record->EventID = RUL_HCE_EventType_NoEvent;
        }
        else
        {
            /* Error in HCE_GetEvent */
            ptx_Utils_Print("Error: HCE_GetEvent status = %d\n", status);
        }
    }

    if ((PTX_STATUS_SUCCESS == status) && (RUL_HCE_EventType_NoEvent != record->EventID))
    {
        *demoState = DemoAppState_HandleHCEEvent;
    }

    return status;
}

static ptx_Status_t ptx_ProcessState_HandleHCEEvent(ptx_RUL_HCE_t* hce, ptx_RUL_HCE_EventRecord_t *record, ptx_DemoAppState_t *demoState)
{
    ptx_Status_t status = PTX_STATUS_E_GEN_INVALID_PARAMETER;

    if ((NULL != hce) && (NULL != record) && (NULL != demoState))
    {
        status = PTX_STATUS_SUCCESS;
        switch (record->EventID)
        {
            case RUL_HCE_EventType_ExtFieldOn:
                ptx_Utils_Print("ExFieldOn\n");
                *demoState = DemoAppState_PollHCEEvent;
                break;
            case RUL_HCE_EventType_ExtFieldOff:
                ptx_Utils_Print("ExFieldOff\n");
                *demoState = DemoAppState_PollHCEEvent;
                break;
            case RUL_HCE_EventType_Activated:
                ptx_Utils_Print("Activated\n");
                *demoState = DemoAppState_PollHCEEvent;
                break;
            case RUL_HCE_EventType_Data:
                ptx_Utils_Print("Received Data\n");
                *demoState = DemoAppState_HandleHCEData;
                break;
            case RUL_HCE_EventType_Deactivated:
            {
                /*
                * Get DEACTIVATE-Reason
                *  => 0x01 = NSC_RF_DEACTIVATE_CMD finished
                *  => 0x02 = DESELECT or Sleep received by Reader
                *  => 0x03 = RELEASE received by Reader
                *  => 0x04 = Field turned off by Reader
                */
                uint8_t reason = record->RxMsgDataShortMsgData[0];
                ptx_Utils_Print("Deactivated reason: %d\n", reason);
                *demoState = DemoAppState_PollHCEEvent;
                break;
            }
            case RUL_HCE_EventType_Error:
            {
                uint8_t error = record->RxMsgDataShortMsgData[0];
                ptx_Utils_Print("Event error: %d\n", error);
                *demoState = DemoAppState_PollHCEEvent;
                break;
            }
            case RUL_HCE_EventType_NoEvent:
            {
                ptx_Utils_Print("No event\n");
                *demoState = DemoAppState_PollHCEEvent;
                break;
            }
            default:
                status = PTX_STATUS_E_GEN_INVALID_PARAMETER;
                break;
        }
    }

    return status;
}

static ptx_Status_t ptx_ProcessState_HandleHCEData(ptx_RUL_HCE_t* hce, ptx_RUL_HCE_EventRecord_t *record, uint8_t *tx, ptx_DemoAppState_t *demoState)
{
    ptx_Status_t status = PTX_STATUS_E_GEN_INVALID_PARAMETER;

    if ((NULL != hce) && (NULL != record) && (NULL != demoState) && (PTX_DEMO_APP_MIN_APDU_LENGTH <= record->RxMsgDataLen))
    {
        if ((record->RxMsgDataLen == PTX_DEMO_APP_PICC_PRECONDITION_LEN) && (0 == memcmp(PTX_DEMO_APP_PICC_PRECONDITION, record->RxMsgData, sizeof(uint8_t) * PTX_DEMO_APP_PICC_PRECONDITION_LEN)))
        {
            memcpy(&tx[0], PTX_DEMO_APP_SW_SUCCESS, PTX_DEMO_APP_SW_LEN);
            status = ptx_RUL_HCE_SendData(hce, &tx[0], PTX_DEMO_APP_SW_LEN);
        }

        if ((PTX_STATUS_SUCCESS != status) && (record->RxMsgDataLen == PTX_DEMO_APP_PICC_TEST_CMD_LEN) && (0 == memcmp(PTX_DEMO_APP_PICC_TEST_CMD_1, record->RxMsgData, sizeof(uint8_t) * PTX_DEMO_APP_PICC_TEST_CMD_LEN)))
        {
            /* Construct response with 10 bytes and status word 0x90 0x00 */
            for (uint32_t i = 0; i < PTX_DEMO_APP_PICC_TEST_CMD_RES_DATA_LEN_1; i++)
            {
                tx[i] = (uint8_t)i;
            }
            memcpy(&tx[PTX_DEMO_APP_PICC_TEST_CMD_RES_DATA_LEN_1], PTX_DEMO_APP_SW_SUCCESS, PTX_DEMO_APP_SW_LEN);

            status = ptx_RUL_HCE_SendData(hce, &tx[0], PTX_DEMO_APP_PICC_TEST_CMD_RES_DATA_LEN_1 + PTX_DEMO_APP_SW_LEN);
        }

        if ((PTX_STATUS_SUCCESS != status) && (record->RxMsgDataLen == PTX_DEMO_APP_PICC_TEST_CMD_LEN) && (0 == memcmp(PTX_DEMO_APP_PICC_TEST_CMD_2, record->RxMsgData, sizeof(uint8_t) * PTX_DEMO_APP_PICC_TEST_CMD_LEN)))
        {
            /* Construct response with 256 bytes and status word 0x90 0x00 */
            for (uint32_t i = 0; i < PTX_DEMO_APP_PICC_TEST_CMD_RES_DATA_LEN_2; i++)
            {
                tx[i] = (uint8_t)i;
            }
            memcpy(&tx[PTX_DEMO_APP_PICC_TEST_CMD_RES_DATA_LEN_2], PTX_DEMO_APP_SW_SUCCESS, PTX_DEMO_APP_SW_LEN);

            status = ptx_RUL_HCE_SendData(hce, &tx[0], PTX_DEMO_APP_PICC_TEST_CMD_RES_DATA_LEN_2 + PTX_DEMO_APP_SW_LEN);
        }

        if ((PTX_STATUS_SUCCESS != status) && (record->RxMsgDataLen == PTX_DEMO_APP_PICC_TEST_CMD_LEN) && (0 == memcmp(PTX_DEMO_APP_PICC_TEST_CMD_3, record->RxMsgData, sizeof(uint8_t) * PTX_DEMO_APP_PICC_TEST_CMD_LEN)))
        {

            /*Perform a sleep to trigger wating timne extension*/
            ptx_RUL_Sleep(hce->Rul, WAITING_TIME_TEST_COMMAND_3);

            /* Construct response with 10 bytes and status word 0x90 0x00 */
            for (uint32_t i = 0; i < PTX_DEMO_APP_PICC_TEST_CMD_RES_DATA_LEN_3; i++)
            {
                tx[i] = (uint8_t)i;
            }
            memcpy(&tx[PTX_DEMO_APP_PICC_TEST_CMD_RES_DATA_LEN_3], PTX_DEMO_APP_SW_SUCCESS, PTX_DEMO_APP_SW_LEN);

            status = ptx_RUL_HCE_SendData(hce, &tx[0], PTX_DEMO_APP_PICC_TEST_CMD_RES_DATA_LEN_3 + PTX_DEMO_APP_SW_LEN);
        }

        *demoState = DemoAppState_PollHCEEvent;
    }

    return status;
}
#endif /* PTX_DISABLE_LISTEN_MODE */

/*
 * PCD Loopback Application
 */
static ptx_Status_t ptx_RunDemoApplicationPCD(ptx_RUL_t *rul)
{
    ptx_Status_t status = PTX_STATUS_E_GEN_INVALID_PARAMETER;

    ptx_DemoAppState_t demo_state = DemoAppState_StartDiscovery;
    ptx_RUL_Status_Params_t status_info = {0};
    ptx_RUL_CardRegistry_t *card_registry = NULL;

    /* Buffers for data */
    uint8_t rx[PTX_DEMO_APP_BUFFER_SIZE];
    uint8_t tx[PTX_DEMO_APP_BUFFER_SIZE];

    if (PTX_COMP_CHECK(rul, Comp_RUL))
    {
        /* Get and store a pointer to the RUL card registry */
        status = ptx_RUL_GetCardRegistry(rul, &card_registry);

        if (PTX_STATUS_SUCCESS == status)
        {
            ptx_Utils_Print("Started ISO PCD Loopback Example!\n");
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

                        case DemoAppState_DataExchange:
                        {
                            status = ptx_ProcessState_DataExchange(rul, &demo_state, &tx[0], &rx[0]);
                            break;
                        }

                        case DemoAppState_Deactivate:
                        {
                            ptx_RUL_DeactivateParams_t deactivate_params = {0};
                            deactivate_params.State = ptx_RUL_DeactivateState_DiscoveryRfReset;
                            deactivate_params.Type = ptx_RUL_DeactivateType_Generic;

                            status = ptx_RUL_Deactivate(rul, &deactivate_params);
                            break;
                        }

                        case DemoAppState_SystemError:
                        {
                            status = ptx_PCD_ProcessState_SystemError(rul, &demo_state, &status_info);
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
        /* Initialize RF-Discovery / Polling Loop in ISO Poll-Mode with standard parameters.*/
        disc_params.PollMode = ptx_RUL_PollMode_Iso;            /* Enable ISO PolL-Mode */
        disc_params.EnablePolling = 1u;                         /* Enable Polling only (Listen-mode can be optionally enabled). */
        disc_params.IdleTime = 100u;                            /* System pauses / idles for 100ms after every full polling loop-cycle. */
        disc_params.PollTypeA = 1u;                             /* Poll for NFC-A Technology. */
        disc_params.PollTypeAPollFrequency = 1u;                /* Poll for NFC-A Technology during every Polling-Loop cycle (otherwise every n-th cycle). */
        disc_params.PollTypeADeviceLimit = 5u;                  /* Limit the number of NFC-A devices / cards to max. 5 devices (application can define max. value). */
        disc_params.PollTypeB = 1u;                             /* Poll for NFC-B Technology. */
        disc_params.PollTypeBPollFrequency = 1u;                /* Poll for NFC-B Technology during every Polling-Loop cycle (otherwise every n-th cycle). */
        disc_params.PollTypeBDeviceLimit = 5u;                  /* Limit the number of NFC-B devices / cards to max. 5 devices (application can define max. value). */
        disc_params.PollIsoDepHbr = ptx_RUL_PollIsoDepHBR_848;  /* Enable High bit rates for ISO-DEP communication */

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
                    (void)ptx_RUL_Sleep(rul, 5);
                    break;
                }

                case ptx_RUL_StatusInfoDiscover_CardDetectionOngoing:
                {
                    /* Multiple cards present, the internal Discovery-procedure is still busy resolving all cards. */
                    ptx_Utils_Print("Multiple Cards found - trying to resolve all Cards ... \n");
                    break;
                }

                case ptx_RUL_StatusInfoDiscover_MultiProtocol: /* FALL THROUGH*/
                case ptx_RUL_StatusInfoDiscover_CardsAvailable:
                {
                    /*
                    * Multiple cards or card supporting multiple protocols present,
                    * this application only supports a single card.
                    * Restarting the application.
                    */
                    ptx_Utils_Print("Multiple Cards/ Multi-protocol card found - Restarting the application.\n");

                    /* Print all available Card-Infos as a List. */
                    for (uint8_t i = 0; i < cardRegistry->CardsCnt; i++)
                    {
                        (void)ptx_Utils_CardInfo_Print_Card_Parameters(rul, i);
                    }

                    *appState = DemoAppState_Deactivate;
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

                    /* Card / Device is ready for (user) data exchange. */
                    *appState = DemoAppState_DataExchange;
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

static ptx_Status_t ptx_PCD_ProcessState_SystemError(ptx_RUL_t *rul, ptx_DemoAppState_t *appState, ptx_RUL_Status_Params_t *statusInfoParam)
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
            status = ptx_PCD_ProcessState_RestartSystem(rul, appState);
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

static ptx_Status_t ptx_PCD_ProcessState_RestartSystem(ptx_RUL_t *rul, ptx_DemoAppState_t *appState)
{
    ptx_Status_t status = PTX_STATUS_E_GEN_INVALID_PARAMETER;

    if (PTX_COMP_CHECK(rul, Comp_RUL) && (NULL != appState))
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

static ptx_Status_t ptx_ProcessState_DataExchange(ptx_RUL_t *rul, ptx_DemoAppState_t *appState, uint8_t *tx, uint8_t *rx)
{
    ptx_Status_t status = PTX_STATUS_E_GEN_INVALID_PARAMETER;
    uint32_t rxLen = 0;
    uint8_t stopExecution = 0;

    if (PTX_COMP_CHECK(rul, Comp_RUL) && (NULL != appState) && (NULL != tx) && (NULL != rx))
    {
        memcpy(&tx[0], PTX_DEMO_APP_PCD_CMD_1, PTX_DEMO_APP_PCD_CMD_1_LEN);
        status = ptx_RUL_Exchange(rul, &tx[0], PTX_DEMO_APP_PCD_CMD_1_LEN, &rx[0], &rxLen, PTX_DEMO_APP_PCD_DATA_EXCHANGE_TIMEOUT_MS);

        if (PTX_STATUS_SUCCESS == status)
        {
            if ((PTX_DEMO_APP_PCD_CMD_1_RES_LEN > rxLen) || (0 != memcmp(&rx[0], PTX_DEMO_APP_PCD_CMD_1_RES, PTX_DEMO_APP_PCD_CMD_1_RES_LEN)))
            {
                stopExecution = 1u;
            }

            if ((PTX_DEMO_APP_PCD_CMD_2_PRECON_LEN <= rxLen) && (0 == memcmp(&rx[0], PTX_DEMO_APP_PCD_CMD_2_PRECONDITION, rxLen)))
            {
                memcpy(&tx[0], PTX_DEMO_APP_PCD_CMD_2_START, PTX_DEMO_APP_PCD_CMD_2_START_LEN);
                for (uint16_t i = 0; i < PTX_DEMO_APP_PCD_CMD_2_DATA_LEN; i++)
                {
                    tx[i + PTX_DEMO_APP_PCD_CMD_2_START_LEN] = (uint8_t)i;
                }

                status = ptx_RUL_Exchange(rul, &tx[0], PTX_DEMO_APP_PCD_CMD_2_START_LEN + PTX_DEMO_APP_PCD_CMD_2_DATA_LEN, &rx[0], &rxLen, PTX_DEMO_APP_PCD_DATA_EXCHANGE_TIMEOUT_MS);

                if (PTX_STATUS_SUCCESS == status)
                {
                    if ((PTX_DEMO_APP_PCD_CMD_2_RES_LEN > rxLen) || (0 != memcmp(&rx[0], PTX_DEMO_APP_PCD_CMD_2_RES, rxLen)))
                    {
                        stopExecution = 1u;
                    }
                    else
                    {
                        stopExecution = 0u;
                    }
                }
            }
        }

        if ((1u == stopExecution) || (PTX_DEMO_APP_PCD_ISO_SEQUENCE == ISO_DEFAULT_TEST_COMMAND_SEQUENCE))
        {
            *appState = DemoAppState_Deactivate;
        }
    }

    return status;
}
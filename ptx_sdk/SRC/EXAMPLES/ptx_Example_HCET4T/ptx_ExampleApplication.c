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
    Module      : SDK Example (Host Card Emulation Typ A - T4T)
    File        : EXAMPLES/ptx_Example_HCET4T/ptx_ExampleApplication.h

    Description : This example shows how to setup Host Card Emulation (HCE) for NFC Typ A emulating a Type 4 Tag (T4T).
                  An exemplary T4T implementation, following the standard can be found in the /HCE_T4T folder. The ptx_RUL_HCE module only handles
                  collecting events and sending data. Which allows more flexibility and modularity in the application.
                  The scenario in this example is as follows:
                  - Initializing all components:
                        * Initialize main Rul component
                        * Initialize Rul hardware
                        * Initialize the T4T implementation
                        * Initialize the RUL_HCE module
                  - Initiate discovery for Type A + Iso Dep
                  - Update emulated ndef memory of T4T to initial value, here "Hello World"
                  - Entering the main loop of the application repeating following steps
                        * Call ptx_RUL_HCE_GetEvent to receive the latest event
                        * If event occurred parse the event accordingly
                  - On application exit, Deinitialize all components in reverse order.

*/

/*
 * ####################################################################################################################
 * INCLUDES
 * ####################################################################################################################
 */

#include "ptx_ExampleApplication.h"
#include "ptx_Status.h"
#include "ptx_RUL.h"
#include "ptx_RUL_HCE.h"
#include "ptx_HCET4T.h"
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

/* Work-buffer max. sizes for current demo example. */
#define PTX_RULHCE_EXAMPLE_BUFFERSIZE     (512u)

/* Define length of custom nfc id used. */
#define PTX_RULHCE_EXAMPLE_NFC_ID_LEN 4

/* Define length of custom ATS */
#define PTX_RULHCE_EXAMPLE_ISO_DEP_ATS_LEN 5

/* Application specific Exit-Flag (controllable by the user on Windows-/Linux-platforms via Ctrl-C). */
static volatile int gExit = 0;

#if defined(PTX_ENABLE_BOARD_EVK)
    /* Corresponds to the voltage on the DC/DC converter on the PTX2xxW Boards. E.g. a value of 330 corresponds to 3.3V. */
    static const uint16_t EVK_DEFAULT_VOLTAGE = 500u;
#endif

/*
 * Compile switch to enable or disable loopback functionality for NFC Forum Listener compliance testing.
 */
//#define PTX_DEMO_APP_ENABLE_NFC_FORUM_LOOPBACK

/*
 * ####################################################################################################################
 * INTERNAL FUNCTIONS DECLARATIONS / HELPERS
 * ####################################################################################################################
 */
static ptx_Status_t ptx_RunDemoApplication(ptx_RUL_t *rul);
static ptx_Status_t ptx_ProcessState_SystemError(ptx_RUL_HCE_t *hce, ptx_DemoAppState_t *appState, ptx_RUL_Status_Params_t *statusInfoParam);
static ptx_Status_t ptx_ProcessState_RestartSystem(ptx_RUL_HCE_t *hce, ptx_DemoAppState_t *appState);

#if defined(PTX_APP_DIVERSITY_WINDOWS)
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
    ptx_Utils_Print("****   Host Card Emulation of Typ A T4T Demo %s        ****\n", PTX_DEMO_VERSION);
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

    /* RUL HCE Extension API component */
    ptx_RUL_HCE_t hce = {0};
    ptx_RUL_HCE_InitParams_t init_params = {0};

    /* HCE T4T Example Extension Add-on API component */
    ptx_HCET4T_t t4t = {0};
    ptx_HCET4T_InitParams_t t4t_init = {0};

    /*
     * Initial example Capability Container CC according to NFC Forum T4T Specification.
     */
    uint8_t init_ccfile[15] = {0x00, 0x0f, 0x20, 0x00, 0x80, 0x00, 0x80, 0x04, 0x06, 0xe1, 0x04, 0x01, 0x00, 0x00, 0x00};
    t4t_init.CCFile = init_ccfile;
    t4t_init.CCFileSize = (uint32_t)sizeof(init_ccfile);

    /*
     * Initial example NDEF Message with protocol https://www. and URL 'renesas.com'
     */
    uint8_t init_ndeffile[512] = {0x00, 0x12, 0xd1, 0x01, 0x0e, 0x55, 0x02, 0x72, 0x65, 0x6e, 0x65, 0x73, 0x61, 0x73, 0x2e, 0x63, 0x6F, 0x6D};
    t4t_init.NDEFFile = init_ndeffile;
    t4t_init.NDEFFileSize = (uint32_t)sizeof(init_ndeffile);

    /* Rx and Tx buffers used as exchange buffers */
    uint8_t rx[PTX_RULHCE_EXAMPLE_BUFFERSIZE] = {0};
    uint32_t rx_len = PTX_RULHCE_EXAMPLE_BUFFERSIZE;
    uint8_t tx[PTX_RULHCE_EXAMPLE_BUFFERSIZE] = {0};
    uint32_t tx_len = PTX_RULHCE_EXAMPLE_BUFFERSIZE;

    ptx_RUL_DiscoverConfig_t disc_config;
    ptx_RUL_HCE_EventRecord_t *record;

    const uint8_t nfc_id[PTX_RULHCE_EXAMPLE_NFC_ID_LEN] = { 0x08, 0x01, 0x02, 0x03 };
    const uint8_t sens_res[PTX_NSC_RF_DISCOVER_SENSRES_LEN] = { 0x01, 0x00 };
    const uint8_t iso_dep_ats[PTX_RULHCE_EXAMPLE_ISO_DEP_ATS_LEN] = {0x05, 0x78, 0x00, 0x71, 0x02};

    if (PTX_COMP_CHECK(rul, Comp_RUL))
    {
        /* Initialize RUL HCE Extension API component */
        init_params.AppRxBuffer     = rx;
        init_params.AppRxBufferSize = rx_len;
        init_params.Rul             = rul;
        init_params.Log             = &rul->Log;

        status = ptx_RUL_HCE_Init(&hce, &init_params);

        if (PTX_STATUS_SUCCESS == status)
        {
            ptx_Utils_Print("RUL HCE initialization successful!\n");
        }
        else
        {
            ptx_Utils_Print("RUL HCE initialization failed with Error-Status %04X!\n", status);
        }

        /* Initialize HCE T4T Example Extension Add-on API component. */
        status = ptx_HCET4T_Init(&t4t, &t4t_init);

        if (PTX_STATUS_SUCCESS == status)
        {
            ptx_Utils_Print("HCE T4T initialization successful!\n");
        }
        else
        {
            ptx_Utils_Print("HCE T4T initialization failed with Error-Status %04X!\n", status);
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
                            memset(&disc_config, 0, sizeof(ptx_RUL_DiscoverConfig_t));

                            /* Enable Listen Mode using Type-A + Field on/off notifications, enable internal ISO-DEP handling. */
                            disc_config.ListenMode   = 3u;
                            disc_config.ListenTypeA  = 1u;
                            disc_config.ListenIsoDep = 1u;

                            /* Set custom 4 byte nfc id */
                            disc_config.ListenTypeANfcIdLen = PTX_RULHCE_EXAMPLE_NFC_ID_LEN;
                            memcpy(disc_config.ListenTypeANfcId, nfc_id, PTX_RULHCE_EXAMPLE_NFC_ID_LEN);

                            /* Set custom sens response */
                            disc_config.ListenTypeASensResLen = PTX_NSC_RF_DISCOVER_SENSRES_LEN;
                            memcpy(disc_config.ListenTypeASensRes, sens_res, PTX_NSC_RF_DISCOVER_SENSRES_LEN);

                            /* Use custom ATS */
                            disc_config.ListenIsoDepAtsLen = PTX_RULHCE_EXAMPLE_ISO_DEP_ATS_LEN;
                            memcpy(disc_config.ListenIsoDepAts, iso_dep_ats, PTX_RULHCE_EXAMPLE_ISO_DEP_ATS_LEN);

                            status = ptx_RUL_InitiateDiscovery(rul, &disc_config);

                            if (PTX_STATUS_SUCCESS == status)
                            {
                                ptx_Utils_Print("RUL-Initiate Discovery successful!\n");
                                ptx_Utils_Print("Starting T4T-A emulation!\n");

                                demo_state = DemoAppState_ProcessEvents;
                            }
                            else
                            {
                                ptx_Utils_Print("RUL-Initiate Discovery failed with Error-Status %04X!\n", status);
                            }
                            break;
                        }

                        case DemoAppState_ProcessEvents:
                        {
                            status = ptx_RUL_HCE_GetEvent(&hce, &record);

                            if ((PTX_STATUS_SUCCESS != status) && (PTX_STATUS_E_GEN_NOT_FOUND != status))
                            {
                                ptx_Utils_Print("Error: ptx_RUL_HCE_GetEvent status = %d\n", status);
                            }

                            if ((PTX_STATUS_SUCCESS == status) && (RUL_HCE_EventType_NoEvent != record->EventID))
                            {
                                switch (record->EventID)
                                {
                                    case RUL_HCE_EventType_ExtFieldOn:
                                    {
                                        status = ptx_HCET4T_Reset(&t4t);
                                        ptx_Utils_Print("Ext. RF-Field = On\n");
                                        break;
                                    }

                                    case RUL_HCE_EventType_ExtFieldOff:
                                    {
                                        status = ptx_HCET4T_Reset(&t4t);
                                        ptx_Utils_Print("Ext. RF-Field = Off\n");
                                        break;
                                    }

                                    case RUL_HCE_EventType_Activated:
                                    {
                                        status = PTX_STATUS_SUCCESS;
                                        ptx_Utils_Print("Activated by remote Poller-/Reader\n");
                                        break;
                                    }

                                    case RUL_HCE_EventType_Data:
                                    {
                                        ptx_Utils_Print("Data received from remote Poller-/Reader\n");

                                        /* reset Tx buffer size to the initial max size */
                                        tx_len = PTX_RULHCE_EXAMPLE_BUFFERSIZE;

#if defined(PTX_DEMO_APP_ENABLE_NFC_FORUM_LOOPBACK)
                                        status = ptx_HCET4T_NFCForumLoopback_CommandProcess(&t4t, record->RxMsgData, record->RxMsgDataLen, tx, &tx_len);
#else
                                        status = ptx_HCET4T_CommandProcess(&t4t, record->RxMsgData, record->RxMsgDataLen, tx, &tx_len);
#endif
                                        if ((status == PTX_STATUS_SUCCESS) && (0 != tx_len))
                                        {
                                            status = ptx_RUL_HCE_SendData(&hce, tx, tx_len);

                                            if (PTX_STATUS_SUCCESS != status)
                                            {
                                                ptx_Utils_Print("Error: ptx_RUL_HCE_SendData status: %d\n", status);
                                            }
                                        }
                                        else
                                        {
                                            ptx_Utils_Print("Error: ptx_HCET4T_CommandProcess status: %d\n", status);
                                        }
                                        break;
                                    }

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
                                        ptx_Utils_Print("Deactivated by remote Poller-/Reader; Reason: %d\n", reason);
                                        break;
                                    }

                                    case RUL_HCE_EventType_Error:
                                    {
                                        uint8_t error = record->RxMsgDataShortMsgData[0];
                                        ptx_Utils_Print("RF-Error: %d\n", error);
                                        break;
                                    }

                                    case RUL_HCE_EventType_NoEvent:
                                    /* No Event, Fall through */
                                    default:
                                        /* ignore */
                                        break;
                                }
                            }

                            if (PTX_STATUS_E_GEN_NOT_FOUND == status)
                            {
                                /* No Event found - reset status to stay in application loop */
                                status = PTX_STATUS_SUCCESS;
                            }
                            break;
                        }

                        case DemoAppState_SystemError:
                        {
                            status = ptx_ProcessState_SystemError(&hce, &demo_state, &status_info);
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

            status = ptx_HCET4T_DeInit(&t4t);

            if (PTX_STATUS_SUCCESS == status)
            {
                ptx_Utils_Print("HCE T4T de-initialization successful!\n");
            }
            else
            {
                ptx_Utils_Print("HCE T4T de-initialization failed with Error-Status %04X!\n", status);
            }

            status = ptx_RUL_HCE_DeInit(&hce);

            if (PTX_STATUS_SUCCESS == status)
            {
                ptx_Utils_Print("RUL HCE de-initialization successful!\n");
            }
            else
            {
                ptx_Utils_Print("RUL HCE de-initialization failed with Error-Status %04X!\n", status);
            }
        }

        ptx_Utils_Print("Exiting Demo-Application...\n");
    }

    return status;
}

static ptx_Status_t ptx_ProcessState_SystemError(ptx_RUL_HCE_t *hce, ptx_DemoAppState_t *appState, ptx_RUL_Status_Params_t *statusInfoParam)
{
    ptx_Status_t status = PTX_STATUS_E_GEN_INVALID_PARAMETER;

    if ((NULL != hce) && (NULL != appState) && (NULL != statusInfoParam))
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
            status = ptx_ProcessState_RestartSystem(hce, appState);
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

static ptx_Status_t ptx_ProcessState_RestartSystem(ptx_RUL_HCE_t *hce, ptx_DemoAppState_t *appState)
{
    ptx_Status_t status = PTX_STATUS_E_GEN_INVALID_PARAMETER;

    if ((NULL != hce) && (NULL != appState))
    {
        do
        {
            /* Wait for 1s to give the chip time to cool down, after that time try to initialize the system again */
            status = ptx_RUL_Sleep(hce->Rul, 1000u);

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

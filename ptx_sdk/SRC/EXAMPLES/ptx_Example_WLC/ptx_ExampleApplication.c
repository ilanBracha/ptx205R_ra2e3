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
    Module      : SDK Example (WLC Poller)
    File        : EXAMPLES/ptx_Example_WLC/ptx_ExampleApplication.c

    Description : This application example demonstrates how to use the Wireless Charging Poller (WLC Poller) API.
                  It performs a wireless charging procedure together with a Wireless Charging Listener (WLC Listener).
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
#include "ptx_RUL_WLC.h"
#include "ptx_RUL_WLC_Proprietary.h"

#if defined(PTX_ENABLE_PROPRIETARY_TDC)
    #include "ptx_RUL_WLC_TDC_Helper.h"
#endif
#if defined(PTX_APP_DIVERSITY_WINDOWS)
    #include <wincon.h>
#endif
#if defined(PTX_APP_DIVERSITY_LINUX)
    #include <signal.h>
    #include <stdlib.h>
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

/* Major WLC protocol version number. */
static const uint8_t PTX_DEMO_APP_WLC_PROTOCOL_VERSION_MAJOR = 2;

/* Minor WLC protocol version number. */
static const uint8_t PTX_DEMO_APP_WLC_PROTOCOL_VERSION_MINOR = 0;

/* Power class that is sent along the WLC Information Record. */
static const ptx_NDEFRecord_WLCINF_PowerClass_t PTX_DEMO_APP_WLC_POWER_CLASS_USED = PowerClass3;

/* Lowest possible charging power level applied by the Poller. This power level will also be used for static mode. (Range: 0 - 177) */
static const uint8_t PTX_DEMO_APP_WLC_POWER_LEVEL_MIN = 44; 

/* Highest possible charging power level applied by the Poller. (Range: 0 - 177) */
static const uint8_t PTX_DEMO_APP_WLC_POWER_LEVEL_MAX = 133;

/* Initial power level used for negotiated charging. (Range: 0 - 177)
 * Setting the initial power level to 0 indicates that the poller shall use
 * the default polling power level (from the RF-configuration) for the initial WPT cycle.
 */
static const uint8_t PTX_DEMO_APP_WLC_POWER_LEVEL_INITIAL = 0;

/* The power level applied after a finished WPT charging phase (Range: 0 - 177). Will only be used depending on
 * PTX_DEMO_APP_WLC_POST_WPT_POWER_THRESHOLD and PTX_DEMO_APP_WLC_POST_WPT_POWER_OPTION.
 * Needs to be between the introduced minimum and maximum levels (with the exception of '0'-value).
 * An post power level of 0 indicates that the Poller shall use the default polling power level (from the RF-configuration)
 * as PostPowerLevel.
 */
static const uint8_t PTX_DEMO_APP_WLC_POST_WPT_POWER_LEVEL = 0;

/* Option used to decide which power level to use after a finished WPT charging phase (Post Power Level or Charging Power Level). */
static const ptx_RUL_WLC_WptPostPwrOption_t PTX_DEMO_APP_WLC_POST_WPT_POWER_OPTION = WptPostPwrOption_B;

/* Threshold used to decide which power level to use after a finished WPT charging phase (Post Power Level or Charging Power Level) (Range: 0 - 177).
 * The value needs to be between the introduced minimum and maximum levels.
 */
static const uint8_t PTX_DEMO_APP_WLC_POST_WPT_POWER_THRESHOLD = 89;

/* Time between polling cycles during discovery. */
static const uint32_t PTX_DEMO_APP_WLC_DISCOVERY_IDLE_TIME = 500;

/* Polling interval, to check if listener is still present during Sleep State (e.g. when battery full).
 * This parameter has to be a power of 2!
 */
#if defined(PTX_ENABLE_NFC_FORUM_TESTING)
    static const uint32_t PTX_DEMO_APP_WLC_SLEEP_INTERVAL = 1 << 14;
#else
    static const uint32_t PTX_DEMO_APP_WLC_SLEEP_INTERVAL = 1 << 10;
#endif

/* Enable/Disable Background Foreign Object Detection (bFOD). */
static const uint8_t PTX_DEMO_APP_WLC_WPT_ICD_EN = 0;

/* Foreign object detection guard time duration.
 * bFOD is enabled after this delay (in ms) upon WPT start.
 */
static const uint8_t PTX_DEMO_APP_WLC_WPT_ICD_GUARD_TIME = 10;

/* Number of LPCD loops before next full poller discovery phase. */
#if defined(PTX_ENABLE_NFC_FORUM_TESTING)
    static const uint8_t PTX_DEMO_APP_WLC_LPCD_LOOPS = 0;
#else
    static const uint8_t PTX_DEMO_APP_WLC_LPCD_LOOPS = 20;
#endif

/* Chip will send a notification to the host MCU once the LPCD is triggered. */
static const uint8_t PTX_DEMO_APP_WLC_LPCD_NTF_EN = 0;

/* Number of maximum retries for reading the control record of the listener. Min 0, Max 3. */
static const uint8_t PTX_DEMO_APP_WLC_NWCC_MAX_RETRIES = 3;

/* Delay for retrying to read the control record of the listener in ms. Min 30. */
static const uint8_t PTX_DEMO_APP_WLC_NWCC_RETRY_DELAY = 35;

/* Additional delay time in ms between writing the Info record and reading the control record.
 * This delay is additionally added to the requested delay by the listener! Min 0, Max 20.
 */
static const uint8_t PTX_DEMO_APP_WLC_WT_MARGIN_DELAY = 0;

/* Settle duration in ms after power level has been decreased, subsequent to a successful WPT cycle.
 * Currently, no values below 1ms are supported! Theoretical minimum is 300 us but current minimum is 1 ms.
 */
static const uint8_t PTX_DEMO_APP_WLC_POWER_SETTLE_DURATION = 0;

/* Dummy Data Length for the proprietary TDC example. */
#define PTX_DEMO_APP_WLC_TDC_DUMMY_DATA_LENGTH (256)

/*
 * ####################################################################################################################
 * INTERNAL FUNCTIONS DECLARATIONS / HELPERS
 * ####################################################################################################################
 */

static ptx_Status_t ptx_RunDemoApplication(ptx_RUL_t *rul);
static void ptx_WLC_NotificationCallback (void *wlc, ptx_RUL_WLC_Notification_t wlcEvent, void *data, uint32_t dataLen);

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
    ptx_Utils_Print("****   RUL WLC Example %s             ****\n", PTX_DEMO_VERSION);
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

    /* RUL WLC API component. */
    ptx_RUL_WLC_t wlc = {0};
    ptx_RUL_WLC_InitParams_t init_params        = {0};
    ptx_RUL_DiscoverConfig_t  disc_config       = {0};

    disc_config.EnablePolling                   = 1u;
    disc_config.PollMode                        = ptx_RUL_PollMode_NfcForum;
    disc_config.EnableLPCD                      = PTX_DEMO_APP_WLC_LPCD_LOOPS;
    disc_config.EnableLPCDNtf                   = PTX_DEMO_APP_WLC_LPCD_NTF_EN;
    disc_config.IdleTime                        = PTX_DEMO_APP_WLC_DISCOVERY_IDLE_TIME;

    disc_config.StandbyCfg.EnableWuByTimer      = 1u;
    disc_config.StandbyCfg.EnableWuByHif        = 1u;

    disc_config.PollTypeA                       = 1u;
    disc_config.PollTypeAPollFrequency          = 1u;

    disc_config.PollTypeB                       = 1u;
    disc_config.PollTypeBPollFrequency          = 1u;

    disc_config.PollTypeF212                    = 1u;
    disc_config.PollTypeFPollFrequency          = 1u;

    disc_config.PollTypeV                       = 1u;
    disc_config.PollTypeVPollFrequency          = 1u;

    disc_config.DisablePollIsoDep               = 1u;

    /** General Parameters. */
    init_params.Rul                             = rul;
    init_params.Log                             = &rul->Log;
    init_params.NtfCb                           = ptx_WLC_NotificationCallback;

    /** NFC LE Parameters. */
    init_params.NfcLeDiscoverConfig             = disc_config;

    /** WLC CAP Parameters. */
    init_params.WlcCapVersionMajor              = PTX_DEMO_APP_WLC_PROTOCOL_VERSION_MAJOR;
    init_params.WlcCapVersionMinor              = PTX_DEMO_APP_WLC_PROTOCOL_VERSION_MINOR;

    /** WLC INFO Parameters. */
    init_params.WlcInfoPowerClass               = PTX_DEMO_APP_WLC_POWER_CLASS_USED;
    init_params.WlcInfoWaitingTimeMarginDelay   = PTX_DEMO_APP_WLC_WT_MARGIN_DELAY;

    /** WLC CTRL Parameters. */
    init_params.WlcCtrlReadRetryDelay           = PTX_DEMO_APP_WLC_NWCC_RETRY_DELAY;
    init_params.WlcCtrlMaxReadRetries           = PTX_DEMO_APP_WLC_NWCC_MAX_RETRIES;

    /** WPT Cycle Parameters. */
    init_params.WptPowerLevelMin                = PTX_DEMO_APP_WLC_POWER_LEVEL_MIN;
    init_params.WptPowerLevelMax                = PTX_DEMO_APP_WLC_POWER_LEVEL_MAX;
    init_params.WptPowerLevelInitial            = PTX_DEMO_APP_WLC_POWER_LEVEL_INITIAL;
    init_params.WptIcdEnable                    = PTX_DEMO_APP_WLC_WPT_ICD_EN;
    init_params.WptIcdGuardTime                 = PTX_DEMO_APP_WLC_WPT_ICD_GUARD_TIME;
    init_params.WptPostPwrLevel                 = PTX_DEMO_APP_WLC_POST_WPT_POWER_LEVEL;
    init_params.WptPostPwrThreshold             = PTX_DEMO_APP_WLC_POST_WPT_POWER_THRESHOLD;
    init_params.WptPostPwrOption                = PTX_DEMO_APP_WLC_POST_WPT_POWER_OPTION;
    init_params.WptPostSettleTime               = PTX_DEMO_APP_WLC_POWER_SETTLE_DURATION;
    
    /** WLC Sleep Parameters. */
    init_params.SleepInterval                   = PTX_DEMO_APP_WLC_SLEEP_INTERVAL;

    if (PTX_COMP_CHECK(rul, Comp_RUL))
    {
        status = ptx_RUL_WLC_Init(&wlc, &init_params);
        ptx_Utils_Print("Opening RUL WLC completed with Status %04X!\n", status);

        if (PTX_STATUS_SUCCESS == status)
        {
            wlc.StateMachineActive = 1;

            do
            {
                if (1 == gExit)
                {
                    wlc.StateMachineActive = 0;
                }

                /* Check regularly for System-state/-health (e.g. temperature/over-current errors etc.) */
                ptx_RUL_Status_Params_t status_params;
                status_params.Type = ptx_RUL_StatusType_System;

                if((State_Error != wlc.State) && (State_Off_DeactivateListener != wlc.State))
                {

                    if (PTX_STATUS_SUCCESS == status)
                    {
                        status = ptx_RUL_GetStatusInfo(rul, &status_params);
                    }

                    if (PTX_STATUS_SUCCESS == status)
                    {
                        if (ptx_RUL_StatusInfoSystem_OK != status_params.Info.System)
                        {
                            /* System-Errors-/-Warnings are critical and needs to be resolved at integration-/application-level.
                            * There can be multiple root causes those errors/warnings, but in most cases they are related
                            * the Hardware-environment (e.g. PCB, component issues etc.).
                            */
                            wlc.State = State_Error;
                        }
                    }
                    else
                    {
                        wlc.State = State_Error;
                    }

                }

                switch (wlc.State)
                {

                    case State_NFCLE_StartPollListener:
                        status = ptx_RUL_WLC_StateNFCLE_StartPollListener(&wlc);
                        break;

                    case State_NFCLE_PollListener:
                        status = ptx_RUL_WLC_StateNFCLE_PollListener(&wlc);
                        break;

                    case State_NFCLE_ActivateListener:
                        status = ptx_RUL_WLC_StateNFCLE_ActivateListener(&wlc);
                        break;

                    case State_WCCA_ReadCapabilityRecord:
                        status = ptx_RUL_WLC_StateWCCA_ReadCapabilityRecord(&wlc);
                        break;

                    case State_WCCA_StartPollListenerReady:
                        status = ptx_RUL_WLC_StateWCCA_StartPollListenerReady(&wlc);
                        break;

                    case State_WCCA_PollListenerReady:
                        status = ptx_RUL_WLC_StateWCCA_PollListenerReady(&wlc);
                        break;

                    case State_WCC_WriteInfoRecord:
                        status = ptx_RUL_WLC_StateWCC_WriteInfoRecord(&wlc);
                        break;

                    case State_WCC_ReadControlRecord:
                    {
                        ptx_RUL_WLC_ListenerType_t listenerType;
                        ptx_RUL_WLC_GetListenerType(&wlc, &listenerType);

#if defined(PTX_ENABLE_PROPRIETARY_GPIO_CTRL)
                        /*
                         * ############ Proprietary GPIO Example ############
                         */
                        if ((ListenerType_Proprietary == listenerType) && (PTX_STATUS_SUCCESS == status))
                        {
                            uint8_t ctl_ctr;
                            status = ptx_NDEFRecord_WLCCTL_GetCounter(&wlc.RecordWlcCtl, &ctl_ctr);
                            if (PTX_STATUS_SUCCESS == status)
                            {
                                if (ctl_ctr % 2 == 0)
                                {
                                    status = ptx_RUL_WLC_Proprietary_SetListenerGpios(&wlc, 1, 0);
                                }
                                else
                                {
                                    status = ptx_RUL_WLC_Proprietary_SetListenerGpios(&wlc, 0, 1);
                                }
                            }
                        }
#endif
#if defined(PTX_ENABLE_PROPRIETARY_LISTENER_SYSTEM_STATUS)
                        /*
                         * ############ Proprietary Listener System Status Example ############
                         */
                        if ((ListenerType_Proprietary == listenerType) && (PTX_STATUS_SUCCESS == status))
                        {
                            ptx_RUL_WLC_Proprietary_ListenerSystemStatus_t listenerStatus;
                            status = ptx_RUL_WLC_Proprietary_GetListenerSystemStatus(&wlc, &listenerStatus);

                            if (PTX_STATUS_SUCCESS == status)
                            {
                                switch (listenerStatus.NtcStatus)
                                {
                                    case NtcStatus_Normal:
                                        ptx_Utils_Print("Listener NTC Temperature Normal.\n");
                                        break;

                                    case NtcStatus_Cold:
                                        ptx_Utils_Print("Listener NTC Temperature Cold.\n");
                                        break;

                                    case NtcStatus_ECold:
                                        ptx_Utils_Print("Listener NTC Temperature Extended Cold.\n");
                                        break;

                                    case NtcStatus_Hot:
                                        ptx_Utils_Print("Listener NTC Temperature Hot.\n");
                                        break;

                                    case NtcStatus_EHot:
                                        ptx_Utils_Print("Listener NTC Temperature Extended Hot.\n");
                                        break;

                                    default: /*< Fall through */
                                    case NtcStatus_Disabled:
                                        ptx_Utils_Print("Listener NTC Disabled.\n");
                                        break;
                                }
                            }
                        }
#endif
#if defined(PTX_ENABLE_PROPRIETARY_TDC)
                        /*
                         * ############ Proprietary TDC Example ############
                         */
                        if ((ListenerType_Proprietary == listenerType) && (PTX_STATUS_SUCCESS == status))
                        {
                            /* Reserve some memory. */
                            uint8_t data[PTX_DEMO_APP_WLC_TDC_DUMMY_DATA_LENGTH];
                            uint32_t dataLen = sizeof(data);

                            /* Generate dummy data */
                            for (uint16_t i = 0; i < PTX_DEMO_APP_WLC_TDC_DUMMY_DATA_LENGTH; ++i)
                            {
                                data[i] = (uint8_t) i;
                            }

                            /* Transmit the data using simple high level protocol. */
                            status = ptx_RUL_WLC_TDC_HlpTx(&wlc, data, dataLen);
                            /* Check if transmission was successful. */
                            if (PTX_STATUS_SUCCESS == status)
                            {
                                /* Clear the TX buffer.*/
                                memset(data, 0x00, PTX_DEMO_APP_WLC_TDC_DUMMY_DATA_LENGTH);
                                /* Reuse the TX buffer for RX payload. */
                                status = ptx_RUL_WLC_TDC_HlpRx(&wlc, data, &dataLen);
                                /* Check if RX operation was successful. */
                                if (PTX_STATUS_SUCCESS == status)
                                {
                                    /* Do something with the data. For example: */
                                    data[0] = data[0] + 1;
                                }
                            }
                        }
#endif
                    }
                    status = ptx_RUL_WLC_StateWCC_ReadControlRecord(&wlc);
                    break;

                    case State_WPT_StartStaticCharging:
                        status = ptx_RUL_WLC_StateWPT_StartStaticCharging(&wlc);
                        break;

                    case State_WPT_StartNegotiatedCharging:
                        status = ptx_RUL_WLC_StateWPT_StartNegotiatedCharging(&wlc);
                        break;

                    case State_WPT_PollCharging:
                        status = ptx_RUL_WLC_StateWPT_PollCharging(&wlc);
                        break;

                    case State_Sleep_StartSleep:
                    {
                        ptx_RUL_WLC_ListenerType_t listenerType;
                        ptx_RUL_WLC_GetListenerType(&wlc, &listenerType);
#if defined(PTX_ENABLE_PROPRIETARY_SHIPPING_MODE_ENABLE_CTRL)
                        /*
                         * ############ Proprietary Shipping Mode Example ############
                         */
                        if ((ListenerType_Proprietary == listenerType) && (PTX_STATUS_SUCCESS == status))
                        {
                            status = ptx_RUL_WLC_Proprietary_SetListenerShippingModeEnable(&wlc);
                        }
#endif
                    }
                    status = ptx_RUL_WLC_StateSleep_StartSleep(&wlc);
                    break;

                    case State_Sleep_PollSleep:
                        status = ptx_RUL_WLC_StateSleep_PollSleep(&wlc);
                        break;

                    case State_Off_DeactivateListener:
                        status = ptx_RUL_WLC_StateOff_DeactivateListener(&wlc);
                        break;

                    default: /*< Fall through */
                    case State_Error:
                        status = ptx_RUL_WLC_StateError_ProcessError(&wlc, status, status_params.Info.System);
                        break;
                }



            }
            while (1u == wlc.StateMachineActive);

            (void) ptx_RUL_WLC_DeInit(&wlc);
        }

        ptx_Utils_Print("Exiting Demo-Application...\n");
    }

    return status;
}


static void ptx_WLC_NotificationCallback(void *ctx, ptx_RUL_WLC_Notification_t wlcEvent, void *data, uint32_t dataLen)
{
    switch (wlcEvent)
    {
        case Notification_PollingStarted:
            /* place holder for event report system-wise. */
            break;

        case Notification_ListenerDetected:
            /* place holder for event report system-wise. */
            break;

        case Notification_ListenerActivated:
            /* place holder for event report system-wise. */
            break;

        case Notification_CapabilityRecordReceived:
        {
            ptx_NDEFRecord_WLCCAP_Mode_t wlc_mode;
            ptx_NDEFRecord_WLCCAP_t *cap_record = (ptx_NDEFRecord_WLCCAP_t *) data;
            (void) ptx_NDEFRecord_WLCCAP_GetMode(cap_record, &wlc_mode);

            switch (wlc_mode)
            {
                case Mode_Static:
                    /* Add your own implementation here */
                    break;
                case Mode_Negotiated:
                    /* Add your own implementation here */
                    break;
                case Mode_BattFull:
                    /* Add your own implementation here */
                    break;
                default:
                    break;
            }
        }
        break;

        case Notification_StatusInfoRecordReceived:
            /* place holder for event report system-wise. */
            break;

        case Notification_DeviceInfoRecordReceived:
            /* place holder for event report system-wise. */
            break;

        case Notification_ControlRecordReceived:
        {
            /* Lets check the current battery level. */
            uint8_t battery_level;
            ptx_NDEFRecord_WLCCTL_t *ctl_record = (ptx_NDEFRecord_WLCCTL_t *) data;
            (void) ptx_NDEFRecord_WLCCTL_GetBatteryLevel(ctl_record, &battery_level);

            /* Did the Listener request another WPT cycle? */
            uint8_t wpt_request;
            (void) ptx_NDEFRecord_WLCCTL_GetWptReq(ctl_record, &wpt_request);
            /* Add your own implementation here */
        }
        break;

        case Notification_StaticWptCycleStarted:
            /* place holder for event report system-wise. */
            break;

        case Notification_NegotiatedWptCycleStarted:
            /* place holder for event report system-wise. */
            break;

        case Notification_WptCycleFinished:
            /* place holder for event report system-wise. */
            break;

        case Notification_SleepStarted:
            /* place holder for event report system-wise. */
            break;

        case Notification_SleepFinished:
            /* place holder for event report system-wise. */
            break;

        case Notification_ListenerDeactivated:
            /* place holder for event report system-wise. */
            break;

        case Notification_Error_InvalidState:
            /* place holder for event report system-wise. */
            break;

        case Notification_Error_IncompatibleTagProtocol:
            /* place holder for event report system-wise. */
            break;

        case Notification_Error_InvalidWlcProtocolVersion:
            /* place holder for event report system-wise. */
            break;

        case Notification_Error_CapabilityRecordMissing:
            /* place holder for event report system-wise. */
            break;

        case Notification_Error_NegoWaitMaxRetries:
            /* place holder for event report system-wise. */
            break;

        case Notification_Error_ControlRecordMissing:
            /* place holder for event report system-wise. */
            break;

        case Notification_Error_ControlRecordMaxRetries:
            /* place holder for event report system-wise. */
            break;

        case Notification_Error_InvalidMode:
            /* place holder for event report system-wise. */
            break;

        case Notification_Error_ProtocolError:
            /* place holder for event report system-wise. */
            break;

        case Notification_Error_InvalidHoldoffDuration:
            /* place holder for event report system-wise. */
            break;

        case Notification_Error_WptIcd:
            /* place holder for event report system-wise. */
            break;

        case Notification_Error_WptTimeout:
        {
            ptx_RUL_WLC_t *wlc = ctx;
            ptx_Status_t *status = (ptx_Status_t *) data;
            if (PTX_STATUS_WLC_WPT_TIMEOUT == *status)
            {
                ptx_RUL_WLC_InitParams_t init_params = {0};
                memcpy(&init_params, &(wlc->InitParams), sizeof(ptx_RUL_WLC_InitParams_t));

                *status = ptx_RUL_WLC_DeInit(wlc);

                if (PTX_STATUS_SUCCESS == *status)
                {
                    *status = ptx_RUL_InitHardware(init_params.Rul);
                }

                if (PTX_STATUS_SUCCESS == *status)
                {
                    *status = ptx_RUL_WLC_Init(wlc, &init_params);
                }
            }
            break;
        }
        case Notification_Error_SystemStatusInfo:
        {
            ptx_RUL_WLC_t *wlc = ctx;
            ptx_RUL_StatusInfoSystem_t *statusInfo = (ptx_RUL_StatusInfoSystem_t *) data;
            if (ptx_RUL_StatusInfoSystem_OK != *statusInfo)
            {
                ptx_RUL_WLC_InitParams_t init_params = {0};
                memcpy(&init_params, &(wlc->InitParams), sizeof(ptx_RUL_WLC_InitParams_t));

                ptx_Status_t status = ptx_RUL_WLC_DeInit(wlc);

                if (PTX_STATUS_SUCCESS == status)
                {
                    status = ptx_RUL_InitHardware(init_params.Rul);
                }

                if (PTX_STATUS_SUCCESS == status)
                {
                    status = ptx_RUL_WLC_Init(wlc, &init_params);
                }

                ptx_RUL_Status_Params_t status_params;
                if (PTX_STATUS_SUCCESS == status)
                {
                    status_params.Type = ptx_RUL_StatusType_System;
                    status = ptx_RUL_GetStatusInfo(wlc->InitParams.Rul, &status_params);
                }

                if (PTX_STATUS_SUCCESS == status)
                {
                    *statusInfo = status_params.Info.System;
                }
            }
            break;
        }


        case Notification_Error_Internal:
            /* place holder for event report system-wise. */
            break;


        default :
            break;
    }

    (void) dataLen;
}

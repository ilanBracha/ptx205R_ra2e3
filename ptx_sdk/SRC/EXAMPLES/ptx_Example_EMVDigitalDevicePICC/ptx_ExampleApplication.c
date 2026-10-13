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
    Module      : SDK Example (EMV PICC Digital Device Application)
    File        : EXAMPLES/ptx_Example_EMVDigitalDevicePICC/ptx_ExampleApplication.c

    Description : This application demonstrates the use of the EMV Digital Device PICC
                  application using the HCE add-on API. The application is used for
                  compliance / type-approval testing.
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

/**
 * \name EMV RAPDUs
 * @{
 */
#define PTX_DEMO_APP_READ_RECORD_RAPDU_LEN  4
#define PTX_DEMO_APP_INS4_RAPDU_LEN         3
#define PTX_DEMO_APP_GPO_RAPDU_LEN          3
#define PTX_DEMO_APP_RAPDU_STATUS_WORD_LEN  2
#define PTX_DEMO_APP_SELECT_RAPDU_LEN       3

const uint8_t PTX_DEMO_APP_READ_RECORD_RAPDU[PTX_DEMO_APP_READ_RECORD_RAPDU_LEN] = { 0x01, 0x02, 0x90, 0x00 };
const uint8_t PTX_DEMO_APP_INS4_RAPDU[PTX_DEMO_APP_INS4_RAPDU_LEN]               = { 0xFF, 0x90, 0x00 };
const uint8_t PTX_DEMO_APP_GPO_RAPDU[PTX_DEMO_APP_GPO_RAPDU_LEN]                 = { 0x77, 0x90, 0x00 };
const uint8_t PTX_DEMO_APP_RAPDU_STATUS_WORD[PTX_DEMO_APP_RAPDU_STATUS_WORD_LEN] = { 0x90, 0x00 };
const uint8_t PTX_DEMO_APP_SELECT_RAPDU[PTX_DEMO_APP_SELECT_RAPDU_LEN]           = { 0x6F, 0x90, 0x00 };

/** @} */

/* Define length of custom nfc id used. */
#define PTX_DEMO_APP_NFC_ID_LEN 4

/* Define length of custom ATS */
#define PTX_DEMO_APP_ISO_DEP_ATS_LEN 5

/* Define length of custom ATTRIB response */
#define PTX_DEMO_APP_ISO_DEP_ATTRIB_RES_LEN 1

/*
 * ####################################################################################################################
 * INTERNAL FUNCTIONS DECLARATIONS / HELPERS
 * ####################################################################################################################
 */

static ptx_Status_t ptx_RunDemoApplication(ptx_RUL_t *rul);
static ptx_Status_t ptx_HandleHCEEvent(ptx_RUL_HCE_t* hce, uint8_t* tx, uint32_t txLen);
static ptx_Status_t ptx_RestartSystem(ptx_RUL_HCE_t* hce);
static ptx_Status_t ptx_HandleHCEData(ptx_RUL_HCE_t* hce, uint8_t* rx, uint32_t rxLen, uint8_t* tx, uint32_t txLen);

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
    ptx_Utils_Print("****   EMV Digital Device PICC Demo %s               ****\n", PTX_DEMO_VERSION);
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

    /* HCE component */
    ptx_RUL_HCE_t hce = {0};
    ptx_RUL_HCE_InitParams_t init_params = {0};

    if (NULL != rul)
    {
        /* Buffers for data */
        uint8_t rx[PTX_DEMO_APP_BUFFER_SIZE];
        uint32_t rx_len = PTX_DEMO_APP_BUFFER_SIZE;
        uint8_t tx[PTX_DEMO_APP_BUFFER_SIZE];
        uint32_t tx_len = PTX_DEMO_APP_BUFFER_SIZE;

        init_params.Rul = rul;
        init_params.AppRxBuffer = rx;
        init_params.AppRxBufferSize = rx_len;
        init_params.Log = &rul->Log;
        
        /* Initialize HCE component with buffers and rul context */
        status = ptx_RUL_HCE_Init(&hce, &init_params);

        if (PTX_STATUS_SUCCESS == status)
        {
            const uint8_t nfc_id[PTX_DEMO_APP_NFC_ID_LEN] = { 0x08, 0x01, 0x02, 0x03 };
            const uint8_t sens_res[PTX_NSC_RF_DISCOVER_SENSRES_LEN] = { 0x01, 0x00 };
            const uint8_t iso_dep_ats[PTX_DEMO_APP_ISO_DEP_ATS_LEN] = {0x05, 0x78, 0x00, 0x71, 0x02};
            const uint8_t iso_dep_attrib_res[PTX_DEMO_APP_ISO_DEP_ATTRIB_RES_LEN] = {0x00};
            const uint8_t prot_info[PTX_NSC_RF_DISCOVER_PROTOCOLINFO_LEN] = {0x00, 0x81, 0x71};
            const uint8_t app_data[PTX_RUL_APPDATA_LEN] = {0x00, 0x00, 0x00, 0x00};

            /* Intialize discovery for EMV Listen Mode*/
            ptx_RUL_DiscoverConfig_t disc_config = {0};

            disc_config.ListenMode   = 3u;
            disc_config.ListenTypeA  = 1u;
            disc_config.ListenTypeB  = 1u;
            disc_config.ListenIsoDep = 3u;

            /* Set custom 4 byte nfc id */
            disc_config.ListenTypeANfcIdLen = PTX_DEMO_APP_NFC_ID_LEN;
            memcpy(disc_config.ListenTypeANfcId, nfc_id, PTX_DEMO_APP_NFC_ID_LEN);
            disc_config.ListenTypeBNfcIdLen = PTX_DEMO_APP_NFC_ID_LEN;
            memcpy(disc_config.ListenTypeBNfcId, nfc_id, PTX_DEMO_APP_NFC_ID_LEN);

            /* Set custom sens response */
            disc_config.ListenTypeASensResLen = PTX_NSC_RF_DISCOVER_SENSRES_LEN;
            memcpy(disc_config.ListenTypeASensRes, sens_res, PTX_NSC_RF_DISCOVER_SENSRES_LEN);

            /* Use custom ATS */
            disc_config.ListenIsoDepAtsLen = PTX_DEMO_APP_ISO_DEP_ATS_LEN;
            memcpy(disc_config.ListenIsoDepAts, iso_dep_ats, PTX_DEMO_APP_ISO_DEP_ATS_LEN);

            /* Use custom attrib response */
            disc_config.ListenIsoDepAttribResLen = PTX_DEMO_APP_ISO_DEP_ATTRIB_RES_LEN;
            memcpy(disc_config.ListenIsoDepAttribRes, iso_dep_attrib_res, PTX_DEMO_APP_ISO_DEP_ATTRIB_RES_LEN);

            /* Use custom AppData */
            disc_config.ListenTypeBAppDataLen = PTX_RUL_APPDATA_LEN;
            memcpy(disc_config.ListenTypeBAppData, app_data, PTX_RUL_APPDATA_LEN);

            /* Use custom Protocol Information */
            disc_config.ListenTypeBProtInfoLen = PTX_NSC_RF_DISCOVER_PROTOCOLINFO_LEN;
            memcpy(disc_config.ListenTypeBProtInfo, prot_info, PTX_NSC_RF_DISCOVER_PROTOCOLINFO_LEN);

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

            do
            {
                /* Check regularly for System-state/-health (e.g. temperature/over-current errors etc.) */
                status_info.Type = ptx_RUL_StatusType_System;
                status = ptx_RUL_GetStatusInfo(rul, &status_info);

                if (PTX_STATUS_SUCCESS == status)
                {
                    if (ptx_RUL_StatusInfoSystem_OK == status_info.Info.System)
                    {
                        /* Query for new HCE events */
                        status = ptx_HandleHCEEvent(&hce, tx, tx_len);
                    }
                    else
                    {
                        if (ptx_RUL_StatusInfoSystem_ErrorOvercurrent == status_info.Info.System)
                        {
                            ptx_Utils_Print("WARNING - Overcurrent Limiter activated - exiting Application!\n");
                            gExit = 1u;
                        }

                        if (ptx_RUL_StatusInfoSystem_ErrorTemperature == status_info.Info.System)
                        {
                            ptx_Utils_Print("WARNING - Temperature Limiter activated - trying to restart!\n");
                            status = ptx_RestartSystem(&hce);

                            if (PTX_STATUS_SUCCESS != status)
                            {
                                ptx_Utils_Print("WARNING - Temperature Limiter activated - cannot restart - exiting Application!\n");
                                gExit = 1u;
                            }
                        }

                        if (ptx_RUL_StatusInfoSystem_ErrorOutOfMemory == status_info.Info.System)
                        {
                            ptx_Utils_Print("WARNING - Out of Memory!\n");
                            gExit = 1u;
                        }

                        if (ptx_RUL_StatusInfoSystem_NSCReset == status_info.Info.System)
                        {
                            ptx_Utils_Print("WARNING - Unexpected System Reset - exiting Application!\n");
                            gExit = 1u;
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

        /* Stop the EMV Loopback Application. */
        (void)ptx_RUL_HCE_DeInit(&hce);

        ptx_Utils_Print("Exiting Demo-Application...\n");
    }

    return status;
}

static ptx_Status_t ptx_RestartSystem(ptx_RUL_HCE_t* hce)
{
    ptx_Status_t status = PTX_STATUS_E_GEN_INVALID_PARAMETER;

    if (NULL != hce)
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
    }

    return status;
}

static ptx_Status_t ptx_HandleHCEEvent(ptx_RUL_HCE_t* hce, uint8_t* tx, uint32_t txLen)
{
    ptx_Status_t status = PTX_STATUS_E_GEN_INVALID_PARAMETER;

    /* Get HCE Event if available */
    ptx_RUL_HCE_EventRecord_t *record;
    ptx_RUL_HCE_EventType_t eventType;
    status = ptx_RUL_HCE_GetEvent(hce, &record);

    if (PTX_STATUS_SUCCESS != status)
    {
        if (PTX_STATUS_E_GEN_NOT_FOUND == status)
        {
            /* There is no HCE event available. Set ID to RUL_HCE_EventType_NoEvent and continue. */
            status = PTX_STATUS_SUCCESS;
            eventType = RUL_HCE_EventType_NoEvent;
        }
        else
        {
            /* Error in HCE_GetEvent */
            ptx_Utils_Print("Error: HCE_GetEvent status = %d\n", status);
        }
    }
    else
    {
        eventType = record->EventID;
    }

    if ((PTX_STATUS_SUCCESS == status) && (RUL_HCE_EventType_NoEvent != eventType))
    {
        switch (eventType)
        {
            case RUL_HCE_EventType_ExtFieldOn:
                status = PTX_STATUS_SUCCESS;
                ptx_Utils_Print("ExFieldOn\n");
                break;
            case RUL_HCE_EventType_ExtFieldOff:
                status = PTX_STATUS_SUCCESS;
                ptx_Utils_Print("ExFieldOff\n");
                break;
            case RUL_HCE_EventType_Activated:
                status = PTX_STATUS_SUCCESS;
                ptx_Utils_Print("Activated\n");
                break;
            case RUL_HCE_EventType_Data:
                ptx_Utils_Print("Received Data\n");
                status = ptx_HandleHCEData(hce, record->RxMsgData, record->RxMsgDataLen, tx, txLen);
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
                break;
            }
            case RUL_HCE_EventType_Error:
            {
                uint8_t error = record->RxMsgDataShortMsgData[0];
                ptx_Utils_Print("Event error: %d\n", error);
                break;
            }
            case RUL_HCE_EventType_NoEvent:
                status = PTX_STATUS_SUCCESS;
                break;
            default:
                status = PTX_STATUS_E_GEN_INVALID_PARAMETER;
                break;
        }
    }

    return status;
}

static ptx_Status_t ptx_HandleHCEData(ptx_RUL_HCE_t* hce, uint8_t* rx, uint32_t rxLen, uint8_t* tx, uint32_t txLen)
{
    ptx_Status_t status = PTX_STATUS_E_GEN_INVALID_PARAMETER;

    /* Minimum length for command */
    if (rxLen >= PTX_DEMO_APP_MIN_APDU_LENGTH)
    {
        /* Get Data byte based on ISO7816 offsets */
        uint8_t ins = rx[PTX_DEMO_APP_ISO7816_OFFSET_INS];

        /* Send response based on instruction byte */
        switch (ins)
        {
            case PTX_DEMO_APP_INS_SELECT:
                memcpy(tx, PTX_DEMO_APP_SELECT_RAPDU, PTX_DEMO_APP_SELECT_RAPDU_LEN);
                status = ptx_RUL_HCE_SendData(hce, tx, PTX_DEMO_APP_SELECT_RAPDU_LEN);
                break;
            case PTX_DEMO_APP_INS_DEACTIVATE_FILE:
                memcpy(tx, PTX_DEMO_APP_INS4_RAPDU, PTX_DEMO_APP_INS4_RAPDU_LEN);
                status = ptx_RUL_HCE_SendData(hce, tx, PTX_DEMO_APP_INS4_RAPDU_LEN);
                break;
            case PTX_DEMO_APP_INS_EMV_GPO:
                memcpy(tx, PTX_DEMO_APP_GPO_RAPDU, PTX_DEMO_APP_GPO_RAPDU_LEN);
                status = ptx_RUL_HCE_SendData(hce, tx, PTX_DEMO_APP_GPO_RAPDU_LEN);
                break;
            case PTX_DEMO_APP_INS_EMV_LOOPBACK:
                if (txLen >= rxLen + PTX_DEMO_APP_RAPDU_STATUS_WORD_LEN)
                {
                    const uint8_t *data = &rx[PTX_DEMO_APP_LOOPBACK_RX_OFFSET];
                    uint32_t length = rx[PTX_DEMO_APP_LOOPBACK_RX_LEN_OFFSET];

                    /* Perform EMV Loopback by returning the received data and prepending a status word. */
                    memcpy(tx, data, length);
                    memcpy(&tx[length], PTX_DEMO_APP_RAPDU_STATUS_WORD, PTX_DEMO_APP_RAPDU_STATUS_WORD_LEN);
                    length += PTX_DEMO_APP_RAPDU_STATUS_WORD_LEN;

                    status = ptx_RUL_HCE_SendData(hce, tx, length);
                }
                break;
            case PTX_DEMO_APP_INS_READ_RECORD:
                memcpy(tx, PTX_DEMO_APP_READ_RECORD_RAPDU, PTX_DEMO_APP_READ_RECORD_RAPDU_LEN);
                status = ptx_RUL_HCE_SendData(hce, tx, PTX_DEMO_APP_READ_RECORD_RAPDU_LEN);
                break;
            default:
                /* Instruction not supported */
                break;
        }
    }

    return status;
}
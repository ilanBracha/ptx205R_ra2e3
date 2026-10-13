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
    Module      : SDK Example (Product Support Library)
    File        : EXAMPLES/ptx_Example_PSL/ptx_ExampleApplication.c

    Description : The Product Support Library (PSL) is designed to assist customers
                  in seamlessly integrating the PTX2xxR/W into the final target
                  application. It provides a toolset of tailored APIs to identify
                  and debug potential performance issues during the design-in phase
                  of the product development, therefore streamlining the integration
                  process and ensuring that any performance bottlenecks are detected
                  at an early stage.
*/

/*
 * ####################################################################################################################
 * INCLUDES
 * ####################################################################################################################
 */
#include "ptx_ExampleApplication.h"
#include "ptx_PSL.h"
#include "ptx_RUL.h"
#include "ptx_Utils_AppParameters.h"
#include "ptx_Utils_VersionInfo.h"
#include "ptx_Utils_Print.h"
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

/*
 * ####################################################################################################################
 * INTERNAL FUNCTIONS DECLARATIONS / HELPERS
 * ####################################################################################################################
 */

static ptx_Status_t ptx_RunDemoApplication(ptx_RUL_t *rul);
static ptx_Status_t ptx_ProcessState_MeasureAntennaQFactor(ptx_PSL_t *psl, ptx_DemoAppState_t *appState);
static ptx_Status_t ptx_ProcessState_MeasureSigDet(ptx_PSL_t *psl, ptx_DemoAppState_t *appState);
static ptx_Status_t ptx_ProcessState_MeasureRSSI(ptx_PSL_t *psl, ptx_DemoAppState_t *appState);
static ptx_Status_t ptx_ProcessState_MeasureDDPC(ptx_PSL_t *psl, ptx_DemoAppState_t *appState);
static ptx_Status_t ptx_ProcessState_SystemError(ptx_PSL_t *psl, ptx_DemoAppState_t *appState, ptx_RUL_Status_Params_t *statusInfoParam, ptx_PSL_InitParams_t *initParams);
static ptx_Status_t ptx_ProcessState_RestartSystem(ptx_PSL_t *psl, ptx_DemoAppState_t *appState, ptx_PSL_InitParams_t *initParams);

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
    ptx_Utils_Print("****           NFC Forum PSL API Demo %s               ****\n", PTX_DEMO_VERSION);
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

    ptx_DemoAppState_t demo_state = DemoAppState_AntennaQMeasurement;
    ptx_RUL_Status_Params_t status_info = {0};

    /* PSL Add-on API component */
    ptx_PSL_t psl = {0};
    ptx_PSL_InitParams_t psl_init_params = {0};

    if (NULL != rul)
    {

        /* Initialize PSL Add-on API component. */
        psl_init_params.Rul = rul;

        status = ptx_PSL_Init(&psl, &psl_init_params);

        if (PTX_STATUS_SUCCESS == status)
        {
            ptx_Utils_Print("PSL API successfully initialized!\n");
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
                        case DemoAppState_AntennaQMeasurement:
                        {
                            status = ptx_ProcessState_MeasureAntennaQFactor(&psl, &demo_state);
                            break;
                        }

                        case DemoAppState_SigDetMeasurment:
                        {
                            status = ptx_ProcessState_MeasureSigDet(&psl, &demo_state);
                            break;
                        }

                        case DemoAppState_RSSIMeasurement:
                        {
                            status = ptx_ProcessState_MeasureRSSI(&psl, &demo_state);
                            break;
                        }

                        case DemoAppState_DDPCMeasurement:
                        {
                            status = ptx_ProcessState_MeasureDDPC(&psl, &demo_state);
                            break;
                        }

                        case DemoAppState_SystemError:
                        {
                            status = ptx_ProcessState_SystemError(&psl, &demo_state, &status_info, &psl_init_params);
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


            /* De-initialize the PSL API Component. */
            status = ptx_PSL_DeInit(&psl);
        }
        else
        {
            ptx_Utils_Print("PSL API initialization failed with Error-Status %04X!\n", status);
        }
        ptx_Utils_Print("Exiting Demo-Application...\n");
    }

    return status;
}

static ptx_Status_t ptx_ProcessState_MeasureAntennaQFactor(ptx_PSL_t *psl, ptx_DemoAppState_t *appState)
{
    ptx_Status_t status = PTX_STATUS_E_GEN_INVALID_PARAMETER;

    if ((NULL != psl) && (NULL != appState))
    {
        /* Allocate the Qmeasurement results datastructure. */
        ptx_PSL_QMeas_Result_t q_meas_result;
        memset(&q_meas_result, 0, sizeof(ptx_PSL_QMeas_Result_t));

        /**
         * The ptx_PSL_QMeasurement() API determines the most critical parameters of the attached antenna system:
         * the Q-factor, resonance frequency, and the lower and upper 3dB corner frequencies. The system will
         * autonomously measure and characterize the antenna without requiring any external measurement equipment.
         * Therefore, the PTX2xxR/W will emit an RF field at various frequency steps within a defined specified span.
         * At each measurement point, the evoked amplitude response from the attached antenna system will be measured,
         * gathering the necessary data to calculate the antenna system's performance characteristics.
        */
        status = ptx_PSL_QMeasurement(psl, NULL, &q_meas_result);

        ptx_Utils_Print("ptx_PSL_QMeasurement() status: %d\n", status);
        ptx_Utils_Print("Measured Q: %ld\n", (uint32_t) (q_meas_result.QFactor / 100));
        ptx_Utils_Print("Resonance frequency [kHz]: %lu\n", (uint32_t) (q_meas_result.ResonanceFreq / 1e3));
        ptx_Utils_Print("Lower 3dB corner frequency [kHz]: %ld\n", (uint32_t) (q_meas_result.LowerCornerFreq / 1e3));
        ptx_Utils_Print("Upper 3dB corner frequency [kHz]: %ld\n", (uint32_t) (q_meas_result.UpperCornerFreq / 1e3));
    }

    return status;
}

static ptx_Status_t ptx_ProcessState_MeasureSigDet(ptx_PSL_t *psl, ptx_DemoAppState_t *appState)
{
    ptx_Status_t status = PTX_STATUS_E_GEN_INVALID_PARAMETER;

    if ((NULL != psl) && (NULL != appState))
    {
        /* Allocate a variable to store the measurement result. */
        uint32_t sig_det_result = 0;

        /* Allocate the SignalDetector input parameter datastructure. */
        ptx_PSL_SigDet_InParams_t sig_det_params;
        memset(&sig_det_params, 0, sizeof(ptx_PSL_SigDet_InParams_t));

        sig_det_params.NfcType = NfcType_A106;
        sig_det_params.TxMultiplier = 0x47;
        sig_det_params.SigmaFactor = 3; /* Sigma decribes the confidence level. Values above 3 don't make much sense here. */
        status = ptx_PSL_SigDetMeasurement(psl, &sig_det_params, &sig_det_result);

        ptx_Utils_Print("ptx_PSL_SigDet_Measurement() status: %d\n", status);
        ptx_Utils_Print("SignalDetector Result: %ld\n", sig_det_result);
    }

    return status;
}

static ptx_Status_t ptx_ProcessState_MeasureRSSI(ptx_PSL_t *psl, ptx_DemoAppState_t *appState)
{
    ptx_Status_t status = PTX_STATUS_E_GEN_INVALID_PARAMETER;

    if ((NULL != psl) && (NULL != appState))
    {
        /* Allocate a variable to store the measurement result. */
        uint32_t rssi_result = 0;

        /* Start value for the powerlevel applied during the RSSI measurement. */
        uint8_t cw_power_level = 0x58;

        status = PTX_STATUS_SUCCESS;
        /* Repeateadly do the RSSI measurement and increase the power level between the measurements which shall yield a higher RSSI value. */
        for (uint8_t i = 0; (i < 5) && (PTX_STATUS_SUCCESS == status); ++i)
        {
            /*
            * The Received Signal Strength Indicator (RSSI) signifies the signal strength picked up by the receiver of the PTX2xxR/W.
            * It can be used to estimate the amplitude of the RF field currently available on the RF pins. The value is derived from
            * the differential configuration of the receive chain and depends on utilizing both the RXp- and RXn-Pins. It can be used
            * during the design-in phase to better understand the impact of an object/tag/coil entering the RF field of the PTX2xxR/W.
            */
            status = ptx_PSL_RssiMeasurement(psl, cw_power_level, AdcAveraging_01Samples, &rssi_result);

            ptx_Utils_Print("ptx_PSL_RssiMeasurement() status: %d\n", status);
            ptx_Utils_Print("RSSI Result: %ld\n", rssi_result);
            /* Increase of the power level. */
            cw_power_level += 0x10;
        }
    }

    return status;
}

static ptx_Status_t ptx_ProcessState_MeasureDDPC(ptx_PSL_t *psl, ptx_DemoAppState_t *appState)
{
    ptx_Status_t status = PTX_STATUS_E_GEN_INVALID_PARAMETER;

    if ((NULL != psl) && (NULL != appState))
    {
        ptx_PSL_Ddpc_Measurement_Params_t ddpc_meas_params;
        ddpc_meas_params.PowerLevel0    = 0x58; /* In this Demo we set half of the maximum power level available for PL0. */
        ddpc_meas_params.PowerLevel1    = 0xB1; /* In this Demo we set the maximum power level available for PL1. */
        ddpc_meas_params.AvgFactor      = AdcAveraging_16Samples;

        ptx_PSL_Ddpc_Results_t result_unloaded;
        memset(&result_unloaded, 0, sizeof(ptx_PSL_Ddpc_Results_t));

        /*
        * The DDPC evaluation API performs two RSSI measurements at the two power levels specified by the user (High Power & Low Power).
        * The API shall be executed twice, once in an unloaded field (i.e., without a card in the immediate vicinity) and the second time
        * with the card in the field at the exact position where the switching process is to take place. The two measurements result in
        * four measured RSSI values for the user, which can be used as a basis for setting suitable thresholds for the DDPC
        * via SetDdpcParameters().
        *
        *
        * Measure the RSSI levels with both power levels.
        * Remove every metal object from the vicinity of the RF field!
        */
        status = ptx_PSL_RunDdpcEvaluation(psl, &ddpc_meas_params, &result_unloaded);

        ptx_Utils_Print("ptx_PSL_RunDdpcEvaluation() status: %d\n", status);
        ptx_Utils_Print("Unloaded PL0 Threshold: %ld\n", result_unloaded.RssiPowerLevel0);
        ptx_Utils_Print("Unloaded PL1 Threshold: %ld\n", result_unloaded.RssiPowerLevel1);

        /* Allocate the DDPC results datastructure. */
        ptx_PSL_Ddpc_Results_t result_loaded;
        memset(&result_loaded, 0, sizeof(ptx_PSL_Ddpc_Results_t));
        /*
        * Measure the RSSI levels with both power levels.
        * This time, put the RFID Card at the desire position of PL switching.
        */
        if (PTX_STATUS_SUCCESS == status)
        {
            status = ptx_PSL_RunDdpcEvaluation(psl, &ddpc_meas_params, &result_loaded);
            ptx_Utils_Print("ptx_PSL_RunDdpcEvaluation() status: %d\n", status);
            ptx_Utils_Print("Loaded PL0_Threshold: %ld\n", result_loaded.RssiPowerLevel0);
            ptx_Utils_Print("Loaded PL1_Threshold: %ld\n", result_loaded.RssiPowerLevel1);
        }

        if (PTX_STATUS_SUCCESS == status)
        {
            ptx_PSL_Ddpc_Params_t ddpc;
            ddpc.PowerLevel0    = 0x58;
            ddpc.PowerLevel1    = 0xB1;
            ddpc.AvgFactor      = AdcAveraging_16Samples;
            /*
             * The intended way to set the thresholds is to choose, for each power level, a threshold that lies
             * between the unloaded and loaded RSSI values. A practical starting point is to simply use the midpoint
             * between the two measurements.
             */
            uint32_t sum0 = (uint32_t)(result_unloaded.RssiPowerLevel0 + (uint32_t)result_loaded.RssiPowerLevel0);
            uint32_t sum1 = (uint32_t)(result_unloaded.RssiPowerLevel1 + (uint32_t)result_loaded.RssiPowerLevel1);

            ddpc.LowThreshold  = (uint8_t)((sum0 + 1000u) / 2000u);
            ddpc.HighThreshold = (uint8_t)((sum1 + 1000u) / 2000u);

            /*
            * The ptx_PSL_RunDdpcEvaluation()-API is utilized to temporarily configure the parameters for the DDPC, including the
            * power levels to be used, the thresholds used by the system to determine when to switch to another power level, and
            * the averaging factor for the internal ADC. Once an acceptable configuration has been found by empirical evaluation,
            * it can be added to the RF configuration of the PTX2xxR/W so that it will be applied upon each system bootup.
            *
            *
            * Set the new DDPC configuration.
            */
            status = ptx_PSL_SetDdpcParameters(psl, &ddpc);
            ptx_Utils_Print("ptx_PSL_SetDdpcParameters() status: %d\n", status);
        }

        if (PTX_STATUS_SUCCESS == status)
        {
            /* Run the DDPC measurement with an interval of 100ms. */
            status = ptx_PSL_RunDdpcTest(psl, 100);
            ptx_Utils_Print("ptx_PSL_RunDdpcTest() status: %d\n", status);
        }

        if (PTX_STATUS_SUCCESS == status)
        {
            ptx_PSL_Ddpc_Status_t ddpc_status;
            memset(&ddpc_status, 0, sizeof(ptx_PSL_Ddpc_Status_t));

            /* Retrieve the DDPC status 200 times with 100ms inbetween (~20 seconds runtime). */
            for (uint8_t i = 0; i < 200; ++i)
            {
                status = ptx_PSL_GetDdpcStatus(psl, &ddpc_status);
                ptx_Utils_Print("Current Power Level: %d\n", ddpc_status.PowerLevel);
                ptx_Utils_Print("Current RSSI: %ld\n", ddpc_status.Rssi);
                /* Sleep for 100ms*/
                (void) ptx_RUL_Sleep(psl->Rul, 100);
            }
        }

        if (PTX_STATUS_SUCCESS == status)
        {
            /* Stop the DDPC measurement. */
            status = ptx_PSL_StopDdpcTest(psl);
            ptx_Utils_Print("ptx_PSL_RunDdpcTest() status: %d\n", status);
        }
    }

    return status;
}

static ptx_Status_t ptx_ProcessState_SystemError(ptx_PSL_t *psl, ptx_DemoAppState_t *appState, ptx_RUL_Status_Params_t *statusInfoParam, ptx_PSL_InitParams_t *initParams)
{
    ptx_Status_t status = PTX_STATUS_E_GEN_INVALID_PARAMETER;

    if ((NULL != psl) && (NULL != appState) && (NULL != statusInfoParam) && (NULL != initParams))
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
            status = ptx_ProcessState_RestartSystem(psl, appState, initParams);
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

static ptx_Status_t ptx_ProcessState_RestartSystem(ptx_PSL_t *psl, ptx_DemoAppState_t *appState, ptx_PSL_InitParams_t *initParams)
{
    ptx_Status_t status = PTX_STATUS_E_GEN_INVALID_PARAMETER;

    if ((NULL != psl) && (NULL != appState) && (NULL != initParams))
    {
        status = ptx_PSL_DeInit(psl);

        if (PTX_STATUS_SUCCESS == status)
        {
            do
            {
                /* Wait for 1s to give the chip time to cool down, after that time try to initialize the system again */
                status = ptx_RUL_Sleep(initParams->Rul, 1000u);

                if (PTX_STATUS_SUCCESS == status)
                {
                    status = ptx_RUL_InitHardware(initParams->Rul);
                }

            }
            while (status != PTX_STATUS_SUCCESS);

            if (PTX_STATUS_SUCCESS == status)
            {
                status = ptx_PSL_Init(psl, initParams);
            }
        }

        if (PTX_STATUS_SUCCESS == status)
        {
            *appState = DemoAppState_AntennaQMeasurement;
        }
    }

    return status;
}
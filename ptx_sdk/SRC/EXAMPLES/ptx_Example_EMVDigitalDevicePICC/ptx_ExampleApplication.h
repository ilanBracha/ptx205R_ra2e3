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
    File        : EXAMPLES/ptx_Example_EMVDigitalDevicePICC/ptx_ExampleApplication.h

    Description : This application demonstrates the use of the EMV Digital Device PICC
                  application using the HCE add-on API. The application is used for
                  compliance / type-approval testing.
*/

/**
 * \addtogroup SDK_EXAMPLES
 *
 * @{
 */

#if !defined(_PTX_SDK_EXAMPLE_EMV_DIGITAL_DEVICE_PICC_H_)
#define _PTX_SDK_EXAMPLE_EMV_DIGITAL_DEVICE_PICC_H_

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * ####################################################################################################################
 * INCLUDES
 * ####################################################################################################################
 */

/*
 * ####################################################################################################################
 * DEFINES / CONSTANTS
 * ####################################################################################################################
 */

/**
 * \brief Version number of current Demo-Application.
 */
const char *PTX_DEMO_VERSION = "V1.0";

#define PTX_DEMO_APP_BUFFER_SIZE          4096
#define PTX_DEMO_APP_LOOPBACK_RX_LEN_OFFSET  4
#define PTX_DEMO_APP_LOOPBACK_RX_OFFSET      5
#define PTX_DEMO_APP_MIN_APDU_LENGTH         5

/**
 * \name ISO-7816 Standard Offsets
 * @{
 */
#define PTX_DEMO_APP_ISO7816_OFFSET_CLA   0x00
#define PTX_DEMO_APP_ISO7816_OFFSET_INS   0x01
#define PTX_DEMO_APP_ISO7816_OFFSET_P1    0x02
/** @} */

/**
 * \name ISO-7816 Standard Instruction Bytes
 * @{
 */
#define PTX_DEMO_APP_INS_SELECT           0xA4
#define PTX_DEMO_APP_INS_READ_RECORD      0xB2
#define PTX_DEMO_APP_INS_DEACTIVATE_FILE  0x04
/** @} */

/**
 * \name EMV Proprietary Instruction Bytes
 * @{
 */
#define PTX_DEMO_APP_INS_EMV_LOOPBACK     0xEE
#define PTX_DEMO_APP_INS_EMV_GPO          0xA8
/** @} */

/*
 * ####################################################################################################################
 * TYPES
 * ####################################################################################################################
 */

/*
 * ####################################################################################################################
 * API FUNCTIONS
 * ####################################################################################################################
 */

#if defined(PTX_PLATFORM_MCU)
int ptx_App_Entry(void);
#endif

#if defined(__cplusplus)
}
#endif

#endif /* Guard */

/** @} */


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
    Module      : SDK Example (NFC Forum NDEF API Add-On API)
    File        : EXAMPLES/ptx_Example_NFCForumNDEF/ptx_ExampleApplication.c

    Description : This application example demonstrates how to
                  - setup and start the basic NFC Forum RF-Discovery loop,
                  - retrieve Card/Tag parameters from the Card-registry,
                  - check and read NDEF-messages from the various NFC Forum Type Tags 2 - 5
                  using the NDEF Add-on API. Typical use cases for the usage of NDEF operations are all
                  types NFC Forum applications such as NFC Forum Wireless Charging (WLC), sending Text / URLs,
                  Bluetooth / WiFi pairing and many more.
                  This example shows exclusively how to read NDEF messages from an NFC Forum Type Tag x.
                  The NDEF API also supports writing of NDEF messages and creation of various NDEF records types.
                  How to use both features is shown in the NFC WLC Poller application.

                  The NDEF add-on API supports two types of NDEF operations:
                    1. "General Tag"-based NDEF operations and
                       Abstracts all NDEF operations for every NFC Forum Type Tag and is agnostic to the used NFC
                       RF-technology and RF-protocol. The API automatically detects which NFC Forum Type Tag is
                       currently active and invokes the proper methods from the "Type Tag"-based NDEF operations (see below).
                       The "General Tag"-based NDEF API is targeted for applications where memory is less of a concern and
                       where the application wants to support all NFC Forum Type Tags without having to deal with which
                       Tag and RF-technology/-protocol is currently being used.
                    2. "Type Tag"-based NDEF operations.
                       These operations work directly on a given NFC Forum Type Tag 2/3/4/5 and can be used to
                       exactly tailor an NDEF application using only the NDEF API needed for the intended scenario.
                       For example, an application knows that it only uses NFC Forum Type Tag 2, then the application
                       only needs to include the T2T NDEF API, but can exclude all others including the "General Tag"-based
                       NDEF API to safe memory.
                       The "Type Tag"-based NDEF API provides the same set of functions like the "General Tag"-based API, but
                       is intended for applications where memory is a concern and high level of customization is required with
                       respect to support for Type Tag 2/3/4/5.
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
#include "ptx_NDEF.h"
#include "ptx_NDEF_T2T.h"
#include "ptx_NDEF_T3T.h"
#include "ptx_NDEF_T4T.h"
#include "ptx_NDEF_T5T.h"
#include "ptx_RUL_HCE.h"
#include "ptx_HCET4T.h"
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

#if defined(PTX_PLATFORM_MCU)
    #include "hal_data.h"
    #if defined(BOARD_RA2E3_FPB)
        /* Defined in ra/board/ra2e3_fpb/board_leds.c (FSP does not export it in a header). */
        extern const bsp_leds_t g_bsp_leds;
    #endif
#endif

/*
 * Board LED status indication (FPB-RA2E3: LED1 = P213, LED2 = P914, via FSP g_bsp_leds).
 *   LED1 ON  -> PTX205R answered over SPI and was initialized (ptx_RUL_InitHardware OK).
 *   LED1 BLINK (fast, forever) -> PTX HW init failed (no/invalid SPI answer).
 *   LED2 ON  -> card/tag activated; OFF when the RF field is switched off again.
 */
/* Card-parameter printing costs ~800 bytes of flash (64 KB RA2E3): disabled to make room for NDEF write support.
 * Define PTX_DEMO_PRINT_CARD_INFO to get it back (only if the image still fits). */
#if !defined(PTX_DEMO_PRINT_CARD_INFO)
    #define PTX_DEMO_PRINT_CARD_PARAMS(rul, id)     ((void)(rul), (void)(id), PTX_STATUS_SUCCESS)
#else
    #define PTX_DEMO_PRINT_CARD_PARAMS(rul, id)     ptx_Utils_CardInfo_Print_Card_Parameters((rul), (id))
#endif

#define PTX_DEMO_LED_PTX_READY      (0u)
#define PTX_DEMO_LED_CARD           (1u)

static void ptx_Demo_LedSet(uint32_t led, uint8_t on)
{
#if defined(PTX_PLATFORM_MCU) && defined(BOARD_RA2E3_FPB)
    if (led < (uint32_t) g_bsp_leds.led_count)
    {
        bsp_io_level_t level = BSP_IO_LEVEL_LOW;
        if (0u != on)
        {
            level = BSP_IO_LEVEL_HIGH;
        }
        R_BSP_PinAccessEnable();
        R_BSP_PinWrite((bsp_io_port_pin_t) g_bsp_leds.p_leds[led], level);
        R_BSP_PinAccessDisable();
    }
#else
    (void)led;
    (void)on;
#endif
}

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
#define PTX_DEMO_APP_EXAMPLE_WORK_BUFFER_SIZE       256u

/* Use "General Tag"-based NDEF API. Comment line to use "Type Tag"-based API */
#define PTX_DEMO_APP_USE_GENERAL_TAG_NDEF_API

/* Demo Example internal Exchange parameters. */
typedef struct ptx_DemoApplication_ExchangeParams
{
    uint8_t                     RxBuffer[PTX_DEMO_APP_EXAMPLE_WORK_BUFFER_SIZE];
    uint32_t                    RxBufferSize;
    uint8_t                     TxBuffer[PTX_DEMO_APP_EXAMPLE_WORK_BUFFER_SIZE];
    uint32_t                    TxBufferSize;
    uint8_t                     WorkBuffer[PTX_DEMO_APP_EXAMPLE_WORK_BUFFER_SIZE];
    uint32_t                    WorkBufferSize;
    uint8_t                     NdefMsgBuffer[PTX_DEMO_APP_EXAMPLE_WORK_BUFFER_SIZE];
    uint32_t                    NdefMsgBufferSize;
    ptx_RUL_CardRegistry_t      *CardRegistry;

} ptx_DemoApplication_ExchangeParams_t;

/*
 * Simple UART CLI (115200 8N1 on the J-Link VCOM):
 *   write <text>  - arm <text>; it is written as NDEF Text record ("en") to the next tag presented
 *   cancel        - drop a pending write
 *   status        - show pending write
 *   help          - list commands
 * After a successful write the tag is read back and printed; the pending text is then cleared (one-shot).
 */
#define PTX_DEMO_CLI_LINE_SIZE          (128u)                              /* Incl. terminating 0 */
#define PTX_DEMO_NDEF_TEXT_LANG         "en"
#define PTX_DEMO_NDEF_TEXT_HDR_LEN      (7u)                                /* 4 byte short-record header + status + "en" */
#define PTX_DEMO_NDEF_TEXT_MAX          (PTX_DEMO_CLI_LINE_SIZE - 7u)       /* "write " prefix removed */

static char     gPendingText[PTX_DEMO_NDEF_TEXT_MAX + 1u];
static uint32_t gPendingTextLen = 0;

/* TAG mode (card emulation): text to present, request flag set by the "tag" command. */
static char     gTagText[PTX_DEMO_NDEF_TEXT_MAX + 1u];
static uint32_t gTagTextLen = 0;
static uint8_t  gTagRequested = 0;
static uint8_t  gTagIsBt = 0;                     /* 1: TAG mode presents a Bluetooth pairing record instead of text */
static uint8_t  gTagBtAddr[6];                    /* Bluetooth device address, as typed (MSB first) */

#define PTX_DEMO_BT_NAME_MAX            (30u)     /* Max. Bluetooth device name in the pairing record */

/* Parses "AA:BB:CC:DD:EE:FF" (':' or '-' separators) into addr[0..5] (MSB first). Returns 1 on success. */
static uint8_t ptx_Demo_ParseBtAddr(const char *str, uint8_t *addr)
{
    uint8_t ok = 1u;

    for (uint32_t i = 0; (i < 6u) && (0u != ok); i++)
    {
        uint8_t val = 0;
        for (uint32_t k = 0; k < 2u; k++)
        {
            char c = str[(i * 3u) + k];
            uint8_t nib;
            if ((c >= '0') && (c <= '9'))      { nib = (uint8_t)(c - '0'); }
            else if ((c >= 'a') && (c <= 'f')) { nib = (uint8_t)(c - 'a' + 10); }
            else if ((c >= 'A') && (c <= 'F')) { nib = (uint8_t)(c - 'A' + 10); }
            else                               { nib = 0; ok = 0u; }
            val = (uint8_t)((val << 4) | nib);
        }
        addr[i] = val;
        if ((i < 5u) && (':' != str[(i * 3u) + 2u]) && ('-' != str[(i * 3u) + 2u]))
        {
            ok = 0u;
        }
    }

    return ok;
}

#define PTX_DEMO_CLI_PROMPT             "$ nfc > "

static void ptx_Demo_CliPrompt(void)
{
    ptx_Utils_Print(PTX_DEMO_CLI_PROMPT);
}

static void ptx_Demo_CliHelp(void)
{
    ptx_Utils_Print("CLI: write <text> | tag <text> | bt <mac> [name] | cancel | status | help\n");
}

static void ptx_Demo_CliPoll(void)
{
    char line[PTX_DEMO_CLI_LINE_SIZE];

    if (0u != ptx_Utils_Cli_GetLine(line, (uint32_t)sizeof(line)))
    {
        if ('\0' == line[0])
        {
            /* Enter on an empty line: just a new prompt. */
        }
        else if (0 == strncmp(line, "write ", 6))
        {
            const char *text = &line[6];
            uint32_t len = (uint32_t)strlen(text);

            if ((0u != len) && (len <= PTX_DEMO_NDEF_TEXT_MAX))
            {
                (void)memcpy(gPendingText, text, len);
                gPendingText[len] = '\0';
                gPendingTextLen = len;
                ptx_Utils_Print("Armed: \"%s\"\n", gPendingText);
            }
            else
            {
                ptx_Utils_Print("Text must be 1..%u chars\n", (uint32_t)PTX_DEMO_NDEF_TEXT_MAX);
            }
        }
        else if ((0 == strncmp(line, "tag ", 4)) || (0 == strncmp(line, "TAG ", 4)))
        {
            const char *text = &line[4];
            uint32_t len = (uint32_t)strlen(text);

            if ((0u != len) && (len <= PTX_DEMO_NDEF_TEXT_MAX))
            {
                (void)memcpy(gTagText, text, len);
                gTagText[len] = '\0';
                gTagTextLen = len;
                gTagIsBt = 0u;
                gTagRequested = 1u;
            }
            else
            {
                ptx_Utils_Print("Text must be 1..%u chars\n", (uint32_t)PTX_DEMO_NDEF_TEXT_MAX);
            }
        }
        else if ((0 == strncmp(line, "bt ", 3)) || (0 == strncmp(line, "BT ", 3)))
        {
            /* bt AA:BB:CC:DD:EE:FF [name] - TAG mode with a Bluetooth (BR/EDR) pairing record for e.g. an earpiece. */
            const char *name = &line[3 + 17];
            uint32_t len = 0;

            if ((strlen(&line[3]) >= 17u) && (0u != ptx_Demo_ParseBtAddr(&line[3], gTagBtAddr)) &&
                (('\0' == *name) || (' ' == *name)))
            {
                if (' ' == *name)
                {
                    name++;
                }
                len = (uint32_t)strlen(name);
                if (len > PTX_DEMO_BT_NAME_MAX)
                {
                    len = PTX_DEMO_BT_NAME_MAX;
                }
                (void)memcpy(gTagText, name, len);
                gTagText[len] = '\0';
                gTagTextLen = len;
                gTagIsBt = 1u;
                gTagRequested = 1u;
            }
            else
            {
                ptx_Utils_Print("Usage: bt AA:BB:CC:DD:EE:FF [name]\n");
            }
        }
        else if (0 == strcmp(line, "cancel"))
        {
            gPendingTextLen = 0;
            ptx_Utils_Print("Write cancelled\n");
        }
        else if (0 == strcmp(line, "status"))
        {
            if (0u != gPendingTextLen)
            {
                ptx_Utils_Print("Pending write: \"%s\"\n", gPendingText);
            }
            else
            {
                ptx_Utils_Print("No pending write\n");
            }
        }
        else
        {
            ptx_Demo_CliHelp();
        }

        ptx_Demo_CliPrompt();
    }
}

/*
 * Prints every record of an NDEF message: record number, type (1-char types like T/U shown) and payload as text.
 * Text records are shown without status byte / language code. Non-printable bytes are shown as '.'.
 */
static void ptx_Demo_PrintNdef(const uint8_t *msg, uint32_t len)
{
    uint32_t pos = 0;
    uint32_t rec = 0;

    while ((pos + 3u) <= len)
    {
        uint8_t  hdr      = msg[pos++];                         /* MB ME CF SR IL TNF */
        uint32_t type_len = msg[pos++];
        uint32_t pay_len;
        uint32_t id_len   = 0;

        if (0u != (hdr & 0x10u))
        {
            pay_len = msg[pos++];                               /* short record: 1 byte payload length */
        }
        else
        {
            if ((pos + 4u) > len)
            {
                break;
            }
            pay_len = ((uint32_t)msg[pos] << 24) | ((uint32_t)msg[pos + 1u] << 16) | ((uint32_t)msg[pos + 2u] << 8) | (uint32_t)msg[pos + 3u];
            pos += 4u;
        }
        if ((0u != (hdr & 0x08u)) && (pos < len))
        {
            id_len = msg[pos++];
        }

        uint32_t type_pos = pos;
        uint32_t start    = pos + type_len + id_len;
        uint32_t end      = ((start + pay_len) > len) ? len : (start + pay_len);

        if ((0x01u == (hdr & 0x07u)) && (1u == type_len) && ((uint8_t)'T' == msg[type_pos]) && (start < end))
        {
            start += 1u + (uint32_t)(msg[start] & 0x3Fu);       /* Text record: skip status byte + language code */
        }

        ptx_Utils_Print("NDEF[%u] %c: \"", rec, ((1u == type_len) && (type_pos < len)) ? (int)msg[type_pos] : (int)'?');
        for (uint32_t i = start; i < end; i++)
        {
            ptx_Utils_Print("%c", ((msg[i] >= 0x20u) && (msg[i] < 0x7Fu)) ? (int)msg[i] : (int)'.');
        }
        ptx_Utils_Print("\"\n");

        rec++;
        pos = end;
        if (0u != (hdr & 0x40u))
        {
            break;                                              /* ME: last record */
        }
    }

    if (0u == rec)
    {
        ptx_Utils_Print("NDEF: %u bytes (not parsed)\n", len);
    }
}

/* Builds a single NDEF Text record (NFC Forum RTD Text, UTF-8, language "en") into buffer. Returns message length. */
static uint32_t ptx_Demo_BuildTextNdef(uint8_t *buffer, uint32_t bufferSize, const char *text, uint32_t textLen)
{
    uint32_t msg_len = 0;
    const uint32_t payload_len = 1u + 2u + textLen;                     /* status byte + "en" + text */

    if ((NULL != buffer) && ((PTX_DEMO_NDEF_TEXT_HDR_LEN + textLen) <= bufferSize) && (payload_len <= 255u))
    {
        buffer[0] = 0xD1u;                                              /* MB=1, ME=1, SR=1, TNF=0x01 (Well-known) */
        buffer[1] = 0x01u;                                              /* Type length */
        buffer[2] = (uint8_t)payload_len;                               /* Payload length (short record) */
        buffer[3] = (uint8_t)'T';                                       /* Type "T" (Text) */
        buffer[4] = 0x02u;                                              /* Status: UTF-8, language code length 2 */
        buffer[5] = (uint8_t)PTX_DEMO_NDEF_TEXT_LANG[0];
        buffer[6] = (uint8_t)PTX_DEMO_NDEF_TEXT_LANG[1];
        (void)memcpy(&buffer[PTX_DEMO_NDEF_TEXT_HDR_LEN], text, textLen);
        msg_len = PTX_DEMO_NDEF_TEXT_HDR_LEN + textLen;
    }

    return msg_len;
}

/*
 * Builds an NFC Forum Bluetooth static-handover record (BR/EDR, MIME "application/vnd.bluetooth.ep.oob"):
 * payload = OOB length (2 byte LE) + BD_ADDR (6 byte, LSB first) + EIR: Class of Device + Complete Local Name.
 * Android reads it, pairs with the device and connects the audio profiles (headset / earpiece).
 */
#define PTX_DEMO_BT_MIME_TYPE           "application/vnd.bluetooth.ep.oob"
#define PTX_DEMO_BT_MIME_TYPE_LEN       (32u)
#define PTX_DEMO_BT_EIR_CLASS_OF_DEVICE (0x0Du)
#define PTX_DEMO_BT_EIR_COMPLETE_NAME   (0x09u)

static uint32_t ptx_Demo_BuildBtNdef(uint8_t *buffer, uint32_t bufferSize, const uint8_t *addr, const char *name, uint32_t nameLen)
{
    /* Class of Device 0x240404: Audio + Rendering service, major Audio/Video, minor Wearable Headset (LSB first). */
    static const uint8_t cod[3] = { 0x04, 0x04, 0x24 };
    uint32_t payload_len = 2u + 6u + 5u + ((0u != nameLen) ? (2u + nameLen) : 0u);
    uint32_t msg_len = 0;

    if ((NULL != buffer) && ((3u + PTX_DEMO_BT_MIME_TYPE_LEN + payload_len) <= bufferSize) && (payload_len <= 255u))
    {
        uint32_t i = 0;
        buffer[i++] = 0xD2u;                                        /* MB=1, ME=1, SR=1, TNF=0x02 (MIME media type) */
        buffer[i++] = (uint8_t)PTX_DEMO_BT_MIME_TYPE_LEN;
        buffer[i++] = (uint8_t)payload_len;
        (void)memcpy(&buffer[i], PTX_DEMO_BT_MIME_TYPE, PTX_DEMO_BT_MIME_TYPE_LEN);
        i += PTX_DEMO_BT_MIME_TYPE_LEN;

        buffer[i++] = (uint8_t)(payload_len & 0xFFu);               /* OOB data length, little endian */
        buffer[i++] = (uint8_t)((payload_len >> 8) & 0xFFu);
        for (uint32_t k = 0; k < 6u; k++)
        {
            buffer[i++] = addr[5u - k];                             /* BD_ADDR, LSB first */
        }
        buffer[i++] = 4u;                                           /* EIR: Class of Device */
        buffer[i++] = PTX_DEMO_BT_EIR_CLASS_OF_DEVICE;
        buffer[i++] = cod[0];
        buffer[i++] = cod[1];
        buffer[i++] = cod[2];
        if (0u != nameLen)
        {
            buffer[i++] = (uint8_t)(nameLen + 1u);                  /* EIR: Complete Local Name */
            buffer[i++] = PTX_DEMO_BT_EIR_COMPLETE_NAME;
            (void)memcpy(&buffer[i], name, nameLen);
            i += nameLen;
        }
        msg_len = i;
    }

    return msg_len;
}

/*
 * ####################################################################################################################
 * TAG MODE - card emulation of an NFC Forum Type 4 Tag (ISO-DEP, NFC-A) holding one NDEF Text record.
 * Uses the SDK Host Card Emulation component (RUL_HCE) and the T4T emulation helper (HCE_T4T example module).
 * ####################################################################################################################
 */
#define PTX_DEMO_TAG_NDEF_FILE_SIZE     (256u)      /* = max. NDEF file size announced in the CC file (0x0100) */
#define PTX_DEMO_TAG_BUFFER_SIZE        (256u)      /* HCE Rx (C-APDU) / Tx (R-APDU) buffers, MLe/MLc = 128 */
#define PTX_DEMO_TAG_NFCID_LEN          (4u)
#define PTX_DEMO_TAG_ATS_LEN            (5u)

static ptx_RUL_HCE_t gTagHce;
static ptx_HCET4T_t  gTagT4t;
static uint8_t       gTagRxBuffer[PTX_DEMO_TAG_BUFFER_SIZE];
static uint8_t       gTagTxBuffer[PTX_DEMO_TAG_BUFFER_SIZE];
static uint8_t       gTagNdefFile[PTX_DEMO_TAG_NDEF_FILE_SIZE];

/* Capability Container: CCLEN=15, mapping v2.0, MLe=0x0080, MLc=0x0080,
 * NDEF File Control TLV: file ID E104, max. NDEF size 0x0100, read access 00 (free), write access FF (read-only). */
static uint8_t       gTagCcFile[15] = { 0x00, 0x0F, 0x20, 0x00, 0x80, 0x00, 0x80, 0x04, 0x06, 0xE1, 0x04, 0x01, 0x00, 0x00, 0xFF };

static ptx_Status_t ptx_Demo_RunTagMode(ptx_RUL_t *rul)
{
    static const uint8_t nfc_id[PTX_DEMO_TAG_NFCID_LEN]          = { 0x08, 0x01, 0x02, 0x03 };     /* 0x08 = random-ID marker */
    static const uint8_t sens_res[PTX_RUL_SENSRES_LEN] = { 0x01, 0x00 };
    static const uint8_t iso_dep_ats[PTX_DEMO_TAG_ATS_LEN]        = { 0x05, 0x78, 0x00, 0x71, 0x02 };

    ptx_Status_t status;
    ptx_RUL_DeactivateParams_t deact_params = {0};
    ptx_RUL_HCE_InitParams_t hce_init = {0};
    ptx_HCET4T_InitParams_t t4t_init = {0};
    ptx_RUL_DiscoverConfig_t disc_config;
    ptx_RUL_HCE_EventRecord_t *record = NULL;

    gTagRequested = 0u;

    /* NDEF file = 2 byte NLEN (big endian) + NDEF message (one Text or one Bluetooth pairing record). */
    (void)memset(gTagNdefFile, 0, sizeof(gTagNdefFile));
    uint32_t msg_len = (0u != gTagIsBt) ?
        ptx_Demo_BuildBtNdef(&gTagNdefFile[2], (uint32_t)sizeof(gTagNdefFile) - 2u, gTagBtAddr, gTagText, gTagTextLen) :
        ptx_Demo_BuildTextNdef(&gTagNdefFile[2], (uint32_t)sizeof(gTagNdefFile) - 2u, gTagText, gTagTextLen);
    gTagNdefFile[0] = (uint8_t)((msg_len >> 8) & 0xFFu);
    gTagNdefFile[1] = (uint8_t)(msg_len & 0xFFu);

    /* Stop the reader (polling loop) first. */
    deact_params.Type  = ptx_RUL_DeactivateType_Generic;
    deact_params.State = ptx_RUL_DeactivateState_Idle;
    status = ptx_RUL_Deactivate(rul, &deact_params);

    if (PTX_STATUS_SUCCESS == status)
    {
        hce_init.AppRxBuffer     = gTagRxBuffer;
        hce_init.AppRxBufferSize = (uint32_t)sizeof(gTagRxBuffer);
        hce_init.Rul             = rul;
        hce_init.Log             = &rul->Log;
        status = ptx_RUL_HCE_Init(&gTagHce, &hce_init);
    }

    if (PTX_STATUS_SUCCESS == status)
    {
        t4t_init.CCFile       = gTagCcFile;
        t4t_init.CCFileSize   = (uint32_t)sizeof(gTagCcFile);
        t4t_init.NDEFFile     = gTagNdefFile;
        t4t_init.NDEFFileSize = (uint32_t)sizeof(gTagNdefFile);
        status = ptx_HCET4T_Init(&gTagT4t, &t4t_init);
    }

    if (PTX_STATUS_SUCCESS == status)
    {
        /* Listen-only discovery: NFC-A + ISO-DEP (Type 4 Tag), RF-field notifications enabled. */
        (void)memset(&disc_config, 0, sizeof(disc_config));
        disc_config.ListenMode            = 3u;
        disc_config.ListenTypeA           = 1u;
        disc_config.ListenIsoDep          = 1u;
        disc_config.ListenTypeANfcIdLen   = (uint8_t)PTX_DEMO_TAG_NFCID_LEN;
        (void)memcpy(disc_config.ListenTypeANfcId, nfc_id, PTX_DEMO_TAG_NFCID_LEN);
        disc_config.ListenTypeASensResLen = (uint8_t)PTX_RUL_SENSRES_LEN;
        (void)memcpy(disc_config.ListenTypeASensRes, sens_res, PTX_RUL_SENSRES_LEN);
        disc_config.ListenIsoDepAtsLen    = (uint8_t)PTX_DEMO_TAG_ATS_LEN;
        (void)memcpy(disc_config.ListenIsoDepAts, iso_dep_ats, PTX_DEMO_TAG_ATS_LEN);
        status = ptx_RUL_InitiateDiscovery(rul, &disc_config);
    }

    if (PTX_STATUS_SUCCESS == status)
    {
        if (0u != gTagIsBt)
        {
            ptx_Utils_Print("TAG BT %02X:%02X:%02X:%02X:%02X:%02X \"%s\" (any key: exit)\n", gTagBtAddr[0], gTagBtAddr[1],
                            gTagBtAddr[2], gTagBtAddr[3], gTagBtAddr[4], gTagBtAddr[5], gTagText);
        }
        else
        {
            ptx_Utils_Print("TAG \"%s\" (any key: exit)\n", gTagText);
        }
        ptx_Demo_LedSet(PTX_DEMO_LED_CARD, 1u);
        (void)ptx_Utils_Cli_AnyKey();                       /* drop keys typed before TAG mode started */
    }
    else
    {
        ptx_Utils_Print("TAG start failed: %04X\n", status);
    }

    while ((PTX_STATUS_SUCCESS == status) && (0u == ptx_Utils_Cli_AnyKey()))
    {
        status = ptx_RUL_HCE_GetEvent(&gTagHce, &record);

        if ((PTX_STATUS_SUCCESS == status) && (NULL != record))
        {
            switch (record->EventID)
            {
                case RUL_HCE_EventType_ExtFieldOn:
                case RUL_HCE_EventType_ExtFieldOff:
                    status = ptx_HCET4T_Reset(&gTagT4t);
                    break;

                case RUL_HCE_EventType_Activated:
                    ptx_Utils_Print("TAG: read start\n");
                    break;

                case RUL_HCE_EventType_Data:
                {
                    uint32_t tx_len = (uint32_t)sizeof(gTagTxBuffer);
                    status = ptx_HCET4T_CommandProcess(&gTagT4t, record->RxMsgData, record->RxMsgDataLen, gTagTxBuffer, &tx_len);
                    if ((PTX_STATUS_SUCCESS == status) && (0u != tx_len))
                    {
                        status = ptx_RUL_HCE_SendData(&gTagHce, gTagTxBuffer, tx_len);
                    }
                    break;
                }

                case RUL_HCE_EventType_Deactivated:
                    ptx_Utils_Print("TAG: read end\n");
                    break;

                case RUL_HCE_EventType_Error:
                    /* RF error (e.g. reader moved away) - keep emulating. */
                    break;

                case RUL_HCE_EventType_NoEvent:
                default:
                    break;
            }
        }

        if (PTX_STATUS_E_GEN_NOT_FOUND == status)
        {
            status = PTX_STATUS_SUCCESS;                    /* no event pending */
        }
    }

    if (PTX_STATUS_SUCCESS != status)
    {
        ptx_Utils_Print("TAG error: %04X\n", status);
    }

    /* Back to reader mode: stop listening, release HCE / T4T emulation. */
    (void)ptx_RUL_Deactivate(rul, &deact_params);
    (void)ptx_HCET4T_DeInit(&gTagT4t);
    (void)ptx_RUL_HCE_DeInit(&gTagHce);
    ptx_Demo_LedSet(PTX_DEMO_LED_CARD, 0u);
    ptx_Utils_Print("Reader mode\n");

    return status;
}

/*
 * ####################################################################################################################
 * INTERNAL FUNCTIONS DECLARATIONS / HELPERS
 * ####################################################################################################################
 */
static ptx_Status_t ptx_RunDemoApplication(ptx_RUL_t *rul);
static ptx_Status_t ptx_ProcessState_StartDiscovery(ptx_RUL_t *rul, ptx_DemoAppState_t *appState);
static ptx_Status_t ptx_ProcessState_WaitForDiscovery(ptx_RUL_t *rul, ptx_DemoAppState_t *appState, ptx_RUL_CardRegistry_t *cardRegistry);
static ptx_Status_t ptx_ProcessState_SelectCard(ptx_RUL_t *rul, ptx_DemoAppState_t *appState, ptx_RUL_CardRegistry_t *cardRegistry);
static ptx_Status_t ptx_ProcessState_NDEFDataExchange(ptx_RUL_t *rul, ptx_DemoAppState_t *appState, ptx_RUL_CardRegistry_t *cardRegistry);
static ptx_Status_t ptx_ProcessState_DeactivateReader(ptx_RUL_t *rul, ptx_DemoAppState_t *appState);
static ptx_Status_t ptx_ProcessState_RestartDiscovery(ptx_RUL_t *rul, ptx_DemoAppState_t *appState);
static ptx_Status_t ptx_ProcessState_SystemError(ptx_RUL_t *rul, ptx_DemoAppState_t *appState, ptx_RUL_Status_Params_t *statusInfoParam);
static ptx_Status_t ptx_ProcessState_RestartSystem(ptx_RUL_t *rul, ptx_DemoAppState_t *appState);
#if defined(PTX_DEMO_APP_USE_GENERAL_TAG_NDEF_API)
    static ptx_Status_t ptx_ProcessState_NDEFDataExchange_General(ptx_RUL_t *rul, ptx_DemoApplication_ExchangeParams_t *exchangeParams);
#else
    static ptx_Status_t ptx_ProcessState_NDEFDataExchange_T2T(ptx_RUL_t *rul, ptx_DemoApplication_ExchangeParams_t *exchangeParams);
    static ptx_Status_t ptx_ProcessState_NDEFDataExchange_T3T(ptx_RUL_t *rul, ptx_DemoApplication_ExchangeParams_t *exchangeParams);
    static ptx_Status_t ptx_ProcessState_NDEFDataExchange_T4T(ptx_RUL_t *rul, ptx_DemoApplication_ExchangeParams_t *exchangeParams);
    static ptx_Status_t ptx_ProcessState_NDEFDataExchange_T5T(ptx_RUL_t *rul, ptx_DemoApplication_ExchangeParams_t *exchangeParams);
#endif

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

    ptx_Utils_Print("-----------------------\n");
    ptx_Utils_Print("** NFC Forum NDEF Demo %s **\n", PTX_DEMO_VERSION);
    ptx_Utils_Print("-----------------------\n");

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
        ptx_Utils_Print("RUL Init OK\n");

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
            ptx_Utils_Print("PTX HW init OK\n");
            ptx_Demo_LedSet(PTX_DEMO_LED_PTX_READY, 1u);

            /* RA2E3 (64 KB flash): revision info printout (ptx_Utils_VersionInfo) removed to make room for TAG mode. */
            status = ptx_RunDemoApplication(&rul);

            ptx_Utils_Print("Demo done: %04X\n", status);
        }
        else
        {
            ptx_Utils_Print("PTX HW init failed: %04X\n", status);

#if defined(PTX_PLATFORM_MCU) && defined(BOARD_RA2E3_FPB)
            /* No valid answer from PTX205R: blink LED1 fast forever so the failure is visible without a debugger. */
            /* Plain busy-wait (approx. 100 ms @ 48 MHz) instead of R_BSP_SoftwareDelay() to save flash (64 KB part). */
            uint8_t led_on = 0u;
            for (;;)
            {
                led_on = (uint8_t)(led_on ^ 1u);
                ptx_Demo_LedSet(PTX_DEMO_LED_PTX_READY, led_on);
                for (volatile uint32_t d = 0u; d < 600000u; d++)
                {
                }
            }
#endif
        }
    }
    else
    {
        ptx_Utils_Print("RUL Init failed: %04X\n", status);
    }

    /* Switch RF-Field off (independent of previous state) */
    deact_params.Type = ptx_RUL_DeactivateType_Generic;
    deact_params.State = ptx_RUL_DeactivateState_Idle;
    (void)ptx_RUL_Deactivate(&rul, &deact_params);

    /* RA2E3 (64 KB flash): no ptx_RUL_DeInit() - the demo only ends after a fatal error and the MCU stops anyway. */
    ptx_Utils_Print("Stopped.\n");

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

    if (NULL != rul)
    {
        /* Get and store a pointer to the RUL card registry */
        status = ptx_RUL_GetCardRegistry(rul, &card_registry);

        if (PTX_STATUS_SUCCESS == status)
        {
            ptx_Demo_CliHelp();

            do
            {
                /* Handle user commands from the UART terminal (non-blocking). */
                ptx_Demo_CliPoll();

                /* "tag" command: switch to card emulation while the reader is idle-polling; returns on any key. */
                if ((0u != gTagRequested) && (DemoAppState_WaitForDiscovery == demo_state))
                {
                    (void)ptx_Demo_RunTagMode(rul);
                    demo_state = DemoAppState_StartDiscovery;
                }

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

                        case DemoAppState_NDEFDataExchange:
                        {
                            status = ptx_ProcessState_NDEFDataExchange(rul, &demo_state, card_registry);
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
                            ptx_Utils_Print("Invalid app state\n");
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
        /* Initialize RF-Discovery / Polling Loop in NFC Forum Poll-Mode with standard parameters.*/
        disc_params.PollMode = ptx_RUL_PollMode_NfcForum;   /* Enable PolL-Mode only in this example (Listen-mode can be optionally enabled). */
        disc_params.EnablePolling = 1u;                     /* Enable Polling only (Listen-mode can be optionally enabled). */
        disc_params.IdleTime = 20u;                         /* FPB-RA2E3: 20 ms idle between polling loops (was 100 ms) -> faster card detection. */
        disc_params.PollTypeA = 1u;                         /* Poll for NFC-A Technology. */
        disc_params.PollTypeAPollFrequency = 1u;            /* Poll for NFC-A Technology during every Polling-Loop cycle (otherwise every n-th cycle). */
        disc_params.PollTypeADeviceLimit = 5u;              /* Limit the number of NFC-A devices / cards to max. 5 devices (application can define max. value). */
        disc_params.PollTypeB = 1u;                         /* Poll for NFC-B Technology. */
        disc_params.PollTypeBPollFrequency = 1u;            /* Poll for NFC-B Technology during every Polling-Loop cycle (otherwise every n-th cycle). */
        disc_params.PollTypeBDeviceLimit = 5u;              /* Limit the number of NFC-B devices / cards to max. 5 devices (application can define max. value). */
        disc_params.PollTypeF212 = 0u;                      /* RA2E3 build: T3T (NFC-F) support removed (PTX_DISABLE_NDEF_T3T) - no NFC-F polling. */
        disc_params.PollTypeFPollFrequency = 1u;            /* Poll for NFC-F Technology during every Polling-Loop cycle (otherwise every n-th cycle). */
        disc_params.PollTypeFDeviceLimit = 5u;              /* Limit the number of NFC-F devices / cards to max. 5 devices (application can define max. value). */
        disc_params.PollTypeV = 0u;                         /* RA2E3 build: T5T (NFC-V) support removed (PTX_DISABLE_NDEF_T5T) - no NFC-V polling. */
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
            ptx_Utils_Print("Waiting for cards...\n");
            ptx_Demo_CliPrompt();
        }
        else
        {
            *appState = DemoAppState_Exit;
            ptx_Utils_Print("Discovery start failed: %04X\n", status);
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
                    ptx_Utils_Print("Multiple cards\n");
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
                    ptx_Utils_Print("All cards resolved\n");

                    /* Print all available Card-Infos as a List. */
                    for (uint8_t i = 0; i < cardRegistry->CardsCnt; i++)
                    {
                        (void)PTX_DEMO_PRINT_CARD_PARAMS(rul, i);
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
                    ptx_Utils_Print("Multi-protocol card\n");

                    /* Print all available Card-Infos as a List. */
                    (void)PTX_DEMO_PRINT_CARD_PARAMS(rul, 0);

                    *appState = DemoAppState_SelectCard;
                    break;
                }

                case ptx_RUL_StatusInfoDiscover_Active:
                {
                    /*
                     * An NFC-device has been activated i.e. selected and is ready for (user) data exchange.
                     * Activation / Selection takes directly place if single device / card supporting only one protocol was detected.
                     */
                    ptx_Utils_Print("Card activated\n");
                    ptx_Demo_LedSet(PTX_DEMO_LED_CARD, 1u);

                    /* Print all available Card-Infos as a List. */
                    (void)PTX_DEMO_PRINT_CARD_PARAMS(rul, cardRegistry->ActiveCardId);

                    /* Card / Device is ready for (user) data exchange based on Native Tag API. */
                    *appState = DemoAppState_NDEFDataExchange;
                    break;
                }

                default:
                {
                    *appState = DemoAppState_Exit;
                    ptx_Utils_Print("Invalid app state\n");
                    break;
                }
            }
        }
        else
        {
            *appState = DemoAppState_Exit;
            ptx_Utils_Print("Discovery status failed: %04X\n", status);
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
            ptx_Utils_Print("Card selected\n");
            ptx_Demo_LedSet(PTX_DEMO_LED_CARD, 1u);

            /* Activation / Selection successfully - print Card-infos and move on to exchange example data. */
            (void)PTX_DEMO_PRINT_CARD_PARAMS(rul, 0);

            *appState = DemoAppState_NDEFDataExchange;
        }
        else
        {
            ptx_Utils_Print("Card select failed: %04X\n", status);

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

static ptx_Status_t ptx_ProcessState_NDEFDataExchange(ptx_RUL_t *rul, ptx_DemoAppState_t *appState, ptx_RUL_CardRegistry_t *cardRegistry)
{
    ptx_Status_t status = PTX_STATUS_E_GEN_INVALID_PARAMETER;

    ptx_DemoApplication_ExchangeParams_t exchange_params = {0};

    uint8_t do_ndef_exchange = 1u;

    if ((NULL != rul) && (NULL != appState) && (NULL != cardRegistry))
    {
        exchange_params.CardRegistry      = cardRegistry;
        exchange_params.TxBufferSize      = (uint32_t)PTX_DEMO_APP_EXAMPLE_WORK_BUFFER_SIZE;
        exchange_params.RxBufferSize      = (uint32_t)PTX_DEMO_APP_EXAMPLE_WORK_BUFFER_SIZE;
        exchange_params.WorkBufferSize    = (uint32_t)PTX_DEMO_APP_EXAMPLE_WORK_BUFFER_SIZE;
        exchange_params.NdefMsgBufferSize = (uint32_t)PTX_DEMO_APP_EXAMPLE_WORK_BUFFER_SIZE;

#if defined(PTX_DEMO_APP_USE_GENERAL_TAG_NDEF_API)
        status = ptx_ProcessState_NDEFDataExchange_General(rul, &exchange_params);
#else
        switch (cardRegistry->ActiveProtocol)
        {
            case ptx_RUL_CardProtocolType_T2T:
            {
                ptx_Utils_Print("Trying to read NDEF Message from T2T ... \n");
                status = ptx_ProcessState_NDEFDataExchange_T2T(rul, &exchange_params);
                break;
            }

            case ptx_RUL_CardProtocolType_T3T:
            {
                ptx_Utils_Print("Trying to read NDEF Message from T3T ... \n");
                status = ptx_ProcessState_NDEFDataExchange_T3T(rul, &exchange_params);
                break;
            }

            case ptx_RUL_CardProtocolType_T4T:
            {
                ptx_Utils_Print("Trying to read NDEF Message from T4T ... \n");
                status = ptx_ProcessState_NDEFDataExchange_T4T(rul, &exchange_params);
                break;
            }

            case ptx_RUL_CardProtocolType_T5T:
            {
                ptx_Utils_Print("Trying to read NDEF Message from T5T ... \n");
                status = ptx_ProcessState_NDEFDataExchange_T5T(rul, &exchange_params);
                break;
            }

            default:
            {
                status = PTX_STATUS_SUCCESS;
                do_ndef_exchange = 0;
                ptx_Utils_Print("Unsupported protocol\n");
                break;
            }
        }
#endif

        if (0 != do_ndef_exchange)
        {
            if (PTX_STATUS_SUCCESS == status)
            {
                ptx_Demo_PrintNdef(&exchange_params.NdefMsgBuffer[0], exchange_params.NdefMsgBufferSize);
            }
            else
            {
                ptx_Utils_Print("NDEF error: %04X\n", status);

                /*
                 * RF-/NFC data exchanges may fail due to various reasons (e.g. movement) or there is
                 * simply no or an invalid NDEF message present - just reset the status and move on.
                 */
                status = PTX_STATUS_SUCCESS;
            }
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
            ptx_Utils_Print("RF off\n");
            ptx_Demo_LedSet(PTX_DEMO_LED_CARD, 0u);
        }
        else
        {
            ptx_Utils_Print("RF off failed: %04X\n", status);
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
            ptx_Utils_Print("Waiting for cards...\n");
            ptx_Demo_CliPrompt();
            *appState = DemoAppState_WaitForDiscovery;
        }
        else
        {
            ptx_Utils_Print("RF reset failed: %04X\n", status);
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
            ptx_Utils_Print("WARN: over-current\n");
            *appState = DemoAppState_Exit;
        }

        if (ptx_RUL_StatusInfoSystem_ErrorTemperature == statusInfoParam->Info.System)
        {
            ptx_Utils_Print("WARN: over-temperature\n");
            status = ptx_ProcessState_RestartSystem(rul, appState);
        }

        if (ptx_RUL_StatusInfoSystem_ErrorOutOfMemory == statusInfoParam->Info.System)
        {
            ptx_Utils_Print("WARNING - Out of Memory!\n");
            gExit = 1u;
        }

        if (ptx_RUL_StatusInfoSystem_NSCReset == statusInfoParam->Info.System)
        {
            ptx_Utils_Print("WARN: unexpected PTX reset - exit\n");
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

#if defined(PTX_DEMO_APP_USE_GENERAL_TAG_NDEF_API)
static ptx_Status_t ptx_ProcessState_NDEFDataExchange_General(ptx_RUL_t *rul, ptx_DemoApplication_ExchangeParams_t *exchangeParams)
{
    ptx_Status_t status = PTX_STATUS_E_GEN_INVALID_PARAMETER;

    /* NDEF Add-on API components */
    ptx_NDEF_t gen_ndef = {0};
    ptx_NDEF_InitParams_t gen_ndef_init_params = {0};

    if ((NULL != rul) && (NULL != exchangeParams))
    {
        /* Start with initializing the Generic NDEF component first */
        gen_ndef_init_params.rul           = rul;
        gen_ndef_init_params.rx            = &exchangeParams->RxBuffer[0];
        gen_ndef_init_params.rxLen         = exchangeParams->RxBufferSize;
        gen_ndef_init_params.tx            = &exchangeParams->TxBuffer[0];
        gen_ndef_init_params.txLen         = exchangeParams->TxBufferSize;
        gen_ndef_init_params.workBuffer    = &exchangeParams->WorkBuffer[0];
        gen_ndef_init_params.workBufferLen = exchangeParams->WorkBufferSize;

        status = ptx_NDEF_Open(&gen_ndef, &gen_ndef_init_params);

        if (PTX_STATUS_SUCCESS == status)
        {
            /* Check if the activated Tag is NDEF-formatted. */
            status = ptx_NDEF_CheckMessage(&gen_ndef);

            if ((PTX_STATUS_SUCCESS == status) && (0u != gPendingTextLen))
            {
                /* A CLI write is pending: write the text as NDEF Text record to this tag. */
                uint32_t msg_len = ptx_Demo_BuildTextNdef(&exchangeParams->NdefMsgBuffer[0], (uint32_t)PTX_DEMO_APP_EXAMPLE_WORK_BUFFER_SIZE,
                                                          gPendingText, gPendingTextLen);

                status = ptx_NDEF_WriteMessage(&gen_ndef, &exchangeParams->NdefMsgBuffer[0], msg_len);

                if (PTX_STATUS_SUCCESS == status)
                {
                    ptx_Utils_Print("WRITE OK: \"%s\"\n", gPendingText);
                    gPendingTextLen = 0;                                /* One-shot: back to read mode. */
                }
                else
                {
                    ptx_Utils_Print("WRITE FAILED: %04X (still armed)\n", status);
                }

                /* Read back what is on the tag now. */
                exchangeParams->NdefMsgBufferSize = (uint32_t)PTX_DEMO_APP_EXAMPLE_WORK_BUFFER_SIZE;
            }
            else if ((PTX_STATUS_SUCCESS != status) && (0u != gPendingTextLen))
            {
                ptx_Utils_Print("WRITE FAILED: not NDEF (%04X)\n", status);
            }
            else
            {
                /* Read mode. */
            }

            if (PTX_STATUS_SUCCESS == status)
            {
                /* Tag is NDEF-formatted - let's try to read an NDEF message from it. */
                status = ptx_NDEF_ReadMessage(&gen_ndef, &exchangeParams->NdefMsgBuffer[0], &exchangeParams->NdefMsgBufferSize);
            }

            /* De-initialize the Generic NDEF component. */
            (void)ptx_NDEF_Close(&gen_ndef);
        }
    }

    return status;
}
#else
static ptx_Status_t ptx_ProcessState_NDEFDataExchange_T2T(ptx_RUL_t *rul, ptx_DemoApplication_ExchangeParams_t *exchangeParams)
{
    ptx_Status_t status = PTX_STATUS_E_GEN_INVALID_PARAMETER;

    /* NDEF Add-on API components */
    ptx_NDEF_T2T_t t2t_ndef = {0};
    ptx_NDEF_T2T_InitParams_t t2t_ndef_init_params = {0};

    if ((NULL != rul) && (NULL != exchangeParams))
    {
        /* Start with initializing the T2T NDEF component first */
        t2t_ndef_init_params.rul            = rul;
        t2t_ndef_init_params.RxBuffer       = &exchangeParams->RxBuffer[0];
        t2t_ndef_init_params.RxBufferSize   = exchangeParams->RxBufferSize;
        t2t_ndef_init_params.TxBuffer       = &exchangeParams->TxBuffer[0];
        t2t_ndef_init_params.TxBufferSize   = exchangeParams->TxBufferSize;
        t2t_ndef_init_params.WorkBuffer     = &exchangeParams->WorkBuffer[0];
        t2t_ndef_init_params.WorkBufferSize = exchangeParams->WorkBufferSize;

        status = ptx_NDEF_T2T_Open(&t2t_ndef, &t2t_ndef_init_params);

        if (PTX_STATUS_SUCCESS == status)
        {
            /* Check if the activated Tag is NDEF-formatted. */
            status = ptx_NDEF_T2T_CheckMessage(&t2t_ndef);

            if (PTX_STATUS_SUCCESS == status)
            {
                /* Tag is NDEF-formatted - let's try to read an NDEF message from it. */
                status = ptx_NDEF_T2T_ReadMessage(&t2t_ndef, &exchangeParams->NdefMsgBuffer[0], &exchangeParams->NdefMsgBufferSize);
            }

            /* De-initialize the T2T NDEF component. */
            (void)ptx_NDEF_T2T_Close(&t2t_ndef);
        }
    }

    return status;
}

static ptx_Status_t ptx_ProcessState_NDEFDataExchange_T3T(ptx_RUL_t *rul, ptx_DemoApplication_ExchangeParams_t *exchangeParams)
{
    ptx_Status_t status = PTX_STATUS_E_GEN_INVALID_PARAMETER;

    /* NDEF Add-on API components */
    ptx_NDEF_T3T_t t3t_ndef = {0};
    ptx_NDEF_T3T_InitParams_t t3t_ndef_init_params = {0};

    if ((NULL != rul) && (NULL != exchangeParams))
    {
        /* Start with initializing the T3T NDEF component first */
        t3t_ndef_init_params.rul            = rul;
        t3t_ndef_init_params.RxBuffer       = &exchangeParams->RxBuffer[0];
        t3t_ndef_init_params.RxBufferSize   = exchangeParams->RxBufferSize;
        t3t_ndef_init_params.TxBuffer       = &exchangeParams->TxBuffer[0];
        t3t_ndef_init_params.TxBufferSize   = exchangeParams->TxBufferSize;

        status = ptx_NDEF_T3T_Open(&t3t_ndef, &t3t_ndef_init_params);

        if (PTX_STATUS_SUCCESS == status)
        {
            /* Check if the activated Tag is NDEF-formatted. */
            status = ptx_NDEF_T3T_CheckMessage(&t3t_ndef);

            if (PTX_STATUS_SUCCESS == status)
            {
                /* Tag is NDEF-formatted - let's try to read an NDEF message from it. */
                status = ptx_NDEF_T3T_ReadMessage(&t3t_ndef, &exchangeParams->NdefMsgBuffer[0], &exchangeParams->NdefMsgBufferSize);
            }

            /* De-initialize the T3T NDEF component. */
            (void)ptx_NDEF_T3T_Close(&t3t_ndef);
        }
    }

    return status;
}

static ptx_Status_t ptx_ProcessState_NDEFDataExchange_T4T(ptx_RUL_t *rul, ptx_DemoApplication_ExchangeParams_t *exchangeParams)
{
    ptx_Status_t status = PTX_STATUS_E_GEN_INVALID_PARAMETER;

    /* NDEF Add-on API components */
    ptx_NDEF_T4T_t t4t_ndef = {0};
    ptx_NDEF_T4T_InitParams_t t4t_ndef_init_params = {0};

    if ((NULL != rul) && (NULL != exchangeParams))
    {
        /* Start with initializing the T4T NDEF component first */
        t4t_ndef_init_params.rul            = rul;
        t4t_ndef_init_params.RxBuffer       = &exchangeParams->RxBuffer[0];
        t4t_ndef_init_params.RxBufferSize   = exchangeParams->RxBufferSize;
        t4t_ndef_init_params.TxBuffer       = &exchangeParams->TxBuffer[0];
        t4t_ndef_init_params.TxBufferSize   = exchangeParams->TxBufferSize;

        status = ptx_NDEF_T4T_Open(&t4t_ndef, &t4t_ndef_init_params);

        if (PTX_STATUS_SUCCESS == status)
        {
            /* Check if the activated Tag is NDEF-formatted. */
            status = ptx_NDEF_T4T_CheckMessage(&t4t_ndef);

            if (PTX_STATUS_SUCCESS == status)
            {
                /* Tag is NDEF-formatted - let's try to read an NDEF message from it. */
                status = ptx_NDEF_T4T_ReadMessage(&t4t_ndef, &exchangeParams->NdefMsgBuffer[0], &exchangeParams->NdefMsgBufferSize);
            }

            /* De-initialize the T4T NDEF component. */
            (void)ptx_NDEF_T4T_Close(&t4t_ndef);
        }
    }

    return status;
}

static ptx_Status_t ptx_ProcessState_NDEFDataExchange_T5T(ptx_RUL_t *rul, ptx_DemoApplication_ExchangeParams_t *exchangeParams)
{
    ptx_Status_t status = PTX_STATUS_E_GEN_INVALID_PARAMETER;

    /* NDEF Add-on API components */
    ptx_NDEF_T5T_t t5t_ndef = {0};
    ptx_NDEF_T5T_InitParams_t t5t_ndef_init_params = {0};

    if ((NULL != rul) && (NULL != exchangeParams))
    {
        /* Start with initializing the T5T NDEF component first */
        t5t_ndef_init_params.rul            = rul;
        t5t_ndef_init_params.RxBuffer       = &exchangeParams->RxBuffer[0];
        t5t_ndef_init_params.RxBufferSize   = exchangeParams->RxBufferSize;
        t5t_ndef_init_params.TxBuffer       = &exchangeParams->TxBuffer[0];
        t5t_ndef_init_params.TxBufferSize   = exchangeParams->TxBufferSize;
        t5t_ndef_init_params.WorkBuffer     = &exchangeParams->WorkBuffer[0];
        t5t_ndef_init_params.WorkBufferSize = exchangeParams->WorkBufferSize;
        t5t_ndef_init_params.isSelected     = 0u;
        t5t_ndef_init_params.isAddressed    = 1u;

        status = ptx_NDEF_T5T_Open(&t5t_ndef, &t5t_ndef_init_params);

        if (PTX_STATUS_SUCCESS == status)
        {
            /* Check if the activated Tag is NDEF-formatted. */
            status = ptx_NDEF_T5T_CheckMessage(&t5t_ndef);

            if (PTX_STATUS_SUCCESS == status)
            {
                /* Tag is NDEF-formatted - let's try to read an NDEF message from it. */
                status = ptx_NDEF_T5T_ReadMessage(&t5t_ndef, &exchangeParams->NdefMsgBuffer[0], &exchangeParams->NdefMsgBufferSize);
            }

            /* De-initialize the T2T NDEF component. */
            (void)ptx_NDEF_T5T_Close(&t5t_ndef);
        }
    }

    return status;
}
#endif

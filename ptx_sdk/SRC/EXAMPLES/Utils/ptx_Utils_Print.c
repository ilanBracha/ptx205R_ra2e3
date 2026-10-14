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
    Module      : Utils
    File        : ptx_Utils_Print.c

    Description : Implementation of the print module. There are 2 different implementations specific for underlying platforms (system).
                  Those 2 versions are included/excluded by specific compile switches.

                  To enable the version for Renesas RA4M2 MCU platform, PTX_PLATFORM_MCU must be defined. This version relies on specific
                  project MCU configuration, i.e. it uses specific predefined UART port and FSP library to print out the messages.

                  To enable the version for Windows/Linux platform, PTX_PLATFORM_OS must be defined. This version prints out the messages
                  to the standard output console by using native printf API.
*/


/*
 * ####################################################################################################################
 * INCLUDES
 * ####################################################################################################################
 */

#include "ptx_Utils_Print.h"

#include <stdio.h>
#include <string.h>
#include <stdarg.h>


#if defined(PTX_PLATFORM_MCU) && !defined(PTX_UTILITY_PRINTING_CONSOLE) && !defined(PTX_UTILITY_PRINTING_RTT)
    /*
     * RA2E3 port: print to UART ptx_uart_dbg (SCI9, P109/P110 -> FPB-RA2E3 J-Link virtual COM port).
     * Baud rate / frame format are taken from the FSP configuration (115200 8N1).
     * A small built-in formatter is used instead of newlib vsnprintf() to keep newlib's printf engine
     * (and malloc/_sbrk) out of the 64 KB / 16 KB RA2E3 image.
     */
    #if !defined(PTX_UTILITY_PRINTING_SERIAL)
        #define PTX_UTILITY_PRINTING_SERIAL
    #endif
    #define PTX_UTILITY_PRINTING_MINI_FORMATTER
    #include "hal_data.h"
#elif defined (PTX_UTILITY_PRINTING_SERIAL)
    #include "hal_data.h"
#elif defined(PTX_UTILITY_PRINTING_RTT)
    /* Optional: print through SEGGER RTT (channel 0, J-Link RTT Viewer). Define PTX_UTILITY_PRINTING_RTT to select. */
    #include "SEGGER_RTT.h"
#endif


/*
 * ####################################################################################################################
 * DEFINES / TYPES
 * ####################################################################################################################
 */

#define PTX_MAX_PRINTBUF_SIZE                     (256U)      /* Maximum message buffer length. Note: last byte shall be 0. */
#define PTX_ASCII_SPACE                           (0x20U)     /* ASCII table SPACE character. */
#define PTX_PRINT_LINE_LENGTH                     80U

#if defined (PTX_UTILITY_PRINTING_SERIAL)

#define PTX_UTILS_UART_DEFAULT_BAUDRATE           (256000u)   /* Baudrate */
#define PTX_UTILS_UART_DEFAULT_BAUDRATE_ERROR     (3000u)     /* Baudrate error, 3% Error, Calculation: x / 1000 */
/* "SCIU" in ASCII.  Used to determine if the control block is open. */
#define SCI_UART_OPEN                             (0x53434955U)

/*
 * Uart states.
 */
typedef enum ptx_Utils_UartState
{
    Uart_State_Idle,
    Uart_State_TxOngoing,
    Uart_State_TxDone,
    Uart_State_TxError
} ptx_Utils_UartState_t;

void ptx_Utils_Print_Callback(uart_callback_args_t *p_args);

static volatile ptx_Utils_UartState_t sUartState = Uart_State_Idle;

/* Simple line-based command input (CLI) on the same UART. Characters are collected in the RX callback. */
#define PTX_UTILS_CLI_LINE_MAX                    (128u)      /* Max. command line length incl. terminating 0. */
static volatile char     sCliRxBuf[PTX_UTILS_CLI_LINE_MAX];
static volatile uint32_t sCliRxLen     = 0;
static volatile uint8_t  sCliLineReady = 0;
static volatile uint8_t  sCliLastWasCr = 0;      /* Swallow the LF of a CR+LF pair (terminal "Enter" = CR LF). */
static uint32_t          sCliEchoLen   = 0;

/* Max. busy-wait loops for one UART transfer (safety net so a UART problem can never hang the application). */
#define PTX_UTILS_UART_TX_TIMEOUT_LOOPS           (2000000u)

#endif

#if defined(PTX_UTILITY_PRINTING_MINI_FORMATTER)
static uint32_t ptx_Utils_MiniFormat(char *out, uint32_t outSize, const char *format, va_list args);
#endif

#if !defined(PTX_UTILITY_PRINTING_RTT)
static void ptx_Utils_Write(char *msgBuffer, uint32_t msgLen);
#endif


/*
 * ####################################################################################################################
 * API FUNCTIONS
 * ####################################################################################################################
 */

void ptx_Utils_Print(const char *format, ...)
{
    va_list argptr;
    va_start(argptr, format);

#if defined(PTX_UTILITY_PRINTING_RTT)
    (void)SEGGER_RTT_vprintf(0U, format, &argptr);
#else
    char buffer[PTX_MAX_PRINTBUF_SIZE];
    memset(buffer, 0, PTX_MAX_PRINTBUF_SIZE);

#if defined(PTX_UTILITY_PRINTING_MINI_FORMATTER)
    uint32_t num_written = ptx_Utils_MiniFormat(buffer, PTX_MAX_PRINTBUF_SIZE, format, argptr);
#else
    uint32_t num_written = (uint32_t)vsnprintf(buffer, PTX_MAX_PRINTBUF_SIZE, format, argptr);
#endif
    if (num_written >= PTX_MAX_PRINTBUF_SIZE)
    {
        /* The output was truncated. Set terminating null character since it has been overwritten. */
        buffer[PTX_MAX_PRINTBUF_SIZE - 1] = '\0';
        num_written = PTX_MAX_PRINTBUF_SIZE - 1;
    }

    /* The following call uses only a null-byte terminated string. */
    ptx_Utils_Write(buffer, num_written);
#endif

    va_end(argptr);
}

void ptx_Utils_Print_Buffer(uint8_t *buffer, uint16_t bufferLength, uint8_t addNewLine, uint8_t printASCII)
{
    uint8_t character_to_print;

    if ((NULL != buffer) && (0 != bufferLength))
    {
        for (uint16_t i = 0; (i < bufferLength) && (i < PTX_MAX_PRINTBUF_SIZE); i++)
        {
            if ((i > 0) && ((i % (PTX_PRINT_LINE_LENGTH - 5) == 0)))
            {
                ptx_Utils_Print("\n     ");
            }

            if (0 == printASCII)
            {
                ptx_Utils_Print("%02X", (uint8_t)buffer[i]);
            }
            else
            {
                character_to_print = (uint8_t)buffer[i];

                /* Avoid unintentional interpretation of ascii-commands */
                if (character_to_print < PTX_ASCII_SPACE)
                {
                    ptx_Utils_Print(".");
                }
                else
                {
                    ptx_Utils_Print("%c", character_to_print);
                }
            }
        }

        if (0 != addNewLine)
        {
            ptx_Utils_Print("\n");
        }
    }
}


#if defined(PTX_DEBUG_PRINTS)

#define PTX_DBG_HEX_MAX_BYTES       (16u)       /* Bytes printed per dump. */
#define PTX_DBG_HEX_MAX_DUMPS       (150u)      /* Total dumps printed (keeps the ~25 KB FW download from flooding the UART). */

void ptx_Utils_Print_Hex(const char *tag, const uint8_t *buf, uint32_t len)
{
    static uint32_t dump_count = 0;

    if (dump_count < PTX_DBG_HEX_MAX_DUMPS)
    {
        dump_count++;
        ptx_Utils_Print("%s n=%u:", tag, len);
        for (uint32_t i = 0; (NULL != buf) && (i < len) && (i < PTX_DBG_HEX_MAX_BYTES); i++)
        {
            ptx_Utils_Print(" %02X", buf[i]);
        }
        ptx_Utils_Print((len > PTX_DBG_HEX_MAX_BYTES) ? " ...\n" : "\n");

        if (PTX_DBG_HEX_MAX_DUMPS == dump_count)
        {
            ptx_Utils_Print("[DBG] hex dump limit reached - further SPI dumps suppressed\n");
        }
    }
}

#endif

/*
 * ####################################################################################################################
 * INTERNAL FUNCTIONS
 * ####################################################################################################################
 */

#if defined (PTX_UTILITY_PRINTING_SERIAL)

static void ptx_Utils_Write(char *msgBuffer, uint32_t msgLen)
{
    /* Send message to MCU UART. Direct access to FSP API. */

    /*
     * NOTE: it is expected here that ptx_uart_dbg uart module instance is valid. No special checking done here.
     *       The instance is typically defined in e2studio project configuration and made available through
     *       automatically generated hal_data module.
     *
     *       This example uses r_sci_uart module. If another UART module is used e.g. r_sci_b_uart, adjust the names if needed
     *       for the call to calculate baudrate API.
     */

    if ((NULL != msgBuffer) && (0 != msgLen))
    {
        /* First, check if interface is open. */
        fsp_err_t err = FSP_ERR_NOT_ENABLED;
        sci_uart_instance_ctrl_t *p_ctrl = (sci_uart_instance_ctrl_t *) ptx_uart_dbg.p_ctrl;
        if (SCI_UART_OPEN != p_ctrl->open)
        {
            /* It is not open. Open and configure it here. */
            err = R_SCI_UART_Open(ptx_uart_dbg.p_ctrl, ptx_uart_dbg.p_cfg);

            if (FSP_SUCCESS == err)
            {
                err = R_SCI_UART_CallbackSet(ptx_uart_dbg.p_ctrl, ptx_Utils_Print_Callback, NULL, NULL);
            }

#if !defined(PTX_UTILITY_PRINTING_MINI_FORMATTER)
            if (FSP_SUCCESS == err)
            {
                baud_setting_t baud_setting;
                err = R_SCI_UART_BaudCalculate(PTX_UTILS_UART_DEFAULT_BAUDRATE, 0, PTX_UTILS_UART_DEFAULT_BAUDRATE_ERROR, &baud_setting);
                if (FSP_SUCCESS == err)
                {
                    err = R_SCI_UART_BaudSet(ptx_uart_dbg.p_ctrl, (void *) &baud_setting);
                }
            }
#endif
            /* RA2E3 port: baud rate comes from the FSP configuration of ptx_uart_dbg (115200) - the
             * terminal on the J-Link VCOM must use the same setting. */
        }
        else
        {
            /* Uart interface is already open. Just proceed. */
            err = FSP_SUCCESS;
        }

        if (FSP_SUCCESS == err)
        {
            /* Interface is ready. */
            sUartState = Uart_State_TxOngoing;
            err = R_SCI_UART_Write(ptx_uart_dbg.p_ctrl, (uint8_t*)msgBuffer, (uint32_t)msgLen);

            if (FSP_SUCCESS == err)
            {
                uint32_t timeout = PTX_UTILS_UART_TX_TIMEOUT_LOOPS;
                while ((Uart_State_TxOngoing == sUartState) && (0u != timeout))
                {
                    /* Wait for the transfer to finish. */
                    --timeout;
                }
            }
        }
    }
}

void ptx_Utils_Print_Callback(uart_callback_args_t * p_args)
{
    /*
     * This callback (name) is predefined in e2studio configuration.
     */
    if (NULL != p_args)
    {
        switch (p_args->event)
        {
            case UART_EVENT_TX_COMPLETE:
                sUartState = Uart_State_TxDone;
                break;

            case UART_EVENT_RX_CHAR:
            {
                /* CLI input: collect one line, finished by CR or LF. Ignored while a line waits to be fetched. */
                char c = (char)(p_args->data & 0xFFu);
                uint8_t lf_after_cr = (uint8_t)((('\n' == c) && (0u != sCliLastWasCr)) ? 1u : 0u);
                sCliLastWasCr = (uint8_t)(('\r' == c) ? 1u : 0u);

                if ((0u == sCliLineReady) && (0u == lf_after_cr))
                {
                    if (('\r' == c) || ('\n' == c))
                    {
                        /* Enter: line complete (an empty line is reported too -> new prompt). */
                        sCliLineReady = 1u;
                    }
                    else if (('\b' == c) || ((char)0x7F == c))
                    {
                        if (0u != sCliRxLen)
                        {
                            sCliRxLen--;
                        }
                    }
                    else if ((c >= ' ') && (sCliRxLen < (PTX_UTILS_CLI_LINE_MAX - 1u)))
                    {
                        sCliRxBuf[sCliRxLen] = c;
                        sCliRxLen++;
                    }
                    else
                    {
                        /* Control character or line full - ignore. */
                    }
                }
                break;
            }

            default:
                /* RX errors / TX_DATA_EMPTY etc. - not relevant for printing. */
                break;
        }
    }
}

uint8_t ptx_Utils_Cli_GetLine(char *line, uint32_t lineSize)
{
    uint8_t line_ready = 0u;
    sci_uart_instance_ctrl_t *p_ctrl = (sci_uart_instance_ctrl_t *) ptx_uart_dbg.p_ctrl;

    /* UART is opened by the first ptx_Utils_Print() call. */
    if ((NULL != line) && (0u != lineSize) && (SCI_UART_OPEN == p_ctrl->open))
    {
        /* Echo what was typed since the last call (terminal has no local echo). */
        uint32_t rx_len = sCliRxLen;
        while (sCliEchoLen < rx_len)
        {
            ptx_Utils_Print("%c", sCliRxBuf[sCliEchoLen]);
            sCliEchoLen++;
        }
        while (sCliEchoLen > rx_len)
        {
            ptx_Utils_Print("\b \b");
            sCliEchoLen--;
        }

        if (0u != sCliLineReady)
        {
            uint32_t i = 0;
            for (; (i < sCliRxLen) && (i < (lineSize - 1u)); i++)
            {
                line[i] = sCliRxBuf[i];
            }
            line[i] = '\0';

            sCliRxLen     = 0;
            sCliEchoLen   = 0;
            sCliLineReady = 0;
            ptx_Utils_Print("\n");
            line_ready = 1u;
        }
    }

    return line_ready;
}

uint8_t ptx_Utils_Cli_AnyKey(void)
{
    uint8_t key = 0u;

    if ((0u != sCliRxLen) || (0u != sCliLineReady))
    {
        /* Something was typed: consume it (not echoed, not executed as command). */
        sCliRxLen     = 0;
        sCliEchoLen   = 0;
        sCliLineReady = 0;
        key = 1u;
    }

    return key;
}

#elif defined(PTX_UTILITY_PRINTING_CONSOLE)

uint8_t ptx_Utils_Cli_AnyKey(void)
{
    return 0u;
}

uint8_t ptx_Utils_Cli_GetLine(char *line, uint32_t lineSize)
{
    (void)line;
    (void)lineSize;
    return 0u;
}

static void ptx_Utils_Write(char *msgBuffer, uint32_t msgLen)
{

    /* Send message to standard output (console). */
    printf("%s", msgBuffer);

    (void)msgLen;
}

#elif defined(PTX_UTILITY_PRINTING_RTT)

/* Not used: ptx_Utils_Print() hands the format string straight to SEGGER_RTT_vprintf(). */

uint8_t ptx_Utils_Cli_AnyKey(void)
{
    return 0u;
}

uint8_t ptx_Utils_Cli_GetLine(char *line, uint32_t lineSize)
{
    /* No CLI input on RTT. */
    (void)line;
    (void)lineSize;
    return 0u;
}

#else

uint8_t ptx_Utils_Cli_AnyKey(void)
{
    return 0u;
}

uint8_t ptx_Utils_Cli_GetLine(char *line, uint32_t lineSize)
{
    (void)line;
    (void)lineSize;
    return 0u;
}

static void ptx_Utils_Write(char *msgBuffer, uint32_t msgLen)
{
    (void)msgBuffer;
    (void)msgLen;
}

#endif

#if defined(PTX_UTILITY_PRINTING_MINI_FORMATTER)

/*
 * Minimal printf-style formatter (RA2E3 flash-size optimized).
 * Supports: %d %i %u %x %X %c %s %% with optional '0' flag, field width and ignored 'l'/'h' length modifiers.
 * '\n' is expanded to "\r\n" for serial terminals. Output is always 0-terminated; returns number of chars written.
 */
static uint32_t ptx_Utils_MiniFormat(char *out, uint32_t outSize, const char *format, va_list args)
{
    static const char hex_digits[] = "0123456789ABCDEF";
    uint32_t len = 0;
    const uint32_t max_len = outSize - 1u;

    while (('\0' != *format) && (len < max_len))
    {
        char c = *format++;

        if ('%' != c)
        {
            if (('\n' == c) && (len < (max_len - 1u)))
            {
                out[len++] = '\r';
            }
            out[len++] = c;
            continue;
        }

        char pad = ' ';
        uint32_t width = 0;

        if ('0' == *format)
        {
            pad = '0';
            format++;
        }
        while ((*format >= '0') && (*format <= '9'))
        {
            width = (width * 10u) + (uint32_t)(*format - '0');
            format++;
        }
        while (('l' == *format) || ('h' == *format))
        {
            format++;
        }

        c = *format;
        if ('\0' == c)
        {
            break;
        }
        format++;

        switch (c)
        {
            case 'd':
            case 'i':
            case 'u':
            case 'x':
            case 'X':
            {
                char digits[12];
                uint32_t n = 0;
                uint32_t base = (('x' == c) || ('X' == c)) ? 16u : 10u;
                uint32_t value;
                uint8_t negative = 0u;

                if (('d' == c) || ('i' == c))
                {
                    int sv = va_arg(args, int);
                    if (sv < 0)
                    {
                        negative = 1u;
                        value = (uint32_t)(-(sv + 1)) + 1u;
                    }
                    else
                    {
                        value = (uint32_t)sv;
                    }
                }
                else
                {
                    value = (uint32_t)va_arg(args, unsigned int);
                }

                do
                {
                    char d = hex_digits[value % base];
                    if (('x' == c) && (d >= 'A'))
                    {
                        d = (char)(d + ('a' - 'A'));
                    }
                    digits[n++] = d;
                    value /= base;
                } while (0u != value);

                if (0u != negative)
                {
                    digits[n++] = '-';
                }
                while ((width > n) && (len < max_len))
                {
                    out[len++] = pad;
                    width--;
                }
                while ((0u != n) && (len < max_len))
                {
                    out[len++] = digits[--n];
                }
                break;
            }

            case 'c':
                out[len++] = (char)va_arg(args, int);
                break;

            case 's':
            {
                const char *str = va_arg(args, const char *);
                if (NULL == str)
                {
                    str = "(null)";
                }
                while (('\0' != *str) && (len < max_len))
                {
                    out[len++] = *str++;
                }
                break;
            }

            default:
                /* '%%' and unsupported conversions: print the character as-is. */
                out[len++] = c;
                break;
        }
    }

    out[len] = '\0';
    return len;
}

#endif

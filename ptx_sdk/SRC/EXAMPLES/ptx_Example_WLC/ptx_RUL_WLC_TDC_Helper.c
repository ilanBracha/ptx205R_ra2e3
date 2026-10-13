/* \file
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
    Module      : High Level TDC Protocol
    File        : EXAMPLES/ptx_Example_WLC/ptx_RUL_WLC_TDC_Helper.c

    Description : Example implementation of a very light weight high level protocol on top
                  of the TDC.
*/

/*
 * ####################################################################################################################
 * INCLUDES
 * ####################################################################################################################
 */
#include "ptx_RUL_WLC_TDC_Helper.h"
#include "ptx_RUL_WLC_Proprietary.h"
#include <string.h>

#define PTX_RUL_WLC_TDC_HELPER_TIMEOUT             (35u)
#define PTX_RUL_WLC_TDC_HELPER_PAYLOAD_LEN         (62u)
#define PTX_RUL_WLC_TDC_HELPER_BUFFER_LEN          (PTX_RUL_WLC_TDC_HELPER_PAYLOAD_LEN + 1u)
#define PTX_RUL_WLC_TDC_HELPER_REQ_FRAME_LEN       (0x05)
#define PTX_RUL_WLC_TDC_HELPER_WRITE_REQ_OPC       (0xA5)
#define PTX_RUL_WLC_TDC_HELPER_READ_REQ_OPC        (0xA6)

ptx_Status_t ptx_RUL_WLC_TDC_HlpRx(ptx_RUL_WLC_t *ctx, uint8_t *rxData, uint32_t *rxDataLen)
{
    ptx_Status_t status = PTX_STATUS_E_GEN_INVALID_PARAMETER;

    if ( PTX_COMP_CHECK(ctx, Comp_WLC) && (NULL != rxData) && (NULL != rxDataLen) )
    {

        uint8_t frame_buffer[PTX_RUL_WLC_TDC_HELPER_BUFFER_LEN];
        uint8_t frame_buffer_len = sizeof(frame_buffer);

        /*
         * Assemble buffer for initial 'handshake'.
         * Indicate that we want to request data from the listener.
         */
        frame_buffer[0] = PTX_RUL_WLC_TDC_HELPER_READ_REQ_OPC;
        /*
         * The poller sends the size information of his buffer to the listener.
         * Not evaluated by the listener YET!
         */
        frame_buffer[1] = (uint8_t) ((*rxDataLen >> 16) & 0xFF);
        frame_buffer[2] = (uint8_t) ((*rxDataLen >> 8)  & 0xFF);
        frame_buffer[3] = (uint8_t) ((*rxDataLen >> 0)  & 0xFF);
        /* End the frame again the request data opcode. */
        frame_buffer[4] = PTX_RUL_WLC_TDC_HELPER_READ_REQ_OPC;

        /* Transmit the initial frame of the handshake.
         * We wait a maximum of PTX_RUL_WLC_TDC_HELPER_TIMEOUT for the listener to read the frame.
         */
        status = ptx_RUL_WLC_Proprietary_TDC_Write(ctx, frame_buffer, PTX_RUL_WLC_TDC_HELPER_REQ_FRAME_LEN, PTX_RUL_WLC_TDC_HELPER_TIMEOUT);

        if (PTX_STATUS_SUCCESS == status)
        {
            /*
             * In case initial frame was successfully sent,
             * the listener must respond with the data length it
             * wants to send to us!
             */
            frame_buffer_len = sizeof(frame_buffer);

            /* We wait a maximum of PTX_RUL_WLC_TDC_HELPER_TIMEOUT again to receive the frame. */
            status = ptx_RUL_WLC_Proprietary_TDC_Read(ctx, frame_buffer, &frame_buffer_len, PTX_RUL_WLC_TDC_HELPER_TIMEOUT);

            /* Validate the frame (it has the same structure as the initial frame). */
            if (     (PTX_STATUS_SUCCESS == status)
                     &&  (PTX_RUL_WLC_TDC_HELPER_REQ_FRAME_LEN == frame_buffer_len)
                     &&  (PTX_RUL_WLC_TDC_HELPER_READ_REQ_OPC == frame_buffer[0])
                     &&  (PTX_RUL_WLC_TDC_HELPER_READ_REQ_OPC == frame_buffer[4]) )
            {
                uint32_t bytes_to_read = 0;
                /* Retrieve the 24-bit length information. */
                bytes_to_read |= ((frame_buffer[1] << 16) & 0xFF0000);
                bytes_to_read |= ((frame_buffer[2] << 8)  & 0x00FF00);
                bytes_to_read |= ((frame_buffer[3] << 0)  & 0x0000FF);

                uint32_t rx_data_offset = 0;
                uint32_t pending_len = bytes_to_read;

                /* Calculate the amount of frames to receive. */
                uint32_t frames_to_read = (pending_len / PTX_RUL_WLC_TDC_HELPER_PAYLOAD_LEN) + 1u;

                /* Receive the data frame by frame. */
                while ( (PTX_STATUS_SUCCESS == status) && (0 != frames_to_read) )
                {

                    frame_buffer_len = sizeof(frame_buffer);
                    status = ptx_RUL_WLC_Proprietary_TDC_Read(ctx, frame_buffer, &frame_buffer_len, PTX_RUL_WLC_TDC_HELPER_TIMEOUT);

                    /*
                     * TDC frames have a maximum length of 63 bytes of payload. In this simple protocol the very first byte
                     * of each frame contains the actual frame number (wraps around every 256 frames). So, 62 bytes of 'real'
                     * payload are left for usage.
                     */
                    if ( (PTX_STATUS_SUCCESS == status)
                         &&  (0 != frame_buffer_len)                                             /* Check if we received something. */
                         &&  (frame_buffer[0] == (rx_data_offset / PTX_RUL_WLC_TDC_HELPER_PAYLOAD_LEN)) )   /* Validate frame number. */
                    {
                        /* Minus 1 due to frame number byte. */
                        uint8_t actual_payload = (uint8_t) (frame_buffer_len - 1u);
                        /* Copy the frame into the correct destination buffer location. */
                        memcpy(&rxData[rx_data_offset], &frame_buffer[1], actual_payload);
                        /* Update all the helper variables. */
                        pending_len -= actual_payload;
                        rx_data_offset += actual_payload;
                        --frames_to_read;
                    }
                    else
                    {
                        status = PTX_STATUS_E_GEN_INTERNAL;
                    }
                }

                if (PTX_STATUS_SUCCESS == status)
                {
                    /* Set the number of received bytes. */
                    *rxDataLen = bytes_to_read;
                }
            }
            else
            {
                status = PTX_STATUS_E_GEN_INTERNAL;
            }
        }

        if (PTX_STATUS_SUCCESS != status)
        {
            *rxDataLen = 0;
        }
    }

    return status;
}


ptx_Status_t ptx_RUL_WLC_TDC_HlpTx(ptx_RUL_WLC_t *ctx, uint8_t *txData, uint32_t txDataLen)
{
    ptx_Status_t status = PTX_STATUS_E_GEN_INVALID_PARAMETER;

    if ( PTX_COMP_CHECK(ctx, Comp_WLC) && (NULL != txData) && (0xFFFFFF >= txDataLen) )
    {

        uint8_t frame_buffer[PTX_RUL_WLC_TDC_HELPER_BUFFER_LEN];
        /*
         * Assemble buffer for initial 'handshake'.
         * Indicate that we want to send data to the listener.
         */
        frame_buffer[0] = PTX_RUL_WLC_TDC_HELPER_WRITE_REQ_OPC;
        /* The poller sends the size information of the amount of data to the listener. */
        frame_buffer[1] = (uint8_t) ((txDataLen >> 16) & 0xFF);
        frame_buffer[2] = (uint8_t) ((txDataLen >> 8)  & 0xFF);
        frame_buffer[3] = (uint8_t) ((txDataLen >> 0)  & 0xFF);
        /* End the frame again the send data opcode. */
        frame_buffer[4] = PTX_RUL_WLC_TDC_HELPER_WRITE_REQ_OPC;

        /* Transmit the initial frame of the handshake.
         * We wait a maximum of PTX_RUL_WLC_TDC_HELPER_TIMEOUT for the listener to read the frame.
         */
        status = ptx_RUL_WLC_Proprietary_TDC_Write(ctx, frame_buffer, PTX_RUL_WLC_TDC_HELPER_REQ_FRAME_LEN, PTX_RUL_WLC_TDC_HELPER_TIMEOUT);

        if (PTX_STATUS_SUCCESS == status)
        {
            uint8_t frame_buffer_len = sizeof(frame_buffer);
            status = ptx_RUL_WLC_Proprietary_TDC_Read(ctx, frame_buffer, &frame_buffer_len, PTX_RUL_WLC_TDC_HELPER_TIMEOUT);

            /* Validate the frame (it has the same structure as the initial frame). */
            if (         (PTX_STATUS_SUCCESS == status)
                         &&  (PTX_RUL_WLC_TDC_HELPER_REQ_FRAME_LEN == frame_buffer_len)
                         &&  (PTX_RUL_WLC_TDC_HELPER_WRITE_REQ_OPC == frame_buffer[0])
                         &&  (PTX_RUL_WLC_TDC_HELPER_WRITE_REQ_OPC == frame_buffer[4]) )
            {

                /* Check the HLP buffer size of the listener. */
                uint32_t listener_buffer_size = 0;
                /* Retrieve the 24-bit length information. */
                listener_buffer_size |= ((frame_buffer[1] << 16) & 0xFF0000);
                listener_buffer_size |= ((frame_buffer[2] << 8)  & 0x00FF00);
                listener_buffer_size |= ((frame_buffer[3] << 0)  & 0x0000FF);

                /* Only start wiht the transmission if listeners HLP buffer is large enough. */
                if (listener_buffer_size >= txDataLen)
                {
                    uint32_t tx_data_offset = 0;
                    uint32_t pending_len = txDataLen;
                    /* Calculate the amount of frames to send. */
                    uint32_t frames_to_write = (pending_len / PTX_RUL_WLC_TDC_HELPER_PAYLOAD_LEN) + 1u;
                    /* Write frame by frame in a loop. */
                    while ( (PTX_STATUS_SUCCESS == status) && (0 != frames_to_write) )
                    {
                        uint8_t payload_len = (uint8_t)(pending_len > PTX_RUL_WLC_TDC_HELPER_PAYLOAD_LEN ? PTX_RUL_WLC_TDC_HELPER_PAYLOAD_LEN : pending_len);
                        /* Frame number. */
                        frame_buffer[0] = (uint8_t) (tx_data_offset / PTX_RUL_WLC_TDC_HELPER_PAYLOAD_LEN);
                        /* Copy payload. */
                        memcpy(&frame_buffer[1], &txData[tx_data_offset], payload_len);
                        /* Write to listener. */
                        status = ptx_RUL_WLC_Proprietary_TDC_Write(ctx, frame_buffer, payload_len + 1u, PTX_RUL_WLC_TDC_HELPER_TIMEOUT);
                        /* Update all the helper variables. */
                        pending_len -= payload_len;
                        tx_data_offset += payload_len;
                        --frames_to_write;
                    }
                }
                else
                {
                    status = PTX_STATUS_E_GEN_INSUFFICIENT_RESOURCES;  /* Buffer size on listener side too small. */
                }
            }
            else
            {
                status = PTX_STATUS_E_GEN_INTERNAL;
            }
        }
    }

    return status;
}

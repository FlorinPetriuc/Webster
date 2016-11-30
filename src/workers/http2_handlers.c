/*
 * Copyright (C) 2016 Florin Petriuc. All rights reserved.
 * Initial release: Florin Petriuc <petriuc.florin@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version
 * 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 */

#include "../main.h"

int handle_http2_receive(void *arg)
{
    return 1;
}

int handle_http2_send_settings(void *arg)
{
    struct handler_prm_t *prm = arg;

    int ret;

    if(prm->file_header_len == 0)
    {
        prm->out_buffer[0] = 0;
        prm->out_buffer[1] = 0;
        prm->out_buffer[2] = 36;

        //frame type
        prm->out_buffer[3] = HTTP2_SETTINGS;

        //flags
        prm->out_buffer[4] = 0;

        //stream id
        prm->out_buffer[5] = 0;
        prm->out_buffer[6] = 0;
        prm->out_buffer[7] = 0;
        prm->out_buffer[8] = 0;

        //SETTINGS_HEADER_TABLE_SIZE
        prm->out_buffer[9] = (SETTINGS_HEADER_TABLE_SIZE >> 8) & 0xFF;
        prm->out_buffer[10] = SETTINGS_HEADER_TABLE_SIZE & 0xFF;
        prm->out_buffer[11] = (prm->server_h2settings->HEADER_TABLE_SIZE >> 24) & 0xFF;
        prm->out_buffer[12] = (prm->server_h2settings->HEADER_TABLE_SIZE >> 16) & 0xFF;
        prm->out_buffer[13] = (prm->server_h2settings->HEADER_TABLE_SIZE >> 8) & 0xFF;
        prm->out_buffer[14] = (prm->server_h2settings->HEADER_TABLE_SIZE) & 0xFF;

        //SETTINGS_ENABLE_PUSH
        prm->out_buffer[15] = (SETTINGS_ENABLE_PUSH >> 8) & 0xFF;;
        prm->out_buffer[16] = SETTINGS_ENABLE_PUSH & 0xFF;
        prm->out_buffer[17] = (prm->server_h2settings->ENABLE_PUSH >> 24) & 0xFF;
        prm->out_buffer[18] = (prm->server_h2settings->ENABLE_PUSH >> 16) & 0xFF;
        prm->out_buffer[19] = (prm->server_h2settings->ENABLE_PUSH >> 8) & 0xFF;
        prm->out_buffer[20] = (prm->server_h2settings->ENABLE_PUSH) & 0xFF;

        //SETTINGS_MAX_CONCURRENT_STREAMS
        prm->out_buffer[21] = (SETTINGS_MAX_CONCURRENT_STREAMS >> 8) & 0xFF;;
        prm->out_buffer[22] = SETTINGS_MAX_CONCURRENT_STREAMS & 0xFF;
        prm->out_buffer[23] = (prm->server_h2settings->MAX_CONCURRENT_STREAMS >> 24) & 0xFF;
        prm->out_buffer[24] = (prm->server_h2settings->MAX_CONCURRENT_STREAMS >> 16) & 0xFF;
        prm->out_buffer[25] = (prm->server_h2settings->MAX_CONCURRENT_STREAMS >> 8) & 0xFF;
        prm->out_buffer[26] = (prm->server_h2settings->MAX_CONCURRENT_STREAMS) & 0xFF;

        //SETTINGS_INITIAL_WINDOW_SIZE
        prm->out_buffer[27] = (SETTINGS_INITIAL_WINDOW_SIZE >> 8) & 0xFF;;
        prm->out_buffer[28] = SETTINGS_INITIAL_WINDOW_SIZE & 0xFF;
        prm->out_buffer[29] = (prm->server_h2settings->INITIAL_WINDOW_SIZE >> 24) & 0xFF;
        prm->out_buffer[30] = (prm->server_h2settings->INITIAL_WINDOW_SIZE >> 16) & 0xFF;
        prm->out_buffer[31] = (prm->server_h2settings->INITIAL_WINDOW_SIZE >> 8) & 0xFF;
        prm->out_buffer[32] = (prm->server_h2settings->INITIAL_WINDOW_SIZE) & 0xFF;

        //SETTINGS_MAX_FRAME_SIZE
        prm->out_buffer[33] = (SETTINGS_MAX_FRAME_SIZE >> 8) & 0xFF;;
        prm->out_buffer[34] = SETTINGS_MAX_FRAME_SIZE & 0xFF;
        prm->out_buffer[35] = (prm->server_h2settings->MAX_FRAME_SIZE >> 24) & 0xFF;
        prm->out_buffer[36] = (prm->server_h2settings->MAX_FRAME_SIZE >> 16) & 0xFF;
        prm->out_buffer[37] = (prm->server_h2settings->MAX_FRAME_SIZE >> 8) & 0xFF;
        prm->out_buffer[38] = (prm->server_h2settings->MAX_FRAME_SIZE) & 0xFF;

        //SETTINGS_MAX_HEADER_LIST_SIZE
        prm->out_buffer[39] = (SETTINGS_MAX_HEADER_LIST_SIZE >> 8) & 0xFF;;
        prm->out_buffer[40] = SETTINGS_MAX_HEADER_LIST_SIZE & 0xFF;
        prm->out_buffer[41] = (prm->server_h2settings->MAX_HEADER_LIST_SIZE >> 24) & 0xFF;
        prm->out_buffer[42] = (prm->server_h2settings->MAX_HEADER_LIST_SIZE >> 16) & 0xFF;
        prm->out_buffer[43] = (prm->server_h2settings->MAX_HEADER_LIST_SIZE >> 8) & 0xFF;
        prm->out_buffer[44] = (prm->server_h2settings->MAX_HEADER_LIST_SIZE) & 0xFF;

        prm->file_header_len = 45;
    }

    ret = send(prm->sockFD, prm->out_buffer + prm->out_buf_offset,
                                    prm->file_header_len - prm->out_buf_offset, 0);

    if(ret < 0)
    {
        if(errno == EAGAIN) return 0;
        if(errno == EINTR) return 0;
        if(errno == EWOULDBLOCK) return 0;

        return 1;
    }

    if(ret == 0)
    {
        return 0;
    }

    prm->out_buf_offset += ret;

    if(prm->out_buf_offset < prm->file_header_len)
    {
        return 0;
    }

    prm->file_header_len = 0;
    prm->out_buf_offset = 0;

    prm->processor = handle_http2_receive;

    prm->expiration_date = _utcTime() + 5;

    return prm->processor(arg);
}
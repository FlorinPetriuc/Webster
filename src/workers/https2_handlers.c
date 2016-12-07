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

int handle_https2_send_page(void *arg)
{
    struct handler_prm_t *prm = arg;

    int ret;
    int errRet;

    uint32_t len;

send_again:

    if(prm->file_len - prm->file_offset == 0)
    {
        goto out_empty_file;
    }

    if(prm->file_header_len == 0)
    {
        if(prm->client_h2settings->MAX_FRAME_SIZE <= prm->file_len - prm->file_offset)
        {
            prm->file_chunk_len = prm->client_h2settings->MAX_FRAME_SIZE;

            if(prm->file_chunk_len >= prm->out_buf_len)
            {
                prm->file_chunk_len = prm->out_buf_len;
            }

            //flags -> end stream = 0
            prm->out_buffer[4] = 0;
        }
        else
        {
            prm->file_chunk_len = prm->file_len - prm->file_offset;

            if(prm->file_chunk_len >= prm->out_buf_len)
            {
                prm->file_chunk_len = prm->out_buf_len;
                prm->out_buffer[4] = 0;
            }
            else
            {
                //flags -> end stream = 1
                prm->out_buffer[4] = 1;
            }
        }

        prm->out_buffer[0] = (prm->file_chunk_len >> 16) & 0xFF;
        prm->out_buffer[1] = (prm->file_chunk_len >> 8) & 0xFF;
        prm->out_buffer[2] = prm->file_chunk_len & 0xFF;

        //type
        prm->out_buffer[3] = HTTP2_DATA;

        //reserved + stream id
        prm->out_buffer[5] = (prm->request->stream_id >> 24) & 0x7F;
        prm->out_buffer[6] = (prm->request->stream_id >> 16) & 0xFF;
        prm->out_buffer[7] = (prm->request->stream_id >> 8) & 0xFF;
        prm->out_buffer[8] = (prm->request->stream_id) & 0xFF;

        prm->file_header_len = 9;
    }

    if(prm->out_buf_offset != prm->file_header_len)
    {
        ERR_clear_error();

        ret = SSL_write(prm->ssl, prm->out_buffer + prm->out_buf_offset,
                                    prm->file_header_len - prm->out_buf_offset);

        if(ret <= 0)
        {
            errRet = SSL_get_error(prm->ssl, ret);

            if(errRet == SSL_ERROR_WANT_READ) return 0;
            if(errRet == SSL_ERROR_WANT_WRITE) return 0;

            return 1;
        }

        prm->out_buf_offset += ret;

        if(prm->out_buf_offset != prm->file_header_len)
        {
            return 0;
        }
    }

    if(prm->file_chunk_loaded != prm->file_chunk_len)
    {
        ret = read(prm->fileFD, prm->out_buffer + prm->file_chunk_loaded, prm->file_chunk_len - prm->file_chunk_loaded);

        if(ret < 0)
        {
            switch(errno)
            {
                case EINTR:
                case EAGAIN: return 0;

                default: return 1;
            }
        }

        if(ret == 0) return 0;

        prm->file_chunk_loaded += ret;

        if(prm->file_chunk_loaded != prm->file_chunk_len)
        {
            return 0;
        }

        prm->body = prm->out_buffer;
    }

    ret = SSL_write(prm->ssl, prm->body, prm->file_chunk_len);

    if(ret <= 0)
    {
        errRet = SSL_get_error(prm->ssl, ret);

        if(errRet == SSL_ERROR_WANT_READ) return 0;
        if(errRet == SSL_ERROR_WANT_WRITE) return 0;

        return 1;
    }

    prm->file_offset += ret;
    prm->file_chunk_len -= ret;
    prm->body += ret;

    if(prm->file_chunk_len)
    {
        return 0;
    }

    prm->file_header_len = 0;
    prm->out_buf_offset = 0;
    prm->file_chunk_loaded = 0;

    goto send_again;

out_empty_file:
    close(prm->fileFD);
    prm->fileFD = -1;

    prm->file_len = 0;
    prm->file_offset = 0;
    prm->file_chunk_len = 0;
    prm->file_header_len = 0;
    prm->out_buf_offset = 0;
    prm->file_chunk_loaded = 0;
    prm->body = NULL;

    prm->processor = handle_https2_receive;

    return 0;
}

int handle_https2_receive(void *arg)
{
    struct handler_prm_t *prm = arg;

    int readRet;
    int errRet;
    int i;

    uint32_t len;

    enum http_request_type_t req_type;

    ERR_clear_error();

    readRet = SSL_read(prm->ssl, prm->in_buffer + prm->in_buf_offset, prm->in_buf_len - prm->in_buf_offset);

    if(readRet <= 0)
    {
        errRet = SSL_get_error(prm->ssl, readRet);

        if(errRet == SSL_ERROR_WANT_READ) return 0;
        if(errRet == SSL_ERROR_WANT_WRITE) return 0;

        return 1;
    }

    prm->in_buf_offset += readRet;

again:

    if(prm->in_buf_offset < 3)
    {
        return 0;
    }

    len = ((prm->in_buffer[0] << 16) | (prm->in_buffer[1] << 8) | (prm->in_buffer[2])) + 9;

    if(prm->in_buf_offset < len)
    {
        return 0;
    }

    logWrite(LOG_TYPE_INFO, "Got http2 frame of len %u from %u total bytes read",
                                                            2, len, prm->in_buf_offset);

    switch(prm->in_buffer[3])
    {
        case HTTP2_DATA:
        {
        }
        break;

        case HTTP2_HEADERS:
        {
            if(prm->client_h2settings == NULL)
            {
                logWrite(LOG_TYPE_ERROR, "Got http2 header without settings", 0);

                return 1;
            }

            if(prm->client_h2settings->MAX_FRAME_SIZE == 0)
            {
                logWrite(LOG_TYPE_ERROR, "Client MAX_FRAME_SIZE is 0", 0);

                return 1;
            }

            prm->header = prm->in_buffer;
            prm->request = process_http2_header(prm->header, prm->request);

            if(prm->request->last_header == 0)
            {
                return 0;
            }

            switch(prm->request->req_type)
            {
                case HTTP_GET: return process_http_get_request(prm, 1);
                default: return 1;
            }
        }
        break;

        case HTTP2_PRIORITY:
        {
        }
        break;

        case HTTP2_RST_STREAM:
        {
        }
        break;

        case HTTP2_SETTINGS:
        {
            prm->header = prm->in_buffer;
            prm->client_h2settings = process_http2_settings_request(prm->header, prm->client_h2settings);
        }
        break;

        case HTTP2_PUSH_PROMISE:
        {
        }
        break;

        case HTTP2_PING:
        {
        }
        break;

        case HTTP2_GO_AWAY:
        {
        }
        break;

        case HTTP2_WINDOW_UPDATE:
        {
        }
        break;

        default: break;
    }

    memcpy(prm->in_buffer, prm->in_buffer + len, prm->in_buf_offset - len);
    prm->in_buf_offset -= len;

    logWrite(LOG_TYPE_INFO, "Moved %u bytes. Offset becomes %u.", 2, len, prm->in_buf_offset);

    prm->expiration_date = _utcTime() + 5;

    goto again;
}

int handle_https2_send_settings(void *arg)
{
    struct handler_prm_t *prm = arg;

    int ret;
    int errRet;

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

    ERR_clear_error();

    ret = SSL_write(prm->ssl, prm->out_buffer + prm->out_buf_offset,
                                    prm->file_header_len - prm->out_buf_offset);

    if(ret <= 0)
    {
        errRet = SSL_get_error(prm->ssl, ret);

        if(errRet == SSL_ERROR_WANT_READ) return 0;
        if(errRet == SSL_ERROR_WANT_WRITE) return 0;

        return 1;
    }

    prm->out_buf_offset += ret;

    if(prm->out_buf_offset < prm->file_header_len)
    {
        return 0;
    }

    prm->file_header_len = 0;
    prm->out_buf_offset = 0;

    prm->processor = handle_https2_receive;

    prm->expiration_date = _utcTime() + 5;

    return prm->processor(arg);
}
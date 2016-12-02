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

#include "../../main.h"

static const char *working_directory = ".";
static int working_directory_len = 1;

void http2_set_working_directory(const char *path)
{
    if(path == NULL)
    {
        working_directory = ".";
        working_directory_len = 1;

        logWrite(LOG_TYPE_INFO, "HTTP2 working directory is %s", 1, working_directory);

        return;
    }

    working_directory = path;
    working_directory_len = strlen(working_directory);

    logWrite(LOG_TYPE_INFO, "HTTP2 working directory is %s", 1, working_directory);
}

const struct http2_settings *http2_server_settings()
{
    static const struct http2_settings h2_srv_settings = { 4096, 1, 100, 4096, 4096, 4096 };

    return &h2_srv_settings;
}

struct http2_settings *process_http2_settings_request(const unsigned char *payload,
                                                    struct http2_settings *old_settings)
{
    uint16_t config_idx;

    uint32_t config_value;
    uint32_t frame_len;

    int i;

    struct http2_settings *ret;

    frame_len = ((payload[0] << 16) | (payload[1] << 8) | payload[2]) + 9;

    if(payload[4] & 1)
    {
        if(frame_len > 9)
        {
            logWrite(LOG_TYPE_ERROR, "Got settings frame with ack and payload", 0);

            free(old_settings);

            return NULL;
        }

        return old_settings;
    }

    if((frame_len - 9) % 6)
    {
        logWrite(LOG_TYPE_ERROR, "Got settings frame which is not multiple of 6: %u", 1, frame_len);

        free(old_settings);

        return NULL;
    }

    if(old_settings == NULL)
    {
        ret = xmalloc(sizeof(struct http2_settings));

        ret->HEADER_TABLE_SIZE = 4096;
        ret->ENABLE_PUSH = 1;
        ret->MAX_CONCURRENT_STREAMS = 100;
        ret->INITIAL_WINDOW_SIZE = 65535;
        ret->MAX_FRAME_SIZE = 16384;
        ret->MAX_HEADER_LIST_SIZE = (uint32_t) -1;
    }
    else
    {
        ret = old_settings;
    }

    if(frame_len == 9)
    {
        return ret;
    }

    for(i = 9; i < frame_len; i += 6)
    {
        config_idx = (payload[i] << 8) | payload[i + 1];
        config_value = (payload[i + 2] << 24) | (payload[i + 3] << 16) |
                       (payload[i + 4] << 8) | payload[i + 5];

        logWrite(LOG_TYPE_INFO, "Setting http idx %u to %u", 2, config_idx, config_value);

        switch(config_idx)
        {
            case SETTINGS_HEADER_TABLE_SIZE:
            {
                ret->HEADER_TABLE_SIZE = config_value;
            }
            break;

            case SETTINGS_ENABLE_PUSH:
            {
                ret->ENABLE_PUSH = config_value;
            }
            break;

            case SETTINGS_MAX_CONCURRENT_STREAMS:
            {
                ret->MAX_CONCURRENT_STREAMS = config_value;
            }
            break;

            case SETTINGS_INITIAL_WINDOW_SIZE:
            {
                ret->INITIAL_WINDOW_SIZE = config_value;
            }
            break;

            case SETTINGS_MAX_FRAME_SIZE:
            {
                ret->MAX_FRAME_SIZE = config_value & 0xFFFFFF;
            }
            break;

            case SETTINGS_MAX_HEADER_LIST_SIZE:
            {
                ret->MAX_HEADER_LIST_SIZE = config_value;
            }
            break;
        }
    }

    return ret;
}

static int check_path(const char *path)
{
    int depth = 0;
    int i = 0;

    while(i < strlen(path))
    {
        if(path[i] == '/')
        {
            ++depth;

            ++i;

            if(path[i] == '/')
            {
                return 1;
            }

            continue;
        }

        if(path[i] != '.')
        {
            ++i;

            continue;
        }

        ++i;

        if(path[i] != '.')
        {
            continue;
        }

        depth -= 2;

        if(depth < 0)
        {
            return 1;
        }

        ++i;
    }

    return 0;
}

struct http_request_t *process_http2_header(const unsigned char *payload, struct http_request_t *old_req)
{
    struct http_request_t *ret = old_req;

    char *path = NULL;
    char *abs_path = NULL;

    unsigned int content_length = 0;

    uint32_t stream_id;

    enum http_request_type_t req_type;

    req_type = hpack_resolve_method(payload);

    content_length = hpack_resolve_content_len(payload);

    path = hpack_resolve_path(payload);

    if(path)
    {
        if(check_path(path))
        {
            logWrite(LOG_TYPE_ERROR, "Path %s failed security check", 1, path);
        }
        else
        {
            abs_path = xmalloc(working_directory_len + strlen(path) + 1);
            sprintf(abs_path, "%s%s", working_directory, path);
        }

        free(path);
    }

    stream_id = ((payload[5] << 24) & 0x7F) | (payload[6] << 16) | (payload[7] << 8) | (payload[8]);

    if(ret == NULL)
    {
        ret = xmalloc(sizeof(struct http_request_t));
        memset(ret, 0, sizeof(struct http_request_t));
    }

    if(req_type != HTTP_UNSUPPORTED_METHOD)
    {
        ret->req_type = req_type;
    }

    if(abs_path)
    {
        free(ret->abs_path);
        ret->abs_path = abs_path;
    }

    if(content_length)
    {
        ret->content_length = content_length;
    }

    ret->stream_id = stream_id;

    ret->last_header = payload[4] & 4;

    logWrite(LOG_TYPE_INFO, "Header resolved to req:%d path %s, version: 2.X, stream id: %u",
                                                3, req_type, abs_path, stream_id);

    return ret;
}
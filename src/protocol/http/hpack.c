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

#define HTTP2_GET           2
#define HTTP2_POST          3
#define HTTP2_CONTENT_TYPE  31

enum http_request_type_t hpack_resolve_method(const unsigned char *payload)
{
    uint32_t i = 9;
    uint32_t len;

    unsigned char indexed_value;
    unsigned char value_len;

    len = (payload[0] << 16) | (payload[1] << 8) | payload[2];

    while(i < len + 9)
    {
        if(payload[i] & 0x80)
        {
            indexed_value = (payload[i] & 0x7F);

            switch(indexed_value)
            {
                case HTTP2_GET: return HTTP_GET;
            }

            ++i;

            continue;
        }

        if(payload[i] & 0x40)
        {
            indexed_value = (payload[i] & 0x3F);
            ++i;

            value_len = (payload[i] & 0x7F);

            i += value_len + 1;

            if(indexed_value)
            {
                continue;
            }

            value_len = (payload[i] & 0x7F);

            i += value_len + 1;

            continue;
        }

        if((payload[i] >> 4) == 0 ||
           (payload[i] >> 4) == 1)
        {
            indexed_value = (payload[i] & 0x0F);
            ++i;

            value_len = (payload[i] & 0x7F);

            i += value_len + 1;

            if(indexed_value)
            {
                continue;
            }

            value_len = (payload[i] & 0x7F);

            i += value_len + 1;

            continue;
        }

        logWrite(LOG_TYPE_INFO, "Got unsupported hpack method", 0);

        return HTTP_UNSUPPORTED_METHOD;
    }

    return HTTP_UNSUPPORTED_METHOD;
}

static char *huffman_decode(const unsigned char *value, const unsigned char len)
{
    return NULL;
}

static unsigned int hpack_process_content_length(const unsigned char *content_length)
{
    unsigned int ret = 0;

    unsigned char huffman_coded;
    unsigned char value_len;
    unsigned char i;

    char *huffman_decoded = NULL;

    huffman_coded = content_length[0] & 0x80;
    value_len = content_length[0] & 0x7F;

    if(huffman_coded)
    {
        huffman_decoded = huffman_decode(content_length + 1, value_len);

        if(huffman_decoded == NULL)
        {
            return 0;
        }

        sscanf(huffman_decoded, "%u", &ret);
        free(huffman_decoded);
    }
    else
    {
        for(i = 0; i < value_len; ++i)
        {
            ret = (ret * 10) + (content_length[i + 1] - '0');
        }
    }

    return ret;
}

static char *hpack_process_path(const unsigned char *path)
{
    char *ret = NULL;

    unsigned char huffman_coded;
    unsigned char value_len;
    unsigned char i;

    char *huffman_decoded = NULL;

    huffman_coded = path[0] & 0x80;
    value_len = path[0] & 0x7F;

    if(huffman_coded)
    {
        huffman_decoded = huffman_decode(path + 1, value_len);

        if(huffman_decoded == NULL)
        {
            return NULL;
        }

        ret = huffman_decoded;
    }
    else
    {
        ret = xmalloc(value_len + 1);
        memcpy(ret, path + 1, value_len);
        ret[value_len] = '\0';
    }

    return ret;
}

char *hpack_resolve_path(const unsigned char *payload)
{
    uint32_t i = 9;
    uint32_t len;

    unsigned char indexed_value;
    unsigned char value_len;

    char *ret;

    len = (payload[0] << 16) | (payload[1] << 8) | payload[2];

    while(i < len + 9)
    {
        if(payload[i] & 0x80)
        {
            indexed_value = (payload[i] & 0x7F);

            switch(indexed_value)
            {
                case 5:
                {
                    ret = xmalloc(sizeof("/index.html"));
                    memcpy(ret, "/index.html", sizeof("/index.html"));
                }
                return ret;

                case 4:
                {
                    ret = hpack_process_path(payload + i + 1);
                }
                return ret;
            }

            ++i;

            continue;
        }

        if(payload[i] & 0x40)
        {
            indexed_value = (payload[i] & 0x3F);
            ++i;

            value_len = (payload[i] & 0x7F);

            i += value_len + 1;

            if(indexed_value)
            {
                continue;
            }

            value_len = (payload[i] & 0x7F);

            i += value_len + 1;

            continue;
        }

        if((payload[i] >> 4) == 0 ||
           (payload[i] >> 4) == 1)
        {
            indexed_value = (payload[i] & 0x0F);
            ++i;

            value_len = (payload[i] & 0x7F);

            i += value_len + 1;

            if(indexed_value)
            {
                continue;
            }

            value_len = (payload[i] & 0x7F);

            i += value_len + 1;

            continue;
        }

        logWrite(LOG_TYPE_INFO, "Got unsupported hpack method", 0);

        return NULL;
    }

    return NULL;
}

unsigned int hpack_resolve_content_len(const unsigned char *payload)
{
    uint32_t i = 9;
    uint32_t len;

    unsigned char indexed_value;
    unsigned char value_len;

    len = (payload[0] << 16) | (payload[1] << 8) | payload[2];

    while(i < len + 9)
    {
        if(payload[i] & 0x80)
        {
            ++i;

            continue;
        }

        if(payload[i] & 0x40)
        {
            indexed_value = (payload[i] & 0x3F);
            ++i;

            value_len = (payload[i] & 0x7F);

            switch(indexed_value)
            {
                case 0:
                {
                    i += value_len + 1;
                }
                break;

                case HTTP2_CONTENT_TYPE: return hpack_process_content_length(payload + i);

                default:
                {
                    i += value_len + 1;
                }
                continue;
            }

            value_len = (payload[i] & 0x7F);

            i += value_len + 1;

            continue;
        }

        if((payload[i] >> 4) == 0 ||
           (payload[i] >> 4) == 1)
        {
            indexed_value = (payload[i] & 0x0F);
            ++i;

            value_len = (payload[i] & 0x7F);

            switch(indexed_value)
            {
                case 0:
                {
                    i += value_len + 1;
                }
                break;

                case HTTP2_CONTENT_TYPE: return hpack_process_content_length(payload + i);

                default:
                {
                    i += value_len + 1;
                }
                continue;
            }

            value_len = (payload[i] & 0x7F);

            i += value_len + 1;

            continue;
        }

        logWrite(LOG_TYPE_INFO, "Got unsupported hpack method", 0);

        return 0;
    }

    return 0;
}
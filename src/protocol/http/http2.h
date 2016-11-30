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

#ifndef _HTTP2_H_
#define _HTTP2_H_

#define SETTINGS_HEADER_TABLE_SIZE      1
#define SETTINGS_ENABLE_PUSH            2
#define SETTINGS_MAX_CONCURRENT_STREAMS 3
#define SETTINGS_INITIAL_WINDOW_SIZE    4
#define SETTINGS_MAX_FRAME_SIZE         5
#define SETTINGS_MAX_HEADER_LIST_SIZE   6

#define HTTP2_DATA          0
#define HTTP2_HEADERS       1
#define HTTP2_PRIORITY      2
#define HTTP2_RST_STREAM    3
#define HTTP2_SETTINGS      4
#define HTTP2_PUSH_PROMISE  5
#define HTTP2_PING          6
#define HTTP2_GO_AWAY       7
#define HTTP2_WINDOW_UPDATE 8

struct http2_settings
{
    uint32_t HEADER_TABLE_SIZE;
    uint32_t ENABLE_PUSH;
    uint32_t MAX_CONCURRENT_STREAMS;
    uint32_t INITIAL_WINDOW_SIZE;
    uint32_t MAX_FRAME_SIZE;
    uint32_t MAX_HEADER_LIST_SIZE;
};

const struct http2_settings *http2_server_settings();

#endif
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

#ifndef _HANDLERS_H_
#define _HANDLERS_H_

typedef int (*processor_t)(void *arg);

enum http_comm_type_t
{
    UNENCRYPTED_HTTP,
    ENCRYPTED_HTTP,
    ENCRYPTED_OR_UNENCRYPTED_HTTP,
};

struct handler_prm_t
{
    int sockFD;
    int fileFD;
    int epoll_fd;

    SSL *ssl;

    const char *certificate;

    enum http_comm_type_t comm_type;

    unsigned char *in_buffer;
    unsigned int in_buf_offset;
    unsigned int in_buf_len;

    unsigned char *out_buffer;
    unsigned int out_buf_offset;
    unsigned int out_buf_len;

    unsigned long long file_offset;
    unsigned long long file_len;
    unsigned int file_header_len;

    const char *header;
    const char *body;

    unsigned char critical;
    unsigned char has_expiration;
    time_t expiration_date;

    struct http_request_t *request;
    struct http2_settings *client_h2settings;
    const struct http2_settings *server_h2settings;

    processor_t processor;
};

#endif
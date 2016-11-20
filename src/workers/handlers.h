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

    enum http_comm_type_t comm_type;

    unsigned char buffer_malloced;
    unsigned char *buffer;
    unsigned int buf_offset;
    unsigned int buf_len;
    unsigned int file_len;
    unsigned int file_header_len;

    unsigned char file_header_sent;
    unsigned char critical;
    unsigned char has_expiration;
    time_t expiration_date;

    struct http_request_t *request;

    const char *certificate;

    processor_t processor;
};

#endif
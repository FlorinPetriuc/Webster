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

#ifndef _HTTP_H_
#define _HTTP_H_

#define HTTP_MAX_BUFFER_LEN 4096
#define HTTP_DEFAULT_PAGE   "index.html"

enum http_request_type_t
{
    HTTP_GET,
    HTTP2_PRISM,
    HTTP_UNSUPPORTED_METHOD
};

struct http_request_t
{
    enum http_request_type_t req_type;

    char *abs_path;

    unsigned int content_length;

    unsigned version_major;
    unsigned version_minor;
};

struct handler_prm_t;

struct http_request_t *parse_http_header(const char *header);
void http_set_working_directory(const char *path);
int process_http_get_request(struct handler_prm_t *prm, const unsigned char encrypted);

#endif
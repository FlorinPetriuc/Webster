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

#ifndef _HTTPS_HANDLERS_H_
#define _HTTPS_HANDLERS_H_

int handle_https_send_page_header(void *arg);
int handle_https_send_page(void *arg);
int handle_https_send_bad_request(void *arg);
int handle_https_send_not_found(void *arg);
int handle_https_send_server_error(void *arg);
int handle_https_receive(void *arg);
int handle_https_accept(void *arg);

#endif
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

#ifndef _LOG_H_
#define _LOG_H_

enum log_type_t
{
    LOG_TYPE_INFO,
    LOG_TYPE_ERROR,
    LOG_TYPE_FATAL
};

int logInit(const char *log_file);

void logWrite(enum log_type_t type, const char *template, const unsigned int n, ...);
void log_ssl_errors();

#endif
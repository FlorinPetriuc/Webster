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

static int log_fd = STDOUT_FILENO;
static FILE *log_stream = NULL;

static pthread_mutex_t log_mtx;

int logInit(const char *log_file)
{
    INIT_MUTEX(log_mtx);

    if(log_file == NULL)
    {
        log_fd = STDOUT_FILENO;
        log_stream = stdout;

        return 0;
    }

    log_fd = open(log_file, O_WRONLY | O_APPEND | O_CREAT);

    if(log_fd < 0)
    {
        perror("can not initialize log file");

        log_fd = STDOUT_FILENO;

        return -1;
    }

    log_stream = fdopen(log_fd, "w");

    return 0;
}

void logWrite(enum log_type_t type, const char *template, const unsigned int n, ...)
{
    va_list args;

    int i;

    char buffer[1024];

    time_t now;

    struct tm *now_tm;

    time(&now);
    now_tm = gmtime(&now);

	i = strftime(buffer, 100,"[%d/%b/%Y:%H:%M:%S]", now_tm);
    
    switch(type)
    {
        case LOG_TYPE_INFO:
        {
            i += sprintf(buffer + i, "[INFO]");
        }
        break;

        case LOG_TYPE_ERROR:
        {
            i += sprintf(buffer + i, "[ERROR]");
        }
        break;

        case LOG_TYPE_FATAL:
        {
            i += sprintf(buffer + i, "[FATAL]");
        }
        break;
    }

    va_start(args, n);
    i += vsnprintf(buffer + i, 900, template, args);
    va_end(args);

    buffer[i] = '\n';

    TAKE_MUTEX(log_mtx);
    write(log_fd, buffer, i + 1);
    RELEASE_MUTEX(log_mtx);
}

void log_ssl_errors()
{
    if(log_stream == NULL)
    {
        return;
    }

    TAKE_MUTEX(log_mtx);

    fseek(log_stream, 0, SEEK_END);

    ERR_print_errors_fp(log_stream);

    RELEASE_MUTEX(log_mtx);
}
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

#include "main.h"

static const char *get_cmd_parameter(const int argc, char **argv, const char *param)
{
    int i;

    for(i = 1; i < argc; ++i)
    {
        if(string_starts_with(argv[i], param))
        {
            continue;
        }

        return argv[i] + strlen(param);
    }

    return NULL;
}

int main(int argc, char **argv)
{
    int ret;
    int epoll_fd;
    int srvFD;

    struct handler_prm_t *acc_str;

    const char *working_dir = NULL;
    const char *logfile = NULL;

    const char *numWorkersC;
    unsigned int numWorkers = 8;

    const char *portC;
    unsigned short int port = 80;

    logInit(NULL);
    logWrite(LOG_TYPE_INFO, "Starting up webster v%d", 1, WEBSTER_VERSION);

    signal(SIGPIPE, SIG_IGN);

    working_dir = get_cmd_parameter(argc, argv, "-wdir=");
    http_set_working_directory(working_dir);

    logfile = get_cmd_parameter(argc, argv, "-logfile=");
    logInit(logfile);

    numWorkersC = get_cmd_parameter(argc, argv, "-workers=");

    if(sscanf(numWorkersC, "%u", &numWorkers) != 1)
    {
        numWorkers = 8;
    }

    if(numWorkers <= 0)
    {
        numWorkers = 8;
    }

    portC = get_cmd_parameter(argc, argv, "-port=");

    if(sscanf(portC, "%hu", &port) != 1)
    {
        port = 80;
    }

    epoll_fd = initialize_pool();

    if(epoll_fd < 0)
    {
        return 1;
    }

    srvFD = start_server(port);

    if(srvFD < 0)
    {
        return 1;
    }

    acc_str = xmalloc(sizeof(struct handler_prm_t));
    acc_str->sockFD = srvFD;
    acc_str->fileFD = -1;
    acc_str->epoll_fd = epoll_fd;

    acc_str->buffer_malloced = 0;
    acc_str->buffer = NULL;
    acc_str->buf_offset = 0;
    acc_str->buf_len = 0;
    acc_str->file_len = 0;
    acc_str->file_header_len = 0;

    acc_str->file_header_sent = 0;
    acc_str->critical = 1;
    acc_str->has_expiration = 0;
    acc_str->expiration_date = 0;

    acc_str->request = NULL;

    acc_str->processor = handle_http_accept;

    ret = submit_to_pool(epoll_fd, acc_str);

    if(ret < 0)
    {
        return 1;
    }

    logWrite(LOG_TYPE_INFO, "Initializing %u workers", 1, numWorkers);

    start_pool_threads(numWorkers - 1, &epoll_fd);
    pool_worker(&epoll_fd);

    return 0;
}
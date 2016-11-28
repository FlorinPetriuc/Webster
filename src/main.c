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

static int pool_submit_server_socket(const int srvFD, const int epoll_fd,
                                            enum http_comm_type_t comm_type, const char *certificate)
{
    struct handler_prm_t *acc_str;

    if(epoll_fd < 0)
    {
        return 1;
    }

    if(srvFD < 0)
    {
        return 1;
    }

    acc_str = xmalloc(sizeof(struct handler_prm_t));
    acc_str->sockFD = srvFD;
    acc_str->ssl = NULL;
    acc_str->fileFD = -1;
    acc_str->epoll_fd = epoll_fd;

    acc_str->certificate = certificate;
    acc_str->comm_type = comm_type;

    acc_str->in_buffer = NULL;
    acc_str->out_buffer = NULL;
    acc_str->buf_offset = 0;
    acc_str->buf_len = 0;

    acc_str->header = NULL;
    acc_str->body = NULL;

    acc_str->file_offset = 0;
    acc_str->file_len = 0;
    acc_str->file_header_len = 0;

    acc_str->critical = 1;
    acc_str->has_expiration = 0;
    acc_str->expiration_date = 0;

    acc_str->request = NULL;

    acc_str->processor = handle_http_accept;

    return submit_to_pool(epoll_fd, acc_str);
}

int main(int argc, char **argv)
{
    int epoll_fd = -1;
    int srvFD = -1;

    const char *working_dir = NULL;
    const char *logfile = NULL;

    const char *numWorkersC = NULL;
    unsigned int numWorkers = 8;

    const char *certificate = "./certificate.crt";

    const char *sTemp = NULL;
    unsigned short int port = 80;
    unsigned short int sPort = 443;

    unsigned char no_ssl = 0;

    logInit(NULL);
    logWrite(LOG_TYPE_INFO, "Starting up webster v%d", 1, WEBSTER_VERSION);

    signal(SIGPIPE, SIG_IGN);

    if(ssl_init())
    {
        return 1;
    }

    working_dir = get_cmd_parameter(argc, argv, "-wdir=");
    http_set_working_directory(working_dir);

    logfile = get_cmd_parameter(argc, argv, "-logfile=");
    logInit(logfile);

    numWorkersC = get_cmd_parameter(argc, argv, "-workers=");
    if(numWorkersC &&
       sscanf(numWorkersC, "%u", &numWorkers) != 1)
    {
        numWorkers = 8;
    }

    if(numWorkers <= 0)
    {
        numWorkers = 8;
    }

    sTemp = get_cmd_parameter(argc, argv, "-port=");
    if(sTemp &&
       sscanf(sTemp, "%hu", &port) != 1)
    {
        port = 80;
    }

    sTemp = get_cmd_parameter(argc, argv, "-sPort=");
    if(sTemp &&
       sscanf(sTemp, "%hu", &sPort) != 1)
    {
        sPort = 443;
    }

    sTemp = get_cmd_parameter(argc, argv, "-no_ssl=");
    if(sTemp &&
       sscanf(sTemp, "%hhu", &no_ssl) != 1)
    {
        no_ssl = 0;
    }

    certificate = get_cmd_parameter(argc, argv, "-certificate=");
    if(certificate == NULL)
    {
        certificate = "./certificate.crt";
    }

    epoll_fd = initialize_pool();
    srvFD = start_server(port);

    if(no_ssl)
    {
        if(pool_submit_server_socket(srvFD, epoll_fd,
                                            UNENCRYPTED_HTTP, NULL) < 0)
        {
            return 1;
        }
    }
    else if(port == sPort)
    {
        if(pool_submit_server_socket(srvFD, epoll_fd,
                        ENCRYPTED_OR_UNENCRYPTED_HTTP, certificate) < 0)
        {
            return 1;
        }
    }
    else
    {
        if(pool_submit_server_socket(srvFD, epoll_fd,
                                            UNENCRYPTED_HTTP, NULL) < 0)
        {
            return 1;
        }

        srvFD = start_server(sPort);

        if(pool_submit_server_socket(srvFD, epoll_fd,
                                            ENCRYPTED_HTTP, certificate) < 0)
        {
            return 1;
        }
    }

    logWrite(LOG_TYPE_INFO, "Initializing %u workers", 1, numWorkers);

    start_pool_threads(numWorkers - 1, &epoll_fd);
    pool_worker(&epoll_fd);

    return 0;
}
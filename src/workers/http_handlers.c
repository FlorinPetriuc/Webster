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

int handle_http_send_page_header(void *arg)
{
    struct handler_prm_t *prm = arg;

    int ret;

    const char *mime_type;

    if(prm->file_header_len == 0)
    {
        if(string_ends_with(prm->request->abs_path, ".html") == 0)
        {
            mime_type = "text/html; charset=utf-8";
        }
        else if(string_ends_with(prm->request->abs_path, ".css") == 0)
        {
            mime_type = "text/css";
        }
        else if(string_ends_with(prm->request->abs_path, ".js") == 0)
        {
            mime_type = "application/javascript";
        }
        else
        {
            mime_type = "application/octet-stream";
        }

        prm->file_header_len = sprintf(prm->out_buffer, "HTTP/1.1 200 OK\r\n"
                                                        "Content-type: %s\r\n"
                                                        "Content-Length: %u\r\n"
                                                        "\r\n", mime_type, prm->file_len);
    }

    ret = send(prm->sockFD, prm->out_buffer + prm->buf_offset, prm->file_header_len - prm->buf_offset, 0);

    if(ret < 0)
    {
        if(errno == EAGAIN) return 0;
        if(errno == EINTR) return 0;
        if(errno == EWOULDBLOCK) return 0;

        return 1;
    }

    if(ret == 0)
    {
        return 0;
    }

    prm->buf_offset += ret;

    if(prm->buf_offset < prm->file_header_len)
    {
        return 0;
    }

    prm->buf_offset = 0;
    prm->file_offset = 0;

    prm->processor = handle_http_send_page;

    return prm->processor(arg);
}

int handle_http_send_page(void *arg)
{
    struct handler_prm_t *prm = arg;

    int ret;

    ret = sendfile(prm->sockFD, prm->fileFD, NULL, prm->file_len - prm->file_offset);

    if(ret < 0)
    {
        if(errno == EAGAIN) return 0;
        if(errno == EINTR) return 0;
        if(errno == EWOULDBLOCK) return 0;

        return 1;
    }

    if(ret == 0)
    {
        return 0;
    }

    prm->file_offset += ret;

    prm->expiration_date = _utcTime() + 5;

    if(prm->file_len != prm->file_offset)
    {
        return 0;
    }

    logWrite(LOG_TYPE_INFO, "Request completed", 0);

    if(prm->request->version_major < 1)
    {
        return 2;
    }

    if(prm->request->version_major == 1 && prm->request->version_minor == 0)
    {
        return 2;
    }

    close(prm->fileFD);
    prm->fileFD = -1;

    free(prm->request->abs_path);
    free(prm->request);
    prm->request = NULL;

    prm->processor = handle_http_receive;

    return prm->processor(arg);
}

int handle_http_send_bad_request(void *arg)
{
    const char *bad_request = HTTP_BAD_REQUEST;
    const int bad_request_len = MACRO_STRLEN(HTTP_BAD_REQUEST);

    struct handler_prm_t *prm = arg;

    int ret;

    if(prm->buf_offset == bad_request_len)
    {
        return 2;
    }

    ret = send(prm->sockFD, bad_request + prm->buf_offset, bad_request_len - prm->buf_offset, 0);

    if(ret < 0)
    {
        if(errno == EAGAIN) return 0;
        if(errno == EINTR) return 0;
        if(errno == EWOULDBLOCK) return 0;

        return 1;
    }

    if(ret == 0)
    {
        return 0;
    }

    prm->buf_offset += ret;

    if(prm->buf_offset == bad_request_len)
    {
        return 2;
    }

    return 0;
}

int handle_http_send_not_found(void *arg)
{
    const char *bad_request = HTTP_NOT_FOUND;
    const int bad_request_len = MACRO_STRLEN(HTTP_NOT_FOUND);

    struct handler_prm_t *prm = arg;

    int ret;

    if(prm->buf_offset == bad_request_len)
    {
        return 2;
    }

    ret = send(prm->sockFD, bad_request + prm->buf_offset, bad_request_len - prm->buf_offset, 0);

    if(ret < 0)
    {
        if(errno == EAGAIN) return 0;
        if(errno == EINTR) return 0;
        if(errno == EWOULDBLOCK) return 0;

        return 1;
    }

    if(ret == 0)
    {
        return 0;
    }

    prm->buf_offset += ret;

    if(prm->buf_offset == bad_request_len)
    {
        return 2;
    }

    return 0;
}

int handle_http_send_server_error(void *arg)
{
    const char *bad_request = HTTP_SERVER_ERROR;
    const int bad_request_len = MACRO_STRLEN(HTTP_SERVER_ERROR);

    struct handler_prm_t *prm = arg;

    int ret;

    if(prm->buf_offset == bad_request_len)
    {
        return 2;
    }

    ret = send(prm->sockFD, bad_request + prm->buf_offset, bad_request_len - prm->buf_offset, 0);

    if(ret < 0)
    {
        if(errno == EAGAIN) return 0;
        if(errno == EINTR) return 0;
        if(errno == EWOULDBLOCK) return 0;

        return 1;
    }

    if(ret == 0)
    {
        return 0;
    }

    prm->buf_offset += ret;

    if(prm->buf_offset == bad_request_len)
    {
        return 2;
    }

    return 0;
}

int handle_http_receive(void *arg)
{
    struct handler_prm_t *prm = arg;

    char *header_end;

    struct stat buf;

    int readRet;
    int len;

    if(prm->buf_offset == prm->buf_len - 1)
    {
        return 1;
    }

    readRet = recv(prm->sockFD, prm->in_buffer + prm->buf_offset, prm->buf_len - prm->buf_offset - 1, 0);

    if(readRet < 0)
    {
        if(errno == EAGAIN) return 0;
        if(errno == EINTR) return 0;
        if(errno == EWOULDBLOCK) return 0;

        return 1;
    }

    if(readRet == 0)
    {
        return 0;
    }

    prm->buf_offset += readRet;
    prm->in_buffer[prm->buf_offset] = '\0';

    header_end = strstr(prm->in_buffer, "\r\n\r\n");

    if(header_end == NULL)
    {
        if(prm->buf_offset < prm->buf_len - 1)
        {
            return 0;
        }

        logWrite(LOG_TYPE_ERROR, "Request header is too long", 0);

        prm->expiration_date = _utcTime() + 5;

        prm->buf_offset = 0;

        prm->processor = handle_http_send_bad_request;

        return prm->processor(arg);
    }

    header_end += MACRO_STRLEN("\r\n\r\n");
    header_end[0] = '\0';

    logWrite(LOG_TYPE_INFO, "Got request header: %s", 1, prm->in_buffer);

    prm->expiration_date = _utcTime() + 5;

    prm->buf_offset = 0;

    prm->request = parse_http_header(prm->in_buffer);
    if(prm->request == NULL)
    {
        prm->processor = handle_http_send_bad_request;

        return prm->processor(arg);
    }

    prm->fileFD = open(prm->request->abs_path, O_RDONLY);

    if(prm->fileFD < 0)
    {
        prm->processor = handle_http_send_not_found;

        return prm->processor(arg);
    }

    if(fstat(prm->fileFD, &buf) != 0)
    {
        close(prm->fileFD);
        prm->fileFD = -1;

        prm->processor = handle_http_send_server_error;

        return prm->processor(arg);
    }

    if(!(S_ISDIR(buf.st_mode)))
    {
        prm->file_len = buf.st_size;
        prm->file_header_len = 0;

        prm->processor = handle_http_send_page_header;

        return prm->processor(arg);
    }

    close(prm->fileFD);

    len = strlen(prm->request->abs_path);

    if(prm->request->abs_path[len - 1] == '/')
    {
        prm->request->abs_path = realloc(prm->request->abs_path, len + sizeof(HTTP_DEFAULT_PAGE));
        memcpy(prm->request->abs_path + len, HTTP_DEFAULT_PAGE, sizeof(HTTP_DEFAULT_PAGE));
    }
    else
    {
        prm->request->abs_path = realloc(prm->request->abs_path, len + sizeof(HTTP_DEFAULT_PAGE) + 1);
        sprintf(prm->request->abs_path + len, "/%s", HTTP_DEFAULT_PAGE);
    }

    prm->fileFD = open(prm->request->abs_path, O_RDONLY);

    if(prm->fileFD < 0)
    {
        prm->processor = handle_http_send_not_found;

        return prm->processor(arg);
    }

    if(fstat(prm->fileFD, &buf) != 0)
    {
        close(prm->fileFD);
        prm->fileFD = -1;

        prm->processor = handle_http_send_server_error;

        return prm->processor(arg);
    }

    if(S_ISDIR(buf.st_mode))
    {
        close(prm->fileFD);
        prm->fileFD = -1;

        prm->processor = handle_http_send_not_found;

        return prm->processor(arg);
    }

    prm->file_len = buf.st_size;
    prm->file_header_len = 0;

    prm->processor = handle_http_send_page_header;

    return prm->processor(arg);
}

int handle_http_or_https_check(void *arg)
{
    struct handler_prm_t *prm = arg;

    int ret;

    unsigned char type;

    ret = recv(prm->sockFD, &type, 1, MSG_PEEK);

    if(ret < 0)
    {
        if(errno == EAGAIN) return 0;
        if(errno == EINTR) return 0;
        if(errno == EWOULDBLOCK) return 0;

        return 1;
    }

    if(ret == 0)
    {
        return 0;
    }

    //check for TLS client hello
    if(type == 0x16)
    {
        prm->processor = handle_https_accept;
    }
    else
    {
        prm->processor = handle_http_receive;
    }

    return prm->processor(arg);
}

int handle_http_accept(void *arg)
{
    struct handler_prm_t *prm = arg;
    struct handler_prm_t *cli_prm;

    unsigned char ip[16];

    socklen_t ip_len = 16;

    int client;

    client = accept_conection(prm->sockFD, ip, &ip_len);

    if(client < 0)
    {
        return 1;
    }

    logWrite(LOG_TYPE_INFO, "got connection %d", 1, client);

    BUG_CHECK(ip_len != 4 && ip_len != 16);

    if(ip_len == 4)
    {
        logWrite(LOG_TYPE_INFO, "IPv4: " IPV4_PRINT_TEMPLATE, 4, IPV4_PRINT(ip));
    }

    if(ip_len == 16)
    {
        logWrite(LOG_TYPE_INFO, "IPv6: " IPV6_PRINT_TEMPLATE, 16, IPV6_PRINT(ip));
    }

    cli_prm = xmalloc(sizeof(struct handler_prm_t));
    cli_prm->sockFD = client;
    cli_prm->ssl = NULL;
    cli_prm->fileFD = -1;
    cli_prm->epoll_fd = prm->epoll_fd;

    cli_prm->certificate = prm->certificate;
    cli_prm->comm_type = prm->comm_type;

    cli_prm->buf_len = HTTP_MAX_BUFFER_LEN;
    cli_prm->in_buffer = xmalloc(cli_prm->buf_len);
    cli_prm->out_buffer = xmalloc(cli_prm->buf_len);
    cli_prm->buf_offset = 0;

    cli_prm->header = NULL;
    cli_prm->body = NULL;

    cli_prm->file_len = 0;
    cli_prm->file_offset = 0;
    cli_prm->file_header_len = 0;

    cli_prm->critical = 0;
    cli_prm->has_expiration = 1;
    cli_prm->expiration_date = _utcTime() + 5;

    cli_prm->request = NULL;

    switch(cli_prm->comm_type)
    {
        case UNENCRYPTED_HTTP:
        {
            cli_prm->processor = handle_http_receive;
        }
        break;

        case ENCRYPTED_OR_UNENCRYPTED_HTTP:
        {
            cli_prm->processor = handle_http_or_https_check;
        }
        break;

        default:
        {
            cli_prm->processor = handle_https_accept;
        }
        break;
    }

    if(submit_to_pool(cli_prm->epoll_fd, cli_prm))
    {
        logWrite(LOG_TYPE_ERROR, "Could not submit %d to work pool %d", 2,
                                                    client, cli_prm->epoll_fd);

        close(client);

        free(cli_prm->in_buffer);
        free(cli_prm->out_buffer);

        free(cli_prm);
    }

    return 0;
}
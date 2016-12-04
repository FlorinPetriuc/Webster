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

int handle_http_send_page_headers(void *arg)
{
    struct handler_prm_t *prm = arg;

    int ret;

    const char *mime_type;

    char number_as_char[20];

    unsigned char i;

    if(prm->file_header_len == 0)
    {
        switch(prm->request->version_major)
        {
            case 2:
            {
                //type
                prm->out_buffer[3] = HTTP2_HEADERS;

                //flags = END_HEADERS
                prm->out_buffer[4] = 4;

                //reserved + stream id
                prm->out_buffer[5] = (prm->request->stream_id >> 24) & 0x7F;
                prm->out_buffer[6] = (prm->request->stream_id >> 16) & 0xFF;
                prm->out_buffer[7] = (prm->request->stream_id >> 8) & 0xFF;
                prm->out_buffer[8] = (prm->request->stream_id) & 0xFF;

                //200 ok
                prm->out_buffer[9] = 0x88;

                //content type
                prm->out_buffer[10] = 0x5F;
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
                prm->out_buffer[11] = strlen(mime_type);
                memcpy(prm->out_buffer + 12, mime_type, prm->out_buffer[11]);

                //content-length
                prm->out_buffer[12 + prm->out_buffer[11]] = 0x5C;
                sprintf(number_as_char, "%llu", prm->file_len);

                prm->out_buffer[13 + prm->out_buffer[11]] = strlen(number_as_char);

                for(i = 0; i < prm->out_buffer[13 + prm->out_buffer[11]]; ++i)
                {
                    prm->out_buffer[14 + prm->out_buffer[11] + i] = number_as_char[i];
                }

                prm->file_header_len = 14 + prm->out_buffer[11] + prm->out_buffer[13 + prm->out_buffer[11]];

                //len
                prm->out_buffer[0] = ((prm->file_header_len - 9) >> 16) & 0xFF;
                prm->out_buffer[1] = ((prm->file_header_len - 9) >> 8) & 0xFF;
                prm->out_buffer[2] = (prm->file_header_len - 9) & 0xFF;
            }
            break;

            case 1:
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
                                                                "Content-Length: %llu\r\n"
                                                                "\r\n", mime_type, prm->file_len);
            }
            break;

            default: return 1;
        }
    }

    ret = send(prm->sockFD, prm->out_buffer + prm->out_buf_offset, prm->file_header_len - prm->out_buf_offset, 0);

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

    prm->out_buf_offset += ret;

    if(prm->out_buf_offset < prm->file_header_len)
    {
        return 0;
    }

    prm->out_buf_offset = 0;

    prm->file_header_len = 0;

    if(prm->request->version_major == 2)
    {
        prm->processor = handle_http2_send_page;
    }
    else
    {
        prm->processor = handle_http_send_page;
    }

    return prm->processor(arg);
}

int handle_http_send_page(void *arg)
{
    struct handler_prm_t *prm = arg;

    int ret;

    if(prm->file_len - prm->file_offset == 0)
    {
        goto out_empty_file;
    }

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

out_empty_file:
    close(prm->fileFD);
    prm->fileFD = -1;

    free(prm->request->abs_path);
    free(prm->request);
    prm->request = NULL;

    prm->file_len = 0;
    prm->file_offset = 0;

    prm->processor = handle_http_receive;

    return 0;
}

int handle_http_send_bad_request(void *arg)
{
    struct handler_prm_t *prm = arg;

    int ret;

    if(prm->file_header_len == 0)
    {
        switch(prm->request->version_major)
        {
            case 2:
            {
                //len
                prm->out_buffer[0] = 0;
                prm->out_buffer[1] = 0;
                prm->out_buffer[2] = 4;

                //type
                prm->out_buffer[3] = HTTP2_HEADERS;

                //flags = END_STREAM & END_HEADERS
                prm->out_buffer[4] = 5;

                //reserved + stream id
                prm->out_buffer[5] = (prm->request->stream_id >> 24) & 0x7F;
                prm->out_buffer[6] = (prm->request->stream_id >> 16) & 0xFF;
                prm->out_buffer[7] = (prm->request->stream_id >> 8) & 0xFF;
                prm->out_buffer[8] = (prm->request->stream_id) & 0xFF;

                //400 bad request
                prm->out_buffer[9] = 0x8C;

                //content-length 0
                prm->out_buffer[10] = 0x5C;
                prm->out_buffer[11] = 0x01;
                prm->out_buffer[12] = 0x30;

                prm->file_header_len = 13;
            }
            break;

            case 1:
            {
                prm->file_header_len = sprintf(prm->out_buffer,
                                                "HTTP/%hhu.%hhu 400 Bad Request\r\n"\
                                                "Content-Length: 0\r\n"\
                                                "\r\n",
                                                prm->request->version_major, prm->request->version_minor);
            }
            break;

            default: return 1;
        }
    }

    ret = send(prm->sockFD, prm->out_buffer + prm->out_buf_offset,
                            prm->file_header_len - prm->out_buf_offset, 0);

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

    prm->out_buf_offset += ret;

    if(prm->out_buf_offset != prm->file_header_len)
    {
        return 0;
    }

    if(prm->request->version_major < 1)
    {
        return 2;
    }

    if(prm->request->version_major == 1 && prm->request->version_minor == 0)
    {
        return 2;
    }

    prm->out_buf_offset = 0;
    prm->file_header_len = 0;

    if(prm->request->version_major == 2)
    {
        prm->processor = handle_http2_receive;
    }
    else
    {
        prm->processor = handle_http_receive;
    }

    prm->expiration_date = _utcTime() + 5;

    return 0;
}

int handle_http_send_not_found(void *arg)
{
    struct handler_prm_t *prm = arg;

    int ret;

    if(prm->file_header_len == 0)
    {
        switch(prm->request->version_major)
        {
            case 2:
            {
                //len
                prm->out_buffer[0] = 0;
                prm->out_buffer[1] = 0;
                prm->out_buffer[2] = 4;

                //type
                prm->out_buffer[3] = HTTP2_HEADERS;

                //flags = END_STREAM & END_HEADERS
                prm->out_buffer[4] = 5;

                //reserved + stream id
                prm->out_buffer[5] = (prm->request->stream_id >> 24) & 0x7F;
                prm->out_buffer[6] = (prm->request->stream_id >> 16) & 0xFF;
                prm->out_buffer[7] = (prm->request->stream_id >> 8) & 0xFF;
                prm->out_buffer[8] = (prm->request->stream_id) & 0xFF;

                //404 not found
                prm->out_buffer[9] = 0x8D;

                //content-length 0
                prm->out_buffer[10] = 0x5C;
                prm->out_buffer[11] = 0x01;
                prm->out_buffer[12] = 0x30;

                prm->file_header_len = 13;
            }
            break;

            case 1:
            {
                prm->file_header_len = sprintf(prm->out_buffer,
                                                "HTTP/%hhu.%hhu 404 Not Found\r\n"\
                                                "Content-Length: 0\r\n"\
                                                "\r\n",
                                                prm->request->version_major, prm->request->version_minor);
            }
            break;

            default: return 1;
        }
    }

    ret = send(prm->sockFD, prm->out_buffer + prm->out_buf_offset,
                            prm->file_header_len - prm->out_buf_offset, 0);

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

    prm->out_buf_offset += ret;

    if(prm->out_buf_offset != prm->file_header_len)
    {
        return 0;
    }

    if(prm->request->version_major < 1)
    {
        return 2;
    }

    if(prm->request->version_major == 1 && prm->request->version_minor == 0)
    {
        return 2;
    }

    prm->out_buf_offset = 0;
    prm->file_header_len = 0;

    if(prm->request->version_major == 2)
    {
        prm->processor = handle_http2_receive;
    }
    else
    {
        prm->processor = handle_http_receive;
    }

    prm->expiration_date = _utcTime() + 5;

    return 0;
}

int handle_http_send_server_error(void *arg)
{
    struct handler_prm_t *prm = arg;

    int ret;

    if(prm->file_header_len == 0)
    {
        switch(prm->request->version_major)
        {
            case 2:
            {
                //len
                prm->out_buffer[0] = 0;
                prm->out_buffer[1] = 0;
                prm->out_buffer[2] = 4;

                //type
                prm->out_buffer[3] = HTTP2_HEADERS;

                //flags = END_STREAM & END_HEADERS
                prm->out_buffer[4] = 5;

                //reserved + stream id
                prm->out_buffer[5] = (prm->request->stream_id >> 24) & 0x7F;
                prm->out_buffer[6] = (prm->request->stream_id >> 16) & 0xFF;
                prm->out_buffer[7] = (prm->request->stream_id >> 8) & 0xFF;
                prm->out_buffer[8] = (prm->request->stream_id) & 0xFF;

                //500 internal server error
                prm->out_buffer[9] = 0x8E;

                //content-length 0
                prm->out_buffer[10] = 0x5C;
                prm->out_buffer[11] = 0x01;
                prm->out_buffer[12] = 0x30;

                prm->file_header_len = 13;
            }
            break;

            case 1:
            {
                prm->file_header_len = sprintf(prm->out_buffer,
                                                "HTTP/%hhu.%hhu 500 Internal Server Error\r\n"\
                                                "Content-Length: 0\r\n"\
                                                "\r\n",
                                                prm->request->version_major, prm->request->version_minor);
            }
            break;

            default: return 1;
        }
    }

    ret = send(prm->sockFD, prm->out_buffer + prm->out_buf_offset,
                            prm->file_header_len - prm->out_buf_offset, 0);

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

    prm->out_buf_offset += ret;

    if(prm->out_buf_offset != prm->file_header_len)
    {
        return 0;
    }

    if(prm->request->version_major < 1)
    {
        return 2;
    }

    if(prm->request->version_major == 1 && prm->request->version_minor == 0)
    {
        return 2;
    }

    prm->out_buf_offset = 0;
    prm->file_header_len = 0;

    if(prm->request->version_major == 2)
    {
        prm->processor = handle_http2_receive;
    }
    else
    {
        prm->processor = handle_http_receive;
    }

    prm->expiration_date = _utcTime() + 5;

    return 0;
}

int handle_http_receive(void *arg)
{
    struct handler_prm_t *prm = arg;

    char *header_end;

    int readRet;

    if(prm->in_buf_offset >= prm->in_buf_len - 1)
    {
        return 1;
    }

    readRet = recv(prm->sockFD, prm->in_buffer + prm->in_buf_offset, prm->in_buf_len - prm->in_buf_offset - 1, 0);

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

    prm->in_buf_offset += readRet;
    prm->in_buffer[prm->in_buf_offset] = '\0';

    header_end = strstr(prm->in_buffer, "\r\n\r\n");

    if(header_end == NULL)
    {
        if(prm->in_buf_offset < prm->in_buf_len - 1)
        {
            return 0;
        }

        logWrite(LOG_TYPE_ERROR, "Request header is too long", 0);

        prm->expiration_date = _utcTime() + 5;

        prm->in_buf_offset = 0;

        prm->processor = handle_http_send_bad_request;

        return prm->processor(arg);
    }

    header_end += MACRO_STRLEN("\r\n\r\n") - 1;
    header_end[0] = '\0';

    prm->header = prm->in_buffer;

    prm->request = parse_http_header(prm->header);
    if(prm->request == NULL)
    {
        prm->expiration_date = _utcTime() + 5;

        prm->in_buf_offset = 0;

        prm->processor = handle_http_send_bad_request;

        return prm->processor(arg);
    }

    if(prm->request->content_length == 0)
    {
        prm->body = header_end;
    }
    else
    {
        prm->body = header_end + 1;

        if(strlen(prm->body) < prm->request->content_length)
        {
            header_end[0] = '\n';

            return 0;
        }
    }

    prm->in_buf_offset = 0;

    prm->expiration_date = _utcTime() + 5;

    logWrite(LOG_TYPE_INFO, "Got request header: %s; body: %s", 2, prm->header, prm->body);

    switch(prm->request->req_type)
    {
        case HTTP_GET: return process_http_get_request(prm, 0);

        case HTTP2_PRISM:
        {
            prm->server_h2settings = http2_server_settings();
            prm->processor = handle_http2_send_settings;
        }
        return prm->processor(prm);

        default: return 1;
    }
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

    cli_prm->in_buf_len = HTTP_MAX_BUFFER_LEN;
    cli_prm->out_buf_len = HTTP_MAX_BUFFER_LEN;
    cli_prm->in_buffer = xmalloc(cli_prm->in_buf_len);
    cli_prm->out_buffer = xmalloc(cli_prm->out_buf_len);
    cli_prm->in_buf_offset = 0;
    cli_prm->out_buf_offset = 0;

    cli_prm->header = NULL;
    cli_prm->body = NULL;

    cli_prm->file_len = 0;
    cli_prm->file_offset = 0;
    cli_prm->file_header_len = 0;
    cli_prm->file_chunk_len = 0;
    cli_prm->file_chunk_loaded = 0;

    cli_prm->critical = 0;
    cli_prm->has_expiration = 1;
    cli_prm->expiration_date = _utcTime() + 5;

    cli_prm->request = NULL;
    cli_prm->client_h2settings = NULL;
    cli_prm->server_h2settings = NULL;

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
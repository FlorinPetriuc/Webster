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

int handle_https_send_page_headers(void *arg)
{
    struct handler_prm_t *prm = arg;

    int ret;
    int errRet;

    const char *mime_type;

    char number_as_char[20];

    unsigned char i;

    logWrite(LOG_TYPE_INFO, "In handle_https_send_page_headers header len is %u",
                                                            1, prm->file_header_len);

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

    logWrite(LOG_TYPE_INFO, "Sending file headers to %p", 1, prm->ssl);

    ERR_clear_error();

    ret = SSL_write(prm->ssl, prm->out_buffer + prm->out_buf_offset, prm->file_header_len - prm->out_buf_offset);

    if(ret <= 0)
    {
        errRet = SSL_get_error(prm->ssl, ret);

        if(errRet == SSL_ERROR_WANT_READ) return 0;
        if(errRet == SSL_ERROR_WANT_WRITE) return 0;

        return 1;
    }

    prm->out_buf_offset += ret;

    if(prm->out_buf_offset < prm->file_header_len)
    {
        return 0;
    }

    logWrite(LOG_TYPE_INFO, "Sent http2 file header", 0);

    prm->out_buf_offset = 0;

    prm->file_header_len = 0;

    if(prm->request->version_major == 2)
    {
        prm->processor = handle_https2_send_page;
    }
    else
    {
        prm->processor = handle_https_send_page;
    }

    return prm->processor(arg);
}

int handle_https_send_page(void *arg)
{
    struct handler_prm_t *prm = arg;

    int ret;
    int rRet;
    int errRet;

    if(prm->file_len - prm->file_offset == 0)
    {
        goto out_empty_file;
    }

    while(prm->file_offset != prm->file_len)
    {
        if(prm->out_buf_len - prm->out_buf_offset >= prm->file_len - prm->file_offset)
        {
            rRet = read(prm->fileFD, prm->out_buffer + prm->out_buf_offset, prm->file_len - prm->file_offset);
        }
        else
        {
            rRet = read(prm->fileFD, prm->out_buffer + prm->out_buf_offset, prm->out_buf_len - prm->out_buf_offset);
        }

        if(rRet < 0)
        {
            switch(errno)
            {
                case EAGAIN:
                {
                    if(prm->out_buf_offset) break;
                }
                return 0;

                case EINTR:
                {
                    if(prm->out_buf_offset) break;
                }
                return 0;

                default: return 1;
            }

            rRet = 0;
        }

        if(rRet == 0 && prm->out_buf_offset == 0)
        {
            return 0;
        }

        prm->file_offset += rRet;
        prm->out_buf_offset += rRet;

        ERR_clear_error();

        ret = SSL_write(prm->ssl, prm->out_buffer, prm->out_buf_offset);

        if(ret <= 0)
        {
            errRet = SSL_get_error(prm->ssl, ret);

            if(errRet == SSL_ERROR_WANT_READ) return 0;
            if(errRet == SSL_ERROR_WANT_WRITE) return 0;

            return 1;
        }

        prm->expiration_date = _utcTime() + 5;

        if(ret > prm->out_buf_offset)
        {
            return 1;
        }

        if(prm->out_buf_offset == ret)
        {
            prm->out_buf_offset = 0;

            continue;
        }

        prm->out_buf_offset -= ret;

        memcpy(prm->out_buffer, prm->out_buffer + ret, prm->out_buf_offset);
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

    prm->out_buf_offset = 0;

    free(prm->request->abs_path);
    free(prm->request);
    prm->request = NULL;

    prm->file_len = 0;
    prm->file_offset = 0;

    prm->processor = handle_https_receive;

    return 0;
}

int handle_https_send_bad_request(void *arg)
{
    struct handler_prm_t *prm = arg;

    int ret;
    int errRet;

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

    ERR_clear_error();

    ret = SSL_write(prm->ssl, prm->out_buffer + prm->out_buf_offset,
                                        prm->file_header_len - prm->out_buf_offset);

    if(ret <= 0)
    {
        errRet = SSL_get_error(prm->ssl, ret);

        if(errRet == SSL_ERROR_WANT_READ) return 0;
        if(errRet == SSL_ERROR_WANT_WRITE) return 0;

        return 1;
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
        prm->processor = handle_https2_receive;
    }
    else
    {
        prm->processor = handle_https_receive;
    }

    prm->expiration_date = _utcTime() + 5;

    return 0;
}

int handle_https_send_not_found(void *arg)
{
    struct handler_prm_t *prm = arg;

    int ret;
    int errRet;

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

    ERR_clear_error();

    ret = SSL_write(prm->ssl, prm->out_buffer + prm->out_buf_offset,
                                        prm->file_header_len - prm->out_buf_offset);

    if(ret <= 0)
    {
        errRet = SSL_get_error(prm->ssl, ret);

        if(errRet == SSL_ERROR_WANT_READ) return 0;
        if(errRet == SSL_ERROR_WANT_WRITE) return 0;

        return 1;
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
        prm->processor = handle_https2_receive;
    }
    else
    {
        prm->processor = handle_https_receive;
    }

    prm->expiration_date = _utcTime() + 5;

    return 0;
}

int handle_https_send_server_error(void *arg)
{
    struct handler_prm_t *prm = arg;

    int ret;
    int errRet;

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

    ERR_clear_error();

    ret = SSL_write(prm->ssl, prm->out_buffer + prm->out_buf_offset,
                                        prm->file_header_len - prm->out_buf_offset);

    if(ret <= 0)
    {
        errRet = SSL_get_error(prm->ssl, ret);

        if(errRet == SSL_ERROR_WANT_READ) return 0;
        if(errRet == SSL_ERROR_WANT_WRITE) return 0;

        return 1;
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
        prm->processor = handle_https2_receive;
    }
    else
    {
        prm->processor = handle_https_receive;
    }

    prm->expiration_date = _utcTime() + 5;

    return 0;
}

int handle_https_receive(void *arg)
{
    struct handler_prm_t *prm = arg;

    char *header_end;

    struct stat buf;

    int readRet;
    int len;
    int errRet;

    if(prm->in_buf_offset == prm->in_buf_len - 1)
    {
        return 1;
    }

    ERR_clear_error();

    readRet = SSL_read(prm->ssl, prm->in_buffer + prm->in_buf_offset, prm->in_buf_len - prm->in_buf_offset - 1);

    if(readRet <= 0)
    {
        errRet = SSL_get_error(prm->ssl, readRet);

        if(errRet == SSL_ERROR_WANT_READ) return 0;
        if(errRet == SSL_ERROR_WANT_WRITE) return 0;

        return 1;
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

        prm->processor = handle_https_send_bad_request;

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

        prm->processor = handle_https_send_bad_request;

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
        case HTTP_GET: return process_http_get_request(prm, 1);

        case HTTP2_PRISM:
        {
            prm->server_h2settings = http2_server_settings();
            prm->processor = handle_https2_send_settings;
        }
        return prm->processor(prm);

        default: return 1;
    }
}

int handle_https_accept(void *arg)
{
    struct handler_prm_t *prm = arg;

    int accept_ret;
    int err_ret;

    if(prm->ssl == NULL)
    {
        prm->ssl = ssl_encap_connection(prm->sockFD, prm->certificate);

        if(prm->ssl == NULL)
        {
            return 1;
        }
    }

    ERR_clear_error();

    accept_ret = SSL_accept(prm->ssl);

    if(accept_ret > 0)
    {
        prm->expiration_date = _utcTime() + 5;
        prm->processor = handle_https_receive;

        return prm->processor(arg);
    }

    if(errno == EAGAIN || errno == EINTR ||
       errno == EWOULDBLOCK)
    {
        return 0;
    }

    err_ret = SSL_get_error(prm->ssl, accept_ret);

    if(err_ret == SSL_ERROR_WANT_READ || err_ret == SSL_ERROR_WANT_WRITE ||
       err_ret == SSL_ERROR_WANT_ACCEPT)
    {
        return 0;
    }

    logWrite(LOG_TYPE_ERROR, "ssl accept failed", 0);
    log_ssl_errors();

    return 1;
}
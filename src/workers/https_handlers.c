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

int handle_https_send_page(void *arg)
{
    struct handler_prm_t *prm = arg;

    int ret;
    int rRet;
    int errRet;

    const char *mime_type;

    logWrite(LOG_TYPE_INFO, "In handle_https_send_page prm is %p", 1, prm);

    if(prm->file_header_sent == 0)
    {
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

            prm->file_header_len = sprintf(prm->buffer, "HTTP/1.1 200 OK\r\n"
                                                        "Content-type: %s\r\n"
                                                        "Content-Length: %u\r\n"
                                                        "\r\n", mime_type, prm->file_len);
        }

        ERR_clear_error();

        ret = SSL_write(prm->ssl, prm->buffer + prm->buf_offset, prm->file_header_len - prm->buf_offset);

        if(ret <= 0)
        {
            errRet = SSL_get_error(prm->ssl, ret);

            if(errRet == SSL_ERROR_WANT_READ) return 0;
            if(errRet == SSL_ERROR_WANT_WRITE) return 0;

            return 1;
        }

        prm->buf_offset += ret;

        if(prm->buf_offset < prm->file_header_len)
        {
            return 0;
        }

        prm->buf_offset = 0;
        prm->file_offset = 0;
        prm->file_header_sent = 1;

        logWrite(LOG_TYPE_INFO, "File header sent", 0);
    }

    while(prm->file_offset != prm->file_len)
    {
        if(prm->buf_len - prm->buf_offset >= prm->file_len - prm->file_offset)
        {
            rRet = read(prm->fileFD, prm->buffer + prm->buf_offset, prm->file_len - prm->file_offset);
        }
        else
        {
            rRet = read(prm->fileFD, prm->buffer + prm->buf_offset, prm->buf_len - prm->buf_offset);
        }

        if(rRet < 0)
        {
            switch(errno)
            {
                case EAGAIN:
                {
                    if(prm->buf_offset) break;
                }
                return 0;

                case EINTR:
                {
                    if(prm->buf_offset) break;
                }
                return 0;

                default: return 1;
            }

            rRet = 0;
        }

        if(rRet == 0 && prm->buf_offset == 0)
        {
            return 0;
        }

        prm->file_offset += rRet;
        prm->buf_offset += rRet;

        ERR_clear_error();

        ret = SSL_write(prm->ssl, prm->buffer, prm->buf_offset);

        if(ret <= 0)
        {
            errRet = SSL_get_error(prm->ssl, ret);

            if(errRet == SSL_ERROR_WANT_READ) return 0;
            if(errRet == SSL_ERROR_WANT_WRITE) return 0;

            return 1;
        }

        prm->expiration_date = _utcTime() + 5;

        if(ret > prm->buf_offset)
        {
            return 1;
        }

        if(prm->buf_offset == ret)
        {
            prm->buf_offset = 0;

            continue;
        }

        prm->buf_offset -= ret;

        memcpy(prm->buffer, prm->buffer + ret, prm->buf_offset);
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

    prm->buf_offset = 0;

    free(prm->request->abs_path);
    free(prm->request);
    prm->request = NULL;

    prm->processor = handle_https_receive;

    return 0;
}

int handle_https_send_bad_request(void *arg)
{
    const char *bad_request = HTTP_BAD_REQUEST;
    const int bad_request_len = MACRO_STRLEN(HTTP_BAD_REQUEST);

    struct handler_prm_t *prm = arg;

    int ret;
    int errRet;

    if(prm->buf_offset == bad_request_len)
    {
        return 2;
    }

    ERR_clear_error();

    ret = SSL_write(prm->ssl, bad_request + prm->buf_offset, bad_request_len - prm->buf_offset);

    if(ret <= 0)
    {
        errRet = SSL_get_error(prm->ssl, ret);

        if(errRet == SSL_ERROR_WANT_READ) return 0;
        if(errRet == SSL_ERROR_WANT_WRITE) return 0;

        return 1;
    }

    prm->buf_offset += ret;

    if(prm->buf_offset == bad_request_len)
    {
        return 2;
    }

    return 0;
}

int handle_https_send_not_found(void *arg)
{
    const char *bad_request = HTTP_NOT_FOUND;
    const int bad_request_len = MACRO_STRLEN(HTTP_NOT_FOUND);

    struct handler_prm_t *prm = arg;

    int ret;
    int errRet;

    if(prm->buf_offset == bad_request_len)
    {
        return 2;
    }

    ERR_clear_error();

    ret = SSL_write(prm->ssl, bad_request + prm->buf_offset, bad_request_len - prm->buf_offset);

    if(ret <= 0)
    {
        errRet = SSL_get_error(prm->ssl, ret);

        if(errRet == SSL_ERROR_WANT_READ) return 0;
        if(errRet == SSL_ERROR_WANT_WRITE) return 0;

        return 1;
    }

    prm->buf_offset += ret;

    if(prm->buf_offset == bad_request_len)
    {
        return 2;
    }

    return 0;
}

int handle_https_send_server_error(void *arg)
{
    const char *bad_request = HTTP_SERVER_ERROR;
    const int bad_request_len = MACRO_STRLEN(HTTP_SERVER_ERROR);

    struct handler_prm_t *prm = arg;

    int ret;
    int errRet;

    if(prm->buf_offset == bad_request_len)
    {
        return 2;
    }

    ERR_clear_error();

    ret = SSL_write(prm->ssl, bad_request + prm->buf_offset, bad_request_len - prm->buf_offset);

    if(ret <= 0)
    {
        errRet = SSL_get_error(prm->ssl, ret);

        if(errRet == SSL_ERROR_WANT_READ) return 0;
        if(errRet == SSL_ERROR_WANT_WRITE) return 0;

        return 1;
    }

    prm->buf_offset += ret;

    if(prm->buf_offset == bad_request_len)
    {
        return 2;
    }

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

    if(prm->buf_offset == prm->buf_len - 1)
    {
        return 1;
    }

    ERR_clear_error();

    readRet = SSL_read(prm->ssl, prm->buffer + prm->buf_offset, prm->buf_len - prm->buf_offset - 1);

    if(readRet <= 0)
    {
        errRet = SSL_get_error(prm->ssl, readRet);

        if(errRet == SSL_ERROR_WANT_READ) return 0;
        if(errRet == SSL_ERROR_WANT_WRITE) return 0;

        return 1;
    }

    prm->buf_offset += readRet;
    prm->buffer[prm->buf_offset] = '\0';

    header_end = strstr(prm->buffer, "\r\n\r\n");

    if(header_end == NULL)
    {
        if(prm->buf_offset < prm->buf_len - 1)
        {
            return 0;
        }

        logWrite(LOG_TYPE_ERROR, "Request header is too long", 0);

        prm->expiration_date = _utcTime() + 5;

        prm->buf_offset = 0;

        prm->processor = handle_https_send_bad_request;

        return 0;
    }

    header_end += MACRO_STRLEN("\r\n\r\n");
    header_end[0] = '\0';

    logWrite(LOG_TYPE_INFO, "Got request header: %s", 1, prm->buffer);

    prm->expiration_date = _utcTime() + 5;

    prm->buf_offset = 0;
    prm->file_header_sent = 0;

    prm->request = parse_http_header(prm->buffer);
    if(prm->request == NULL)
    {
        prm->processor = handle_https_send_bad_request;

        return 0;
    }

    prm->fileFD = open(prm->request->abs_path, O_RDONLY);

    if(prm->fileFD < 0)
    {
        prm->processor = handle_https_send_not_found;

        return 0;
    }

    if(fstat(prm->fileFD, &buf) != 0)
    {
        close(prm->fileFD);
        prm->fileFD = -1;

        prm->processor = handle_https_send_server_error;

        return 0;
    }

    if(!(S_ISDIR(buf.st_mode)))
    {
        prm->file_len = buf.st_size;
        prm->file_header_len = 0;

        prm->processor = handle_https_send_page;

        return 0;
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
        prm->processor = handle_https_send_not_found;

        return 0;
    }

    if(fstat(prm->fileFD, &buf) != 0)
    {
        close(prm->fileFD);
        prm->fileFD = -1;

        prm->processor = handle_https_send_server_error;

        return 0;
    }

    if(S_ISDIR(buf.st_mode))
    {
        close(prm->fileFD);
        prm->fileFD = -1;

        prm->processor = handle_https_send_not_found;

        return 0;
    }

    prm->file_len = buf.st_size;
    prm->file_header_len = 0;

    prm->processor = handle_https_send_page;

    return 0;
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

        return 0;
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
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

#include "../../main.h"

static const char *working_directory = ".";
static int working_directory_len = 1;

void http_set_working_directory(const char *path)
{
    if(path == NULL)
    {
        working_directory = ".";
        working_directory_len = 1;

        logWrite(LOG_TYPE_INFO, "Working directory is %s", 1, working_directory);

        return;
    }

    working_directory = path;
    working_directory_len = strlen(working_directory);

    logWrite(LOG_TYPE_INFO, "Working directory is %s", 1, working_directory);
}

static char *http_resolve_path_and_version(const char *header, unsigned char *version_major,
                                                                unsigned char *version_minor)
{
    unsigned int i = 0;
    unsigned int j = 0;

    unsigned int length;

    int depth = 0;

    char *ret;

    //GET <path> HTTP/X.X
    while(header[i] != ' ')
    {
        if(header[i] == '\0')
        {
            return NULL;
        }

        ++i;
    }

    ++i;
    j = i;

    //<path> HTTP/X.X
    while(header[j] != ' ')
    {
        if(header[j] == '\0')
        {
            return NULL;
        }

        if(header[j] == '/')
        {
            ++depth;

             ++j;

            if(header[j] == '/')
            {
                return NULL;
            }

            continue;
        }

        if(header[j] != '.')
        {
            ++j;

            continue;
        }

        ++j;

        if(header[j] != '.')
        {
            continue;
        }

        depth -= 2;

        if(depth < 0)
        {
            return NULL;
        }

        ++j;
    }

    if(sscanf(header + j + 1, "HTTP/%hhu.%hhu", version_major, version_minor) != 2)
    {
        return NULL;
    }

    length = j - i;

    ret = xmalloc(working_directory_len + length + 1);
    memcpy(ret, working_directory, working_directory_len);

    j = working_directory_len;

    length += i;

    while(i < length)
    {
        if(header[i] != '%')
        {
            ret[j] = header[i];

            ++j;
            ++i;

            continue;
        }

        ++i;

        if(header[i] == '%')
        {
            ret[j] = '%';

            ++j;
            ++i;

            continue;
        }

        if(header[i] >= '0' && header[i] <= '9')
        {
            ret[j] = (header[i] - '0') << 4;
        }
        else if(header[i] >= 'a' && header[i] <= 'f')
        {
            ret[j] = (header[i] - 'a' + 10) << 4;
        }
        else if(header[i] >= 'A' && header[i] <= 'F')
        {
            ret[j] = (header[i] - 'A' + 10) << 4;
        }

        ++i;

        if(header[i] >= '0' && header[i] <= '9')
        {
            ret[j] += header[i] - '0';
        }
        else if(header[i] >= 'a' && header[i] <= 'f')
        {
            ret[j] += header[i] - 'a' + 10;
        }
        else if(header[i] >= 'A' && header[i] <= 'F')
        {
            ret[j] += header[i] - 'A' + 10;
        }

        ++j;
        ++i;
    }

    ret[j] = '\0';

    return ret;
}

static enum http_request_type_t http_resolve_method(const char *header)
{
    if(string_starts_with(header, "GET ") == 0)
    {
        return HTTP_GET;
    }
    else if(string_starts_with(header, "PRI ") == 0)
    {
        return HTTP2_PRISM;
    }
    else
    {
        return HTTP_UNSUPPORTED_METHOD;
    }
}

static unsigned int http_resolve_content_len(const char *header)
{
    unsigned int ret = 0;

    const char *content_len_header;

    content_len_header = strstr(header, "Content-Length: ");

    if(content_len_header == NULL)
    {
        if(string_starts_with(header, "PRI ") == 0)
        {
            return 6;
        }

        return 0;
    }

    content_len_header += MACRO_STRLEN("Content-Length: ");

    if(sscanf(content_len_header, "%u", &ret) != 1)
    {
        return 0;
    }

    return ret;
}

struct http_request_t *parse_http_header(const char *header)
{
    struct http_request_t *ret;

    char *path;

    unsigned char version_major = 0;
    unsigned char version_minor = 0;

    unsigned int content_length = 0;

    enum http_request_type_t req_type;

    req_type = http_resolve_method(header);

    if(req_type == HTTP_UNSUPPORTED_METHOD)
    {
        logWrite(LOG_TYPE_ERROR, "Unsupported request", 0);

        return NULL;
    }

    content_length = http_resolve_content_len(header);

    path = http_resolve_path_and_version(header, &version_major, &version_minor);

    if(path == NULL)
    {
        logWrite(LOG_TYPE_ERROR, "Could not solve http path or version", 0);

        return NULL;
    }

    ret = xmalloc(sizeof(struct http_request_t));
    ret->req_type = req_type;

    ret->abs_path = path;

    ret->content_length = content_length;

    ret->version_major = version_major;
    ret->version_minor = version_minor;

    logWrite(LOG_TYPE_INFO, "Header resolved to req:%d path %s, version: %hhu.%hhu", 4,
                                            req_type, path, version_major, version_minor);

    return ret;
}

int process_http_get_request(struct handler_prm_t *prm, const unsigned char encrypted)
{
    int len;

    struct stat buf;

    prm->fileFD = open(prm->request->abs_path, O_RDONLY);

    if(prm->fileFD < 0)
    {
        if(encrypted)
        {
            prm->processor = handle_https_send_not_found;
        }
        else
        {
            prm->processor = handle_http_send_not_found;
        }

        return prm->processor(prm);
    }

    if(fstat(prm->fileFD, &buf) != 0)
    {
        close(prm->fileFD);
        prm->fileFD = -1;

        if(encrypted)
        {
            prm->processor = handle_https_send_server_error;
        }
        else
        {
            prm->processor = handle_http_send_server_error;
        }

        return prm->processor(prm);
    }

    if(!(S_ISDIR(buf.st_mode)))
    {
        prm->file_len = buf.st_size;

        if(encrypted)
        {
            prm->processor = handle_https_send_page_headers;
        }
        else
        {
            prm->processor = handle_http_send_page_headers;
        }

        return prm->processor(prm);
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
        if(encrypted)
        {
            prm->processor = handle_https_send_not_found;
        }
        else
        {
            prm->processor = handle_http_send_not_found;
        }

        return prm->processor(prm);
    }

    if(fstat(prm->fileFD, &buf) != 0)
    {
        close(prm->fileFD);
        prm->fileFD = -1;

        if(encrypted)
        {
            prm->processor = handle_https_send_server_error;
        }
        else
        {
            prm->processor = handle_http_send_server_error;
        }

        return prm->processor(prm);
    }

    if(S_ISDIR(buf.st_mode))
    {
        close(prm->fileFD);
        prm->fileFD = -1;

        if(encrypted)
        {
            prm->processor = handle_https_send_not_found;
        }
        else
        {
            prm->processor = handle_http_send_not_found;
        }

        return prm->processor(prm);
    }

    prm->file_len = buf.st_size;
    prm->file_header_len = 0;

    if(encrypted)
    {
        prm->processor = handle_https_send_page_headers;
    }
    else
    {
        prm->processor = handle_http_send_page_headers;
    }

    return prm->processor(prm);
}
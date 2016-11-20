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

int handle_https_receive(void *arg)
{
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
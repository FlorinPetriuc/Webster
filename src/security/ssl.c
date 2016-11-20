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

static pthread_mutex_t *ssl_mtx = NULL;
static int mtx_nr = 0;

static unsigned long thread_id_callback()
{
    return ((unsigned long) pthread_self());
}

static void lock_callback(int mode, int n, const char *file, int line)
{
    if(n >= mtx_nr)
    {
        return;
    }

    if(mode & CRYPTO_LOCK)
    {
        TAKE_MUTEX(ssl_mtx[n]);
    }
    else
    {
        RELEASE_MUTEX(ssl_mtx[n]);
    }
}

int ssl_init()
{
    int i;

    SSL_library_init();
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();

    mtx_nr = CRYPTO_num_locks();

    ssl_mtx = xmalloc(mtx_nr * sizeof(pthread_mutex_t));

    for (i = 0; i < mtx_nr; ++i)
    {
        INIT_MUTEX(ssl_mtx[i]);
    }

    CRYPTO_set_id_callback(thread_id_callback);
    CRYPTO_set_locking_callback(lock_callback);

    return 0;
}

SSL *ssl_encap_connection(const int fd, const char *certificate)
{
    SSL *ret = NULL;

    SSL_CTX *ctx = NULL;

    if(certificate == NULL)
    {
        logWrite(LOG_TYPE_ERROR, "Secure connection with no certificate", 0);

        return NULL;
    }

    ctx = SSL_CTX_new(TLSv1_2_method());

    if(ctx == NULL)
    {
        logWrite(LOG_TYPE_ERROR, "SSL_CTX_new with TLSv1_2_method returns NULL", 0);
        log_ssl_errors();

        return NULL;
    }

    if(SSL_CTX_use_certificate_file(ctx, certificate, SSL_FILETYPE_PEM) < 0)
    {
        logWrite(LOG_TYPE_ERROR, "SSL_CTX_use_certificate_file with %s failed: %s",
                                                        2, certificate, strerror(errno));
        log_ssl_errors();

        goto out_err;
    }

    if(SSL_CTX_use_PrivateKey_file(ctx, certificate, SSL_FILETYPE_PEM) < 0)
    {
        logWrite(LOG_TYPE_ERROR, "SSL_CTX_use_PrivateKey_file with %s failed: %s",
                                                        2, certificate, strerror(errno));
        log_ssl_errors();

        goto out_err;
    }

    if(!SSL_CTX_check_private_key(ctx))
    {
        logWrite(LOG_TYPE_ERROR, "SSL_CTX_check_private_key with %s failed: %s",
                                                        2, certificate, strerror(errno));
        log_ssl_errors();

        goto out_err;
    }

    if(SSL_CTX_use_certificate_chain_file(ctx, certificate) < 0)
    {
        logWrite(LOG_TYPE_ERROR, "SSL_CTX_use_certificate_chain_file with %s failed: %s",
                                                        2, certificate, strerror(errno));
        log_ssl_errors();

        goto out_err;
    }

    ret = SSL_new(ctx);
    if(ret == NULL)
    {
        logWrite(LOG_TYPE_ERROR, "SSL_new returns NULL", 0);
        log_ssl_errors();

        goto out_err;
    }

    if(SSL_set_fd(ret, fd) == 0)
    {
        logWrite(LOG_TYPE_ERROR, "SSL_set_fd returns 0", 0);
        log_ssl_errors();

        goto out_err;
    }

    return ret;

out_err:
    if(ctx)
    {
        SSL_CTX_free(ctx);
    }

    if(ret)
    {
        SSL_free(ret);
    }

    return NULL;
}

void close_secure_connection(SSL *ssl)
{
    int fd = SSL_get_fd(ssl);
    SSL_CTX *ctx = SSL_get_SSL_CTX(ssl);

    SSL_free(ssl);
    SSL_CTX_free(ctx);

    close(fd);
}
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

void close_secure_connection(SSL *ssl)
{
    int fd = SSL_get_fd(ssl);
    SSL_CTX *ctx = SSL_get_SSL_CTX(ssl);

    SSL_free(ssl);
    SSL_CTX_free(ctx);

    close(fd);
}
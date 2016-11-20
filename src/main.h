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

#ifndef _MAIN_H_
#define _MAIN_H_

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <stdarg.h>
#include <pthread.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/sendfile.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <sys/epoll.h>

#include "openssl/ssl.h"
#include "openssl/err.h"

#include "version.h"

#include "debug/debug.h"
#include "server/server.h"
#include "misc/misc.h"
#include "security/ssl.h"
#include "protocol/http/http.h"
#include "log/log.h"
#include "workers/handlers.h"
#include "workers/http_handlers.h"
#include "workers/https_handlers.h"
#include "workers/pool.h"

#endif
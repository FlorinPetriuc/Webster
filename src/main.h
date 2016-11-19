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

#include <sys/socket.h>
#include <netinet/in.h>

#include <sys/epoll.h>

#include "openssl/ssl.h"
#include "openssl/err.h"
#include "openssl/sha.h"

#include "version.h"

#include "debug/debug.h"
#include "server/server.h"
#include "misc/misc.h"
#include "protocol/http/http.h"
#include "log/log.h"
#include "workers/handlers.h"
#include "workers/http_handlers.h"
#include "workers/pool.h"

#endif
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

#ifndef _DEBUG_H_
#define _DEBUG_H_

//#define BUG_CHECK(X)

#ifndef BUG_CHECK
#define BUG_CHECK(X) \
    if(X) \
    { \
        logWrite(LOG_TYPE_FATAL, "BUG DETECTED in file %s, line %d", 2, __FILE__, __LINE__); \
        exit(EXIT_FAILURE); \
    }
#endif

#endif
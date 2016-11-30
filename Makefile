#
# Copyright (C) 2016 Florin Petriuc. All rights reserved.
# Initial release: Florin Petriuc <petriuc.florin@gmail.com>
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License version
# 2 as published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
#

CFLG=$(CFLAGS) -c -Werror
BFILES=./build/obj/ssl.o ./build/obj/log.o ./build/obj/misc.o ./build/obj/http.o ./build/obj/http2.o ./build/obj/server.o ./build/obj/http_handlers.o ./build/obj/https_handlers.o ./build/obj/http2_handlers.o ./build/obj/https2_handlers.o ./build/obj/pool.o
OFILES=$(BFILES) ./build/obj/main.o

.PHONY: all
all: directories program

directories:
	mkdir -p ./build
	mkdir -p ./build/obj
	mkdir -p ./build/bin
	cp ./build/items/* ./build/bin

program: $(OFILES)
	$(CC) $(LDFLAGS) $(OFILES) -o ./build/bin/webster -lpthread -lssl -lcrypto

./build/obj/log.o: ./src/log/log.c
	$(CC) $(CFLG) ./src/log/log.c -o ./build/obj/log.o

./build/obj/ssl.o: ./src/security/ssl.c
	$(CC) $(CFLG) ./src/security/ssl.c -o ./build/obj/ssl.o

./build/obj/http.o: ./src/protocol/http/http.c
	$(CC) $(CFLG) ./src/protocol/http/http.c -o ./build/obj/http.o

./build/obj/http2.o: ./src/protocol/http/http2.c
	$(CC) $(CFLG) ./src/protocol/http/http2.c -o ./build/obj/http2.o

./build/obj/misc.o: ./src/misc/misc.c
	$(CC) $(CFLG) ./src/misc/misc.c -o ./build/obj/misc.o

./build/obj/main.o: ./src/main.c
	$(CC) $(CFLG) ./src/main.c -o ./build/obj/main.o

./build/obj/server.o: ./src/server/server.c
	$(CC) $(CFLG) ./src/server/server.c -o ./build/obj/server.o

./build/obj/http_handlers.o: ./src/workers/http_handlers.c
	$(CC) $(CFLG) ./src/workers/http_handlers.c -o ./build/obj/http_handlers.o

./build/obj/https_handlers.o: ./src/workers/https_handlers.c
	$(CC) $(CFLG) ./src/workers/https_handlers.c -o ./build/obj/https_handlers.o

./build/obj/http2_handlers.o: ./src/workers/http2_handlers.c
	$(CC) $(CFLG) ./src/workers/http2_handlers.c -o ./build/obj/http2_handlers.o

./build/obj/https2_handlers.o: ./src/workers/https2_handlers.c
	$(CC) $(CFLG) ./src/workers/https2_handlers.c -o ./build/obj/https2_handlers.o

./build/obj/pool.o: ./src/workers/pool.c
	$(CC) $(CFLG) ./src/workers/pool.c -o ./build/obj/pool.o

clean: 
	rm ./build/obj/* 
	rm ./build/bin/*
CFLG=$(CFLAGS) -c -Werror
BFILES=./build/obj/log.o ./build/obj/misc.o ./build/obj/server.o ./build/obj/http_handlers.o ./build/obj/pool.o
OFILES=$(BFILES) ./build/obj/main.o

.PHONY: all
all: $(OFILES)
	$(CC) $(LDFLAGS) $(OFILES) -o ./build/bin/webster -lpthread -lssl -lcrypto -ldl

./build/obj/log.o: ./src/log/log.c
	$(CC) $(CFLG) ./src/log/log.c -o ./build/obj/log.o

./build/obj/misc.o: ./src/misc/misc.c
	$(CC) $(CFLG) ./src/misc/misc.c -o ./build/obj/misc.o

./build/obj/main.o: ./src/main.c
	$(CC) $(CFLG) ./src/main.c -o ./build/obj/main.o

./build/obj/server.o: ./src/server/server.c
	$(CC) $(CFLG) ./src/server/server.c -o ./build/obj/server.o

./build/obj/http_handlers.o: ./src/workers/http_handlers.c
	$(CC) $(CFLG) ./src/workers/http_handlers.c -o ./build/obj/http_handlers.o

./build/obj/pool.o: ./src/workers/pool.c
	$(CC) $(CFLG) ./src/workers/pool.c -o ./build/obj/pool.o

clean: 
	rm ./build/obj/* 
	rm ./build/bin/*
CFLAGS=-std=gnu99 -Wall -pthread
LIBS=-lm

all: client server

client: client.c communication.h
	gcc $(CFLAGS) -o client client.c $(LIBS)
server: server.c communication.h
	gcc $(CFLAGS) -o server server.c $(LIBS)

clean:
	rm -f client server

.PHONY: all client server clean

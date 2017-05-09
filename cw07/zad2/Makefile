CFLAGS=-std=gnu99 -Wall -lrt -pthread

all: clients barber

clients: clients.c fifo.c util.c communication.h
	gcc $(CFLAGS) -o clients clients.c
barber: barber.c fifo.c util.c communication.h
	gcc $(CFLAGS) -o barber barber.c

clean:
	rm -f clients barber

.PHONY: all clients barber clean

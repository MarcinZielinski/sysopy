CFLAGS=-std=gnu99 -Wall

all: clients barber

clients: clients.c fifo.c semaphores.c util.c communication.h
	gcc $(CFLAGS) -o clients clients.c
barber: barber.c fifo.c semaphores.c util.c communication.h
	gcc $(CFLAGS) -o barber barber.c

clean:
	rm -f clients barber

.PHONY: all clients barber clean

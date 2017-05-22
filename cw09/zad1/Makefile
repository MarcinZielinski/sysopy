CFLAGS=-std=gnu99 -Wall -pthread

all: fifo_sync starving_writers

fifo_sync: fifo_sync.c fifo_sync.h
	gcc $(CFLAGS) -o fifo_sync fifo_sync.c
starving_writers: starving_writers.c starving_writers.h
	gcc $(CFLAGS) -o starving_writers starving_writers.c
clean:
	rm -f fifo_sync starving_writers

.PHONY: all fifo_sync starving_writers clean

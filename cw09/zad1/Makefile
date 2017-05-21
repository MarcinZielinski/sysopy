CFLAGS=-std=gnu99 -Wall -pthread

all: fifo_sync starving_readers

fifo_sync: fifo_sync.c fifo_sync.h
	gcc $(CFLAGS) -o fifo_sync fifo_sync.c
starving_readers: starving_readers.c starving_readers.h
	gcc $(CFLAGS) -o starving_readers starving_readers.c
clean:
	rm -f fifo_sync starving_readers

.PHONY: all fifo_sync starving_readers clean

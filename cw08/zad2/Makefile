CFLAGS=-std=gnu99 -Wall -pthread -Wdiv-by-zero
LIBS=-lm

all: main_v1 main_v2 main_v3 main_v4 main_v5 main_v6

main_v1: main_v1.c finals.h
	gcc $(CFLAGS) -o main_v1 main_v1.c $(LIBS)
main_v2: main_v2.c finals.h
	gcc $(CFLAGS) -o main_v2 main_v2.c $(LIBS)
main_v3: main_v3.c finals.h
	gcc $(CFLAGS) -o main_v3 main_v3.c $(LIBS)
main_v4: main_v4.c finals.h
	gcc $(CFLAGS) -o main_v4 main_v4.c $(LIBS)
main_v5: main_v5.c finals.h
	gcc $(CFLAGS) -o main_v5 main_v5.c $(LIBS)
main_v6: main_v6.c finals.h
	gcc $(CFLAGS) -o main_v6 main_v6.c $(LIBS)
generator: file_generator.c finals.h
	gcc $(CFLAGS) -o file_generator file_generator.c $(LIBS)

clean:
	rm -f main_v1 main_v2 main_v3 main_v4 main_v5 main_v6 file_generator

.PHONY: all main_v1 main_v2 main_v3 main_v4 main_v5 main_v6 generator clean

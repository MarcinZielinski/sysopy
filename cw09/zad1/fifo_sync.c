#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "fifo.c"
#include "fifo_sync.h"

int R, W; // R - number of readers, W - number of writers
int verbose;
pthread_t *go_ahead;
struct fifo* fifo;

void exit_handler() {
    fifo_destroy(fifo);
}

void reading() {
    // TODO: reader stuff
}

void writing() {
    // TODO: writer stuff
}

void *reader_handler(void *args) {

    return 0;
}

void *writer_handler(void *args) {

    return 0;
}

int main(int argc, char **argv) {
    atexit(exit_handler);

    if(argc == 2) {
        if(strcmp(argv[2],"-i") == 0) {
            verbose = 1;
        }
    }

    R = 5; // TODO: change it to
    W = 2; // TODO: some rand();

    go_ahead = malloc(sizeof(pthread_t) * (R+W));

    fifo = fifo_init();

    pthread_t tid;
    pthread_attr_t attr;
    struct reader_args args;

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED);

    for(int i = 0;i<R;++i) {
        args.id = i;
        args.divider = 4; // TODO: insert rand() here
        pthread_create(&tid,&attr,reader_handler,&args);
    }
    for(int i = R;i<(W+R);++i) {
        int id = i;
        pthread_create(&tid,&attr,writer_handler,&id);
    }

    return 0;
}
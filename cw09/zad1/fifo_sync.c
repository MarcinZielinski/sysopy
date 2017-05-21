#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include "fifo.c"
#include "fifo_sync.h"

int R, W; // R - number of readers, W - number of writers
int verbose;
int shutdown;
pthread_t *go_ahead;
struct fifo* fifo;
pthread_mutex_t parent, mutex, end_of_work_mutex, fifo_mutex;
pthread_cond_t end_of_work_cond, cond, parent_cond;
int readers, writer_busy, go_parent;

void init_mutexes_and_conds() {
    mutex = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
    end_of_work_mutex = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
    parent = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
    fifo_mutex = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
    cond = (pthread_cond_t)PTHREAD_COND_INITIALIZER;
    end_of_work_cond = (pthread_cond_t)PTHREAD_COND_INITIALIZER;
    parent_cond = (pthread_cond_t)PTHREAD_COND_INITIALIZER;
}

int destroy_mutexes_and_conds() {
    int res = 0;
    res+=pthread_mutex_destroy(&mutex);
    res+=pthread_mutex_destroy(&end_of_work_mutex);
    res+=pthread_mutex_destroy(&parent);
    res+=pthread_mutex_destroy(&fifo_mutex);
    res+=pthread_cond_destroy(&cond);
    res+=pthread_cond_destroy(&end_of_work_cond);
    res+=pthread_cond_destroy(&parent_cond);
    return res;
}

void exit_program(int status, char *msg) {
    if(status == EXIT_FAILURE) {
        perror(msg);
    } else {
        printf("%s\n",msg);
    }
    exit(status);
}

void exit_handler() {
    fifo_destroy(fifo);
    if(destroy_mutexes_and_conds()) {
        fprintf(stderr,"Error while destroying mutexes and conditional variables");
    }
}

void sigint_handler(int signum) {
    shutdown = 1;
}

void reading() {
    // TODO: reader stuff
    printf("I'm reader!\n");
}

void writing() {
    printf("I'm writer!\n");
    // TODO: writer stuff
}

void *reader_handler(void *args) {
    struct reader_args *r_args = (struct reader_args*) args;

    struct thread_info info;
    info.tid = r_args->id;
    info.function = READER;

    int divider = r_args->divider;

    while(!shutdown) {
        // Push myself to the fifo
        pthread_mutex_lock(&fifo_mutex);
        fifo_push(fifo, info);
        pthread_mutex_unlock(&fifo_mutex);

        // Lock the waiting-for-handle mutex
        pthread_mutex_lock(&mutex);

        // Signal the parent: "I'm entering the waiting state, handle me parent, after I release waiting-for-handle mutex!"
        pthread_mutex_lock(&parent);
        ++go_parent;
        pthread_cond_signal(&parent_cond);
        pthread_mutex_unlock(&parent);

        while (!go_ahead[info.tid]) {
            // Enter the waiting state, free the waiting-for-handle mutex for other threads and parent
            pthread_cond_wait(&cond, &mutex);
        }
        // You were released from the waiting state, you're free to work!
        pthread_mutex_unlock(&mutex);
        reading();
        // Work finished signal the parent about that
        pthread_mutex_lock(&end_of_work_mutex);
        --readers;
        go_ahead[info.tid] = 0;
        if (readers == 0) writer_busy = 0;
        pthread_cond_signal(&end_of_work_cond);
        pthread_mutex_unlock(&end_of_work_mutex);
    }

    return 0;
}

void *writer_handler(void *args) {
    struct writer_args *w_args = (struct writer_args*) args;

    struct thread_info info;
    info.tid = w_args->id;
    info.function = WRITER;

    while(!shutdown) {
        // Push myself to the fifo
        pthread_mutex_lock(&fifo_mutex);
        fifo_push(fifo, info);
        pthread_mutex_unlock(&fifo_mutex);

        // Lock the waiting-for-handle mutex
        pthread_mutex_lock(&mutex);

        // Signal the parent: "I'm entering the waiting state, handle me parent, after I release waiting-for-handle mutex!"
        pthread_mutex_lock(&parent);
        ++go_parent;
        pthread_cond_signal(&parent_cond);
        pthread_mutex_unlock(&parent);

        while (!go_ahead[info.tid]) {
            // Enter the waiting state, free the waiting-for-handle mutex for other threads and parent
            pthread_cond_wait(&cond, &mutex);
        }
        // You were released from the waiting state, you're free to work!
        pthread_mutex_unlock(&mutex);

        writing();
        // Work finished signal the parent about that
        pthread_mutex_lock(&end_of_work_mutex);
        writer_busy = 0;
        go_ahead[info.tid] = 0;
        pthread_cond_signal(&end_of_work_cond);
        pthread_mutex_unlock(&end_of_work_mutex);
    }

    return 0;
}

int main(int argc, char **argv) {
    atexit(exit_handler);
    signal(SIGINT,sigint_handler);
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

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED);

    init_mutexes_and_conds();

    struct reader_args **r_args = malloc(sizeof(struct reader_args*) * R);
    struct writer_args **w_args = malloc(sizeof(struct writer_args*) * W);

    for(int i = 0;i<R;++i) {
        r_args[i] = malloc(sizeof(struct reader_args));
        r_args[i]->id = i;
        r_args[i]->divider = 4; // TODO: insert rand() here
        pthread_create(&tid,&attr,reader_handler,r_args[i]);
    }
    for(int i = 0;i<W;++i) {
        w_args[i] = malloc(sizeof(struct writer_args));
        w_args[i]->id = i+R;
        pthread_create(&tid,&attr,writer_handler,w_args[i]);
    }

    struct thread_info info;
    while(!shutdown) {
        // Wait for the signal from worker threads
        pthread_mutex_lock(&parent);
        while (!go_parent) {
            pthread_cond_wait(&parent_cond, &parent);
        }
        // Some thread signals you, to begin to handle it
        --go_parent;
        pthread_mutex_unlock(&parent);

        // Some thread signaled you, so it must be waiting now in the queue, pop it
        pthread_mutex_lock(&fifo_mutex);
        info = fifo_pop(fifo);
        pthread_mutex_unlock(&fifo_mutex);

        // Check the function of the thread
        if (info.function == WRITER) {
            pthread_mutex_lock(&end_of_work_mutex);
            // Okay so we've got a writer here. Wait for all the threads to finish work, so the writer can go.
            while (writer_busy) {
                pthread_cond_wait(&end_of_work_cond, &end_of_work_mutex);
            }
            // Writer can go work, signal it about that
            writer_busy = 1;
            pthread_mutex_unlock(&end_of_work_mutex);

            pthread_mutex_lock(&mutex);
            go_ahead[info.tid] = 1;
            pthread_cond_broadcast(&cond);
            pthread_mutex_unlock(&mutex);

        } else {
            pthread_mutex_lock(&end_of_work_mutex);
            // We've got a reader. Wait for writer to finish writing. Otherwise if no one is writing, launch the reader.
            while (writer_busy) {
                pthread_cond_wait(&end_of_work_cond, &end_of_work_mutex);
            }
            ++readers;
            writer_busy = 1; // Writer cannot write right now! Because someone will be reading the data
            pthread_mutex_unlock(&end_of_work_mutex);

            pthread_mutex_lock(&mutex);
            go_ahead[info.tid] = 1;
            pthread_cond_broadcast(&cond);
            pthread_mutex_unlock(&mutex);
        }
    }

    pthread_exit(0);
}
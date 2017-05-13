#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include "finals.h"

size_t k;
int file;
char* word;
int N;
pthread_t* threads;
volatile sig_atomic_t *thread_complete;
pthread_mutex_t mutex;

struct thread_args {
    int id;
} **args;


void exit_program(int status, char* message) {
    if(status == EXIT_SUCCESS) {
        printf("%s\n",message);
    } else {
        perror(message);
    }
    exit(status);
}

int seek_for_word(char *buffer) {
    char id_str[sizeof(int)], text[RECORDSIZE+1]; // +1 cause of null given in strcpy
    char *strtok_pointer;
    char strtok_buf[RECORDSIZE*k];
    strtok_pointer = strtok_buf;
    for(int i =0;i<k;++i) {
        char *p = strtok_r(buffer, SEPARATOR,&strtok_pointer);

        strcpy(id_str, p);
        int id = atoi(id_str);  // in id_str we've got the id, let's change it to int

        p = strtok_r(NULL, "\n",&strtok_pointer); // in p now we have the rest of the record, so called - text
        if(p!=NULL) {
            strncpy(text, p, RECORDSIZE+1);
            if (strstr(text, word) != NULL) {
                return id;
            }
        }
    }
    return -1;
}

void *parallel_reader(void *arg) {
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS,NULL);
    int id;
    char buffer[1024*k];
    struct thread_args *tmp = arg;
    int jump = tmp->id;
    long multiplier = RECORDSIZE*jump*k;
    printf("%zu, file_desc= %d, arg_passed = %d\n",pthread_self(),file,jump);

    while(pread(file,buffer,RECORDSIZE*k,multiplier) > 0) {
        if((id = seek_for_word(buffer)) != -1) {
            pthread_mutex_lock(&mutex); // found the word - block the other threads from unwanted exiting/writing down their word
            printf("Found the word %s! Record id: %d, thread id: %zu\n",word,id,pthread_self());
            pthread_t self_id = pthread_self();
            for(int i=0;i<N;++i) {
                if(threads[i] != self_id) {
                    int err;
                    if(!thread_complete[i]) {
                        if ((err = pthread_cancel(threads[i])) != 0) {
                            fprintf(stderr, "Error cancelling thread %zu: %s\n", threads[i], strerror(err));
                            break;
                        }
                    }
                }
            }
            pthread_mutex_unlock(&mutex);
            pthread_cancel(self_id);
        }
        multiplier += (N*RECORDSIZE*k);
    }
    pthread_mutex_lock(&mutex);
    thread_complete[jump] = 1;
    pthread_mutex_unlock(&mutex);
    return 0;
}

void exit_handler() {
    if(file!=0) {
        close(file);
    }
    if(threads != NULL) {
        free(threads);
    }
    if(thread_complete != NULL) {
        free((void *) thread_complete);
    }
    if(args !=NULL) {
        for (int i = 0; i < N; ++i) {
            if (args[i] != NULL)
                free(args[i]);
        }
        free(args);
    }
}

int main(int argc, char ** argv) {
    if(argc != 5) {
        exit_program(EXIT_SUCCESS, "Pass 4 arguments: N - the number of threads, filename - the name of the file to read records from"
                "k - the number of records read by a thread in a single access, word - the word we seek in the file for");
    }
    atexit(exit_handler);

    N = atoi(argv[1]);
    char *filename = argv[2];
    k = (size_t) atoi(argv[3]);
    if(k<=0 || N <= 0) {
        exit_program(EXIT_SUCCESS,"Pass the N and k parameters > 0");
    }
    word = argv[4]; // eg. "pellentesque";

    if((file = open(filename, O_RDONLY)) == -1) {
        exit_program(EXIT_FAILURE, "Couldn't open the file to read records from");
    }

    pthread_mutex_init(&mutex,NULL);
    threads = malloc(sizeof(int)*N);
    thread_complete = calloc((size_t) N, sizeof(sig_atomic_t));
    args = malloc(sizeof(struct thread_args*)*N);
    for(int i=0;i<N;++i) {
        args[i] = malloc(sizeof(struct thread_args));
        args[i]->id = i;
        if(pthread_create(&threads[i],NULL,parallel_reader,args[i])) {
            exit_program(EXIT_FAILURE,"Failed to create thread");
        }
    }

    for(int i=0;i<N;++i) {
        if(pthread_join(threads[i],NULL)) {
            exit_program(EXIT_FAILURE,"Threads didn't end successfully");
        }
    }
    pthread_mutex_destroy(&mutex);

    return 0;
}
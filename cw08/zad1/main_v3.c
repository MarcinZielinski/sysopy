#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include "finals.h"

size_t k;
int file;
char* word;
int N;
pthread_t* threads;

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

    while(pread(file,buffer,RECORDSIZE*k,multiplier) > 0) {
        if((id = seek_for_word(buffer)) != -1) {
            printf("Found the word %s! Record id: %d, thread id: %zu\n",word,id,pthread_self());
        }
        multiplier += (N*RECORDSIZE*k);
    }
    return 0;
}

void exit_handler() {
    if(file!=0) {
        close(file);
    }
    if(threads != NULL) {
        free(threads);
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

    threads = malloc(sizeof(int)*N);
    args = malloc(sizeof(struct thread_args*)*N);

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED);

    for(int i=0;i<N;++i) {
        args[i] = malloc(sizeof(struct thread_args));
        args[i]->id = i;
        if(pthread_create(&threads[i],&attr,parallel_reader,args[i])) {
            exit_program(EXIT_FAILURE,"Failed to create thread");
        }
    }

    pthread_attr_destroy(&attr);
    pthread_exit(0);
}

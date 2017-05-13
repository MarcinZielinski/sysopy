#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <semaphore.h>
#include "finals.h"

const char* SEM_STR = "SEM";
size_t k;
int file;
char* word;
int N;
pthread_t* threads;
volatile sig_atomic_t *thread_complete;
volatile sig_atomic_t word_found;
pthread_mutex_t mutex;
sem_t *sem;

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
    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE,NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED,NULL);
    int id;
    char buffer[1024*k];
    struct thread_args *tmp = arg;
    int jump = tmp->id;
    long multiplier = RECORDSIZE*jump*k;
    printf("%zu, file_desc= %d, arg_passed = %d\n",pthread_self(),file,jump);

    while(pread(file,buffer,RECORDSIZE*k,multiplier) > 0) {
        if((id = seek_for_word(buffer)) != -1) {
            //printf("Obtaining mutex %zu\n",pthread_self());
            //pthread_mutex_lock(&mutex); // found the word - block the other threads from unwanted exiting/writing down their word
            sem_wait(sem);
            //while(word_found);
            //printf("Got mutex. Testing for cancelation %zu\n",pthread_self());
            pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,NULL);
            pthread_testcancel(); // mutex_lock isn't cancellation point, unfortunately
            pthread_setcancelstate(PTHREAD_CANCEL_DISABLE,NULL);
            word_found = 1;
            printf("Found the word %s! Record id: %d, thread id: %zu\n",word,id,pthread_self());
            pthread_t self_id = pthread_self();
            for(int i=0;i<N;++i) {
                if(threads[i] != self_id) {
                    int err;
                    if(!thread_complete[i]) {
                        printf("Cancelling %zu\n",threads[i]);
                        if ((err = pthread_cancel(threads[i])) != 0) {
                            fprintf(stderr, "Error cancelling thread %zu: %s\n", threads[i], strerror(err));
                            break;
                        }
                    }
                    thread_complete[i] = 1;
                }
            }
            for(int i =0;i<N;++i) {
                sem_post(sem);
            }
            word_found = 0;
            //pthread_mutex_unlock(&mutex);
            printf("Self cancelling %zu\n",self_id);
            pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,NULL);
            //pthread_cancel(self_id);
            pthread_exit(EXIT_SUCCESS);
            pthread_testcancel();
            //pthread_cancel(self_id);
        }
        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,NULL);
        pthread_testcancel();
        pthread_setcancelstate(PTHREAD_CANCEL_DISABLE,NULL);
        multiplier += (N*RECORDSIZE*k);
    }
    //while(word_found);
    //pthread_mutex_lock(&mutex);
    sem_wait(sem);
    thread_complete[jump] = 1;
    sem_post(sem);
    //pthread_mutex_unlock(&mutex);
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
    if(sem_unlink(SEM_STR) == -1) {
        perror("Error while deleting semaphores");
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
    sem = sem_open(SEM_STR, O_CREAT | O_RDWR | O_CREAT, 0600, 1);

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
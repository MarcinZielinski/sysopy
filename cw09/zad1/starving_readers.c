//
// Created by Mrz355 on 21.05.17.
//

#include <semaphore.h>
#include "starving_readers.h"

int verbose;
sem_t read_semaphore, end_read_semaphore, write_semaphore;

void exit_program(int status, char *msg) {
    if(status == EXIT_FAILURE) {
        perror(msg);
    } else {
        printf("%s\n",msg);
    }
    exit(status);
}

const char* sigint_received = "SIGINT received.\n";
void sigint_handler(int signum) {
    write(STDOUT_FILENO,sigint_received, strlen(sigint_received));
    exit(0);
}

void reading_l(int divider) {
    int count = 0;
    char *indexes_str = malloc(sizeof(char) * BUFFER_SIZE);
    char *numbers_str = malloc(sizeof(char) * BUFFER_SIZE);
    char *indexes = indexes_str;
    char *numbers = numbers_str;

    for(int i = 0;i<TAB_SIZE;++i) {
        if(tab[i] % divider == 0) {
            int to_move = sprintf(indexes_str,"%d ",i);
            indexes_str += to_move;
            to_move = sprintf(numbers_str,"%d ", tab[i]);
            numbers_str += to_move;
            ++count;
        }
    }
    printf("READER id: %zu, finished. Found numbers: %s at the indexes: %s (total: %d)\n",pthread_self(),numbers,indexes,count);
    fflush(stdin);
    free(indexes);
    free(numbers);
}

void writing_l() {
    char *indexes_str = malloc(sizeof(char) * BUFFER_SIZE);
    char *numbers_str = malloc(sizeof(char) * BUFFER_SIZE);
    char *indexes = indexes_str;
    char *numbers = numbers_str;

    int N = rand()%TAB_SIZE;
    for(int n = 0 ; n <= N ; ++n) {
        int index = rand()%2 ? n : -1;
        if(index == -1) continue;
        int value = rand()%TAB_SIZE;
        tab[index] = value;

        int to_move =sprintf(indexes_str,"%d ",index);
        indexes_str += to_move;
        to_move = sprintf(numbers_str,"%d ",value);
        numbers_str += to_move;
    }
    printf("WRITER id: %zu, finished. Changed to numbers: %s at the indexes: %s\n",pthread_self(), numbers,indexes);
    fflush(stdin);
    free(indexes);
    free(numbers);
}

void reading(int divider) {
    if(verbose) {
        reading_l(divider);
        return;
    }
    int count = 0;
    for(int i = 0;i<TAB_SIZE;++i) {
        if(tab[i] % divider == 0) {
            ++count;
        }
    }
    printf("READER, finished. Found numbers: %d\n",count);
    fflush(stdin);
}

void writing() {
    if(verbose) {
        writing_l();
        return;
    }
    for(int n = rand()%TAB_SIZE ; n >= 0 ; --n) {
        tab[rand()%TAB_SIZE] = rand();
    }
    printf("WRITER, finished.\n");
    fflush(stdin);
}

void *reader_handler(void *args) {
    struct reader_args *r_args = (struct reader_args*) args;
    int divider = r_args->divider;
    free(args);
    int readers = 0;
    while(1) {
        sem_wait(&read_semaphore);

        if (readers == 0) {
            sem_wait(&write_semaphore);
        }
        ++readers;
        sem_post(&read_semaphore);
        reading(divider);
        sem_wait(&end_read_semaphore);
        --readers;
        if (readers == 0) {
            sem_post(&write_semaphore);
        }
        sem_post(&end_read_semaphore);
    }
}

void *writer_handler(void *args) {
    while(1) {
        sem_wait(&write_semaphore);
        writing();
        sem_post(&write_semaphore);
    }
}

int main(int argc, char **argv) {
    srand((unsigned int) time(NULL));

    int R = rand() % TAB_SIZE + 1;
    int W = rand() % TAB_SIZE + 1;

    if (argc == 2) {
        if (strcmp(argv[1], "-i") == 0) {
            verbose = 1;
        }
    } else if (argc == 3) {
        R = atoi(argv[1]);
        W = atoi(argv[2]);
    } else if (argc == 4) {
        if (strcmp(argv[1], "-i") == 0) {
            verbose = 1;
        }
        R = atoi(argv[1]);
        W = atoi(argv[2]);
    }

    struct sigaction sa;
    sigemptyset(&(sa.sa_mask));
    sa.sa_flags = 0;
    sa.sa_handler = sigint_handler;
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        exit_program(EXIT_FAILURE, "Error while setting SIGINT handler");
    }

    pthread_t tid;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED);

    sem_init(&read_semaphore,0,1); // Initializing no-pshared semaphore with starting value 1
    sem_init(&write_semaphore,0,1);
    sem_init(&end_read_semaphore,0,1);

    struct reader_args **r_args = malloc(sizeof(struct reader_args*)*R);

    int r=R, w=W;
    for(int i = 0;i<(R+W);++i) {
        if(r) {
            r_args[i] = malloc(sizeof(struct reader_args));
            r_args[i]->divider = 4;
            if(pthread_create(&tid, &attr, reader_handler, r_args[i])) {
                exit_program(EXIT_FAILURE,"Error while creating reader thread");
            }
            --r;
        }
        if(w) {
            if(pthread_create(&tid, &attr, writer_handler, NULL)) {
                exit_program(EXIT_FAILURE,"Error while creating writing thread");
            }
            --w;
        }
    }

    free(r_args);

    pthread_exit(0);
}
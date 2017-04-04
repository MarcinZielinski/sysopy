//
// Created by Mrz355 on 30.03.17.
//
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <values.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#include <wait.h>

int N; // number of forked processes
int K; // number of needed requests
pid_t parent; // parent (main program) pid
volatile sig_atomic_t permission = 0; // the permission to grant
volatile sig_atomic_t requests = 0; // number of requests

struct childStruct {
    pid_t pid;
    int usr1;
    int rt;
} *childs;



int stoi(char* str) {
    char *endptr;
    errno = 0;

    int res = (int)strtol(str,&endptr,10);

    if ((errno == ERANGE && (res == LONG_MAX || res == LONG_MIN))
        || (errno != 0 && res == 0)) {
        perror("Cannot convert argument to int\n");
        exit(EXIT_FAILURE);
    }
    if (endptr == str) {
        fprintf(stderr, "No digits were found\n");
        exit(EXIT_FAILURE);
    }
    return res;
}

void killAll(int signum) {
    int pid = getpid();
    if(pid != parent) {
        for(int i=0;i<N;++i) {
            if(childs[i].pid == pid) {
                childs[i].usr1 = 2;
            }
        }
        exit(255);
    }
    else {
        printf("\nExiting program.\n");
        sleep(1);
        printf("\nEXIT\n");
        exit(EXIT_SUCCESS);
    }
}
void sigHandler(int signum, siginfo_t *value, void *extra) {
    ++requests;

    childs[requests-1].pid = value->si_value.sival_int;
    childs[requests-1].usr1 = signum;
    if (requests > K) {
        kill(childs[requests-1].pid,SIGALRM);
    } else if (requests == K) {
        for (int i = 0; i < requests; ++i) {
            kill(childs[i].pid,SIGALRM);
        }
    }
    return;
}
void childHandler(int signum) {
    permission = 1;
}
volatile sig_atomic_t realReceived = 0;
void sigRealHandler(int signum, siginfo_t *value, void *extra) {
    childs[realReceived].rt=signum;
    ++realReceived;
    //printf("\nReal-time signal received: %d from %zu\n", signum, value->si_value);
    return;
}
int main(int argc, char **argv) {
    if(argc != 3) {
        fprintf(stderr,"\nYou must specify two arguments. N - number of child processes and K - number of request needed.\n");
        exit(EXIT_FAILURE);
    }
    N = stoi(argv[1]); // number of children
    K = stoi(argv[2]); // number of needed requests

    if(K > N) {
        fprintf(stderr, "\nThe number of requests mustn't be greater than the processes number.\n");
        exit(EXIT_FAILURE);
    }
    childs = calloc((size_t) N, sizeof(childs));

    //USR1 signal handle
    struct sigaction actionStruct;
    actionStruct.sa_flags = SA_SIGINFO | SA_NODEFER;
    actionStruct.sa_sigaction = &sigHandler;
    sigfillset(&actionStruct.sa_mask);
    sigdelset(&actionStruct.sa_mask,SIGUSR1);
    if(sigaction(SIGUSR1, &actionStruct, NULL) == 0) {
        printf("Sigaction completed.\n");
    }
    else {
        perror("\nError with sigaction\n");
    }

    //real-time signal handle
    struct sigaction realActionStruct;
    realActionStruct.sa_flags = SA_SIGINFO;
    realActionStruct.sa_sigaction = &sigRealHandler;
    sigfillset(&realActionStruct.sa_mask);
    for(int i = SIGRTMIN; i < SIGRTMIN+32;++i) {
        sigdelset(&realActionStruct.sa_mask,i);
        sigaction(i,&realActionStruct,NULL);
        sigaddset(&realActionStruct.sa_mask,i);
    }

    //interrupt signal handle
    signal(SIGINT,killAll);
    signal(SIGALRM,childHandler); // listen for alarm

    parent = getpid();
    pid_t child = 1;
    for(int i=0; i<N; ++i) {
        if((child = fork())==0) {
            break;
        }
    }
    if(child == 0) {
        srand((unsigned int) (time(NULL) ^ (getpid() << 16)));
        unsigned int sleepTime = (unsigned int) (rand() % 11);
        sleep(sleepTime);

        union sigval *value = malloc(sizeof(union sigval));

        value -> sival_int = getpid();

        sigqueue(parent, SIGUSR1, *value); // send SIGUSR1 with own PID

        clock_t before = clock();

        while(permission == 0) { // wait for permission
            if((clock()-before)/CLOCKS_PER_SEC > 10) {
                printf("\nNo response from parent. Signal probably overwriten.\n");
                raise(SIGINT);
            }
        }

        //send realtimesignal
        int realTimeSignal = SIGRTMIN + (rand() % 32);
        value -> sival_int = getpid();
        sigqueue(parent,realTimeSignal, *value);

        int diff = (int)((clock() - before) / (CLOCKS_PER_SEC));
        free(value);

        exit(diff);
    }

    int status = 0;
    int pid = 0;
    int counter = 0;
    while(counter<N) { // wait for signals
        if((pid = wait(&status))!=-1) {
            if(WEXITSTATUS(status)==255) {
                printf("\nChildren finished working with SIGINT. PID: %d, EXIT CODE: %d\n", pid, WEXITSTATUS(status));
            } else {
                printf("\nChildren finished working. PID: %d, 1st signal: %d, 2nd signal: %d, EXIT CODE: %d\n", pid,
                       childs[counter].usr1, childs[counter].rt, WEXITSTATUS(status));
            }
            ++counter;
        }
    }
    printf("\nAll processes finished job, exiting program\n");
    sleep(1);
    printf("\nEXIT\n");
    exit(EXIT_SUCCESS);
}
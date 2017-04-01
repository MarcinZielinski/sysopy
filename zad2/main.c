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
#include <sys/resource.h>
#include <string.h>

int N; // number of forked processes
int K; // number of needed requests
pid_t *PIDs; // array of processes that requested permission
pid_t parent; // parent (main program) pid
int permission = 0; // the permission to grant

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
    if(getpid() != parent) {
        printf("\nSIGINT received, killing %d\n", getpid());
        kill(getpid(), SIGKILL);
    }
    else {
        printf("\nExiting program.\n");
        sleep(1);
        printf("\nEXIT\n");
        exit(EXIT_SUCCESS);
    }
}
volatile int requests = 0;
void sigHandler(int signum, siginfo_t *value, void *extra) {
    PIDs[requests++] = value->si_value.sival_int;
    printf("\nRequest: %d\n",requests);
    printf("Signal received: %d from %zu\n", signum, value->si_value);
    if (requests > K) {
        kill(PIDs[requests-1], SIGALRM);
    } else if (requests == K) {
        for (int i = 0; i < requests; ++i) {
            kill(PIDs[i], SIGALRM);
        }
    }
    return;
}
void childHandler(int signum) {
    permission = 1;
}
volatile sig_atomic_t realReceived = 0;
void sigRealHandler(int signum, siginfo_t *value, void *extra) {
    ++realReceived;
    printf("\nReal-time signal received: %d from %zu\n", signum, value->si_value);
    if(realReceived >= N) {
        printf("\nAll processes finished job, exiting program\n");
        sleep(1);
        printf("\nEXIT\n");
        exit(EXIT_SUCCESS);
    }
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
    PIDs = malloc(sizeof(sigval_t)*N); // store the PIDs of child which has requested the access to continue

    //USR1 signal handle
    struct sigaction actionStruct;
    actionStruct.sa_flags = SA_SIGINFO;
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
    for(int i = SIGRTMIN; i < SIGRTMIN + 32;++i) {
        sigdelset(&realActionStruct.sa_mask,i);
        sigaction(i,&realActionStruct,NULL);
        sigaddset(&realActionStruct.sa_mask,i);
    }

    //interrupt signal handle
    signal(SIGINT,killAll);

    parent = getpid();
    pid_t child = 1;
    for(int i=0; i<N; ++i) {
        if((child = fork())==0) {
            break;
        }
    }

    if(child == 0) {
        srand((unsigned int) (time(NULL) ^ (getpid() << 16)));
        struct rusage *usage = malloc(sizeof(struct rusage));
        sleep((unsigned int) (rand() % 11));

        union sigval *value = malloc(sizeof(union sigval));

        signal(SIGALRM,childHandler); // listen for alarm
        value -> sival_int = getpid();

        //time_t start_t, end_t;
        //double diff_t;

        sigqueue(parent, SIGUSR1, *value); // send SIGUSR1 with own PID
        clock_t before = clock();
        //time(&start_t);

        while(!permission); // wait for permission

        //send realtimesignal
        int realTimeSignal = SIGRTMIN + (rand() % 32);
        value -> sival_int = getpid();
        sigqueue(parent,realTimeSignal, *value);

        //time(&end_t);
        //diff_t = difftime(end_t, start_t);

        //printf("Execution time = %f\n", diff_t);

        //display statistics
//        if(getrusage(RUSAGE_SELF,usage) == 0) {
//            printf("\nChild process PID: %d has finished job. S: %lf s, U: %lf s\n",
//                   getpid(), (double)usage->ru_stime.tv_sec + (double)(usage->ru_stime.tv_usec)/1e6,
//                   (double)usage->ru_utime.tv_sec + (double)(usage->ru_utime.tv_usec)/1e6);
//        } else {
//            perror("\nCouldn't get the usage info of child process\n");
//            exit(EXIT_FAILURE);
//        }
        clock_t diff = clock() - before;

        free(usage);
        free(value);

        exit((int) diff);
    }

    while(1) {
        pause();
    } // wait for signals
}
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

int requests = 0;
int K;
pid_t *PIDs;

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
}
int permission = 0;
void childHandler(int signum) {
    permission = 1;
}

int main(int argc, char **argv) {
    if(argc != 3) {
        fprintf(stderr,"\nYou must specify two arguments. N - number of child processes and K - number of request needed.\n");
        exit(EXIT_FAILURE);
    }
    int N = stoi(argv[1]); // number of children
    K = stoi(argv[2]); // number of needed requests

    if(K > N) {
        fprintf(stderr, "\nThe number of requests mustn't be greater than the processes number.\n");
        exit(EXIT_FAILURE);
    }


    PIDs = malloc(sizeof(sigval_t)*N); // store the PIDs of child which has requested the access to continue

    struct sigaction actionStruct;
    actionStruct.sa_flags = SA_SIGINFO;
    actionStruct.sa_sigaction = &sigHandler;
    sigemptyset(&actionStruct.sa_mask);
    if(sigaction(SIGUSR1, &actionStruct, NULL) == 0) {
        printf("Sigaction completed.\n");
    }
    else {
        perror("\nError with sigaction\n");
    }

    pid_t parent = getpid();
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

        signal(SIGALRM,childHandler);
        value -> sival_int = getpid();
        sigqueue(parent, SIGUSR1, *value);
        //pause();
        while(!permission);
        //send realtimesignal

        if(getrusage(RUSAGE_CHILDREN,usage) == 0) {
            printf("\nChild process PID: %d has finished job. S: %lf s, U: %lf s\n",
                   getpid(), (double)usage->ru_stime.tv_sec + (double)(usage->ru_stime.tv_usec)/1e6,
                   (double)usage->ru_utime.tv_sec + (double)(usage->ru_utime.tv_usec)/1e6);
        } else {
            perror("\nCouldn't get the usage info of child process\n");
            exit(EXIT_FAILURE);
        }

        free(usage);
        free(value);

        exit(EXIT_SUCCESS);
    }

    while(1) {}// wait for signals

    return 0;
}
//
// Created by Mrz355 on 01.04.17.
//

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <values.h>
#include <unistd.h>
#include <signal.h>
#include <wait.h>

// pid's
pid_t parent;
pid_t child;

// counters
volatile sig_atomic_t signalsSentToChild;
volatile sig_atomic_t signalsReceivedByChild;
volatile sig_atomic_t signalsReceivedFromChild;

volatile sig_atomic_t the_end_of_child = 0; // used to determine if child got the second-type signal

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
    printf("\nExiting program.\n");
    sleep(1);
    printf("\nEXIT\n");
    exit(EXIT_SUCCESS);
}

void firstHandler(int signum) {
    if(child != 0) {
        ++signalsReceivedFromChild;
    } else {
        ++signalsReceivedByChild;
        kill(parent, signum);
    }
}

void secondHandler(int signum) {
    if(child == 0) {
        ++signalsReceivedByChild;
        ++the_end_of_child;
    }
}

int main(int argc, char** argv) {
    if(argc != 3) {
        fprintf(stderr,"\nYou must specify two arguments. L - number of signals to be send and Type - type of method to be used (1,2,3)\n");
        exit(EXIT_FAILURE);
    }
    int L = stoi(argv[1]); // number of children
    int type = stoi(argv[2]); // number of needed requests

    if(type > 3 || type < 1) {
        fprintf(stderr,"\nYou must specify the Type argument between values: 1,2,3. 1 - kill, 2 - sigqueue, 3 - real-time signal with kill\n");
        exit(EXIT_FAILURE);
    }

    signal(SIGINT,killAll);

    int sig1 = SIGUSR1;
    int sig2 = SIGUSR2;

    if(type == 3) {
        sig1 = SIGRTMIN + 5;
        sig2 = SIGRTMIN + 6;
    }

    signal(sig1, firstHandler);
    signal(sig2, secondHandler);

    parent = getpid();
    child = fork();

    if(child == 0) {
        while (the_end_of_child == 0){pause();}
        exit(signalsReceivedByChild);
    } else {
        sleep(1);
        union sigval *value = malloc(sizeof(union sigval));
        for(int i=0;i<L;++i) {
            if (type == 2) {
                sigqueue(child, sig1, *value);
            } else {
                kill(child, sig1);
            }
            ++signalsSentToChild;
            sleep(1);
        }
        sleep(1);
        if(type == 2) {
            sigqueue(child,sig2,*value);
        } else {
            kill(child, sig2);
        }
        ++signalsSentToChild;

        free(value);
    }

    int status;
    wait(&status);
    printf("Number of received signals by child: %d\n", WEXITSTATUS(status));
    printf("Number of sent signals to child: %d\n",signalsSentToChild);
    printf("Number of received signals from child: %d\n", signalsReceivedFromChild);

    return 0;
}
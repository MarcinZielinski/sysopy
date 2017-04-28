//
// Created by Mrz355 on 28.04.17.
//


#include <signal.h>
#include <stdlib.h>
#include <stdio.h>

int N;

void exit_handler() {
    // TODO: inform clients about closing the barber shop
}

void exit_program(int status, char *message) {
    status == EXIT_SUCCESS? printf("%s\n",message) : perror(message);
    exit(status);
}

void sigint_handler(int signum) {
    exit_program(EXIT_SUCCESS,"Sigint received.");
}

int set_sigint_handling() {
    struct sigaction sig;
    sig.sa_flags = 0;
    sigset_t mask;
    if(sigemptyset(&mask) == -1) return -1;
    sig.sa_mask = mask;
    sig.sa_handler = sigint_handler;
    return sigaction(SIGINT, &sig, NULL);
}

int main(int argc, char** argv) { // N - amount of chairs in waiting room
    if(argc != 2) {
        //TODO: uncomment this exit_program(EXIT_FAILURE, "Bad number of arguments! Pass the number of chairs you want to be in waiting room");
    }

    N = 1;
    //TODO: uncomment this N = atoi(argv[1]);

    atexit(exit_handler);

    if(set_sigint_handling() == -1) {
        exit_program(EXIT_FAILURE, "Error while setting the sigint handler");
    }



    return 0;
}


//
// Created by Mrz355 on 28.04.17.
//

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <wait.h>

int N,S;

void exit_handler() {
    // TODO: exit handler code..
}

void exit_program(int status, char *message) {
    status == EXIT_SUCCESS? printf("%s\n",message) : perror(message);
    exit(status);
}

int visit_barber() {
    int status = 0;
    int actual_cuts = 0;

    while(actual_cuts < S) {
        //TODO: try to cut
    }

    return status;
}

int main(int argc, char **argv) { // N - number of clients to create and S - number of required cuts
    if(argc != 3) {
        //TODO: uncomment this exit_program(EXIT_FAILURE, "Bad number of arguments! Pass the number of clients to create and the number of required cuts");
    }
    atexit(exit_handler);

    N = 2;
    S = 2;
    //TODO: uncomment N = atoi(argv[2]); S = atoi(argv[3]);

    for(int i = 0; i < N; ++i) {
        if(fork() == 0) { // don't let children create their own children
            visit_barber() == -1 ? _exit(EXIT_FAILURE) : _exit(EXIT_SUCCESS);
        }
    }

    int clients_served = 0;
    int status;
    while(clients_served < N) {
        wait(&status);
        if(status == EXIT_SUCCESS) ++clients_served;
        else exit_program(EXIT_SUCCESS, "Child failed to execute properly"); // success, because it's not fault of parent process
    }

    return 0;
}
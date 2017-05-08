//
// Created by Mrz355 on 28.04.17.
//

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <wait.h>
#include "communication.h"

int N,S;
int sem_id;
int *shm_tab;
int cut_received = 0;
int actual_cuts = 0;

void exit_handler() {
    // TODO: exit handler code..
}

int try_to_seat() {
    int status;
    //printf("%zu, %d CLIENT: Trying to get fifo.\n", get_time(), getpid());
    take_semaphore(sem_id,FIFO);
    //printf("%zu, %d CLIENT: Got fifo.\n", get_time(), getpid());
    int barber_semval = semctl(sem_id,BARBER,GETVAL);
    if(barber_semval == -1) {
        exit_program(EXIT_FAILURE, "Barber semaphore checking error");
    }
    //printf("%zu, %d CLIENT: Barber SEEMAPHORE RETURNED: %d\n", get_time(), getpid(), barber_semval);

    if(barber_semval == 0) {    // if barber sleeping
        printf("%zu, %d CLIENT: Barber is sleeping. Let's wake him up.\n", get_time(), getpid());
        give_semaphore(sem_id,BARBER); // wake him up
        give_semaphore(sem_id,BARBER); // WAKE HIM MORE UP
        fifo_sit_on_chair(shm_tab,getpid()); // sit on chair
        status = 0;
        // and give him the fifo semaphore
    } else { // if barber doing something try to seat in the waiting room
        //printf("%zu, %d CLIENT: Barber is busy. I'm going to DO STH!\n", get_time(), getpid());
        if(fifo_push(shm_tab,getpid()) == -1) { // if it is full
            printf("%zu, %d CLIENT: Barber is full. Going away.\n", get_time(), getpid());
            status = -1;
        } else {
            printf("%zu, %d CLIENT: Barber is busy. Taking seat in waiting room. \n", get_time(), getpid());
            status = 0;
        }
    }

    give_semaphore(sem_id,FIFO); // give the semaphore to someone else
    return status;
}

int visit_barber() {

    while(actual_cuts < S) {
        take_semaphore(sem_id,CLIENT);
        //printf("%zu, %d CLIENT: Took client semaphore.\n", get_time(), getpid());
        int res = try_to_seat();
        give_semaphore(sem_id,CLIENT);
        if(res == 0) {
            //give_semaphore(sem_id,CLIENT);
            while(!cut_received);
            cut_received = 0;
            ++actual_cuts;
            printf("%zu, %d CLIENT: Got pretty cut.\n", get_time(), getpid());
        }
    }
    if(shmdt(shm_tab) == -1)
        exit_program(EXIT_FAILURE,"Error while detaching from shared memory");
    return 0;
}

void sigrtmin_handler(int signum) {
    cut_received = 1;
}


void sigint_handler(int signum) {
    _exit(EXIT_FAILURE);
}

int main(int argc, char **argv) { // N - number of clients to create and S - number of required cuts
    if(argc != 3) {
        //TODO: uncomment this exit_program(EXIT_FAILURE, "Bad number of arguments! Pass the number of clients to create and the number of required cuts");
    }
    atexit(exit_handler);

    N = 5;
    S = 2;
    //TODO: uncomment N = atoi(argv[2]); S = atoi(argv[3]);

    struct sigaction sigact;
    sigemptyset(&(sigact.sa_mask));
    sigact.sa_handler = sigint_handler;
    if(sigaction(SIGINT,&sigact,NULL) == -1) {
        exit_program(EXIT_FAILURE,"Couldn't set handler for sigint signal");
    }

    struct sigaction sigact2;
    sigemptyset(&(sigact2.sa_mask));
    sigact2.sa_handler = sigrtmin_handler;
    if(sigaction(SIGRTMIN,&sigact2,NULL) == -1) {
        exit_program(EXIT_FAILURE,"Couldn't set handler for sigint signal");
    }


    key_t key = ftok(FTOK_PATH,FTOK_ID);
    int shm_id = shmget(key,0,0);
    if(shm_id == -1)
        exit_program(EXIT_FAILURE, "Error while getting shared memory");

    shm_tab = shmat(shm_id,NULL,0);
    if(shm_tab == (int *) -1)
        exit_program(EXIT_FAILURE, "Error while getting address of shared memory");

    sem_id = semget(key,0,0);
    if(sem_id == -1)
        exit_program(EXIT_FAILURE,"Error while getting semaphores");

    for(int i = 0; i < N; ++i) {
        if(fork() == 0) { // don't let children create their own children
            visit_barber() == -1 ? _exit(EXIT_FAILURE) : _exit(EXIT_SUCCESS);
        }
    }

    int clients_served = 0;
    int status;
    while(clients_served < N) {
        wait(&status);
        ++clients_served;
        if(status == EXIT_SUCCESS) ++clients_served;
        else exit_program(EXIT_SUCCESS, "Child failed to execute properly"); // success, because it's not fault of parent process
    }

    return 0;
}
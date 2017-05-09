//
// Created by Mrz355 on 28.04.17.
//

#include "communication.h"


int N,S;
int sem_id;
int *shm_tab;
int actual_cuts = 0;
//sigset_t mask;

int try_to_seat() {
    int status;
    take_semaphore(sem_id,FIFO);
    int barber_semval = semctl(sem_id,BARBER,GETVAL);
    if(barber_semval == -1) {
        exit_program(EXIT_FAILURE, "Barber semaphore checking error");
    }

    if(barber_semval == 0) {    // if barber sleeping
        printf("%zu, %d CLIENT: Barber is sleeping. Let's wake him up.\n", get_time(), getpid());
        give_semaphore(sem_id,BARBER); // wake him up
        give_semaphore(sem_id,BARBER); // WAKE HIM MORE UP (indicates that barber is working - let to know it to the others)
        fifo_sit_on_chair(shm_tab,getpid()); // sit on chair
        status = 0;
        // and give him the fifo semaphore
    } else { // if barber doing something try to seat in the waiting room
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
        //take_semaphore(sem_id,CLIENT);    // synchronizing clients' access to barber shop - may be not used
        int res = try_to_seat();
        //give_semaphore(sem_id,CLIENT);
        if(res == 0) {
            take_semaphore(sem_id,CUT);
            //sigsuspend(&mask);
            ++actual_cuts;
            printf("%zu, %d CLIENT: Got pretty cut.\n", get_time(), getpid());
        }
    }
    if(shmdt(shm_tab) == -1)
        exit_program(EXIT_FAILURE,"Error while detaching from shared memory");
    return 0;
}

void sigint_handler(int signum) {
    _exit(EXIT_FAILURE);
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
// This commented code throughout my program is solving the problem with signals.
// I'm aware that semaphores don't always queue their access order, but the real-time signals are queued, that's why I left this solution.
//
//void sigrtmin_handler(int signum) {
//
//}
//
//int set_sigrtmin_handling() {
//    struct sigaction sig;
//    sig.sa_flags = 0;
//    sigset_t mask2;
//    if(sigemptyset(&mask2) == -1) return -1;
//    sig.sa_mask = mask2;
//    sig.sa_handler = sigrtmin_handler;
//    return sigaction(SIGRTMIN, &sig, NULL);
//}
//
//void mask_signals() {
//    sigfillset(&mask);
//    sigdelset(&mask,SIGRTMIN);
//    sigdelset(&mask,SIGINT);
//}

void init_res() {
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
}

int main(int argc, char **argv) { // N - number of clients to create and S - number of required cuts
    if(argc != 3) {
        exit_program(EXIT_FAILURE, "Bad number of arguments! Pass the number of clients to create and the number of required cuts");
    }

    N = atoi(argv[1]); S = atoi(argv[2]);

    if(set_sigint_handling() == -1) {
        exit_program(EXIT_FAILURE, "Error while setting the sigint handler");
    }
    //if(set_sigrtmin_handling() == -1) {
    //    exit_program(EXIT_FAILURE, "Error while setting the sigrtmin handler");
    //}
    //mask_signals();
    init_res();

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
//
// Created by Mrz355 on 28.04.17.
//

#include "communication.h"

int N,S;
int shm_size;
sem_t *sem_tab[3];
pid_t *shm_tab;
int actual_cuts = 0;

int try_to_seat() {
    int status;
    sem_wait(sem_tab[FIFO]);
    int barber_semval;
    sem_getvalue(sem_tab[BARBER],&barber_semval);
    if(barber_semval == -1) {
        exit_program(EXIT_FAILURE, "Barber semaphore checking error");
    }

    if(barber_semval == 0) {    // if barber sleeping
        printf("%zu, %d CLIENT: Barber is sleeping. Let's wake him up.\n", get_time(), getpid());
        sem_post(sem_tab[BARBER]); // wake him up
        sem_post(sem_tab[BARBER]); // WAKE HIM MORE UP (indicates that barber is working - let to know it to the others)
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

    sem_post(sem_tab[FIFO]); // give the semaphore to someone else
    return status;
}

void free_res() {
    if(sem_close(sem_tab[BARBER]) == -1 || sem_close(sem_tab[FIFO]) == -1 || sem_close(sem_tab[CUT]) == -1 ) {
        exit_program(EXIT_FAILURE, "Error while closing semaphores.");
    }
    if(munmap(shm_tab,shm_size*sizeof(int)) == -1) {
        exit_program(EXIT_FAILURE,"Error while detaching from shared memory");
    }
}

int visit_barber() {

    while(actual_cuts < S) {
        int res = try_to_seat();
        if(res == 0) {
            sem_wait(sem_tab[CUT]);
            ++actual_cuts;
            printf("%zu, %d CLIENT: Got pretty cut.\n", get_time(), getpid());
        }
    }
    free_res();

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

void init_res() {
    sem_tab[BARBER] = sem_open(BARBER_STR, O_RDWR, 0600, 0);
    sem_tab[FIFO] = sem_open(FIFO_STR, O_RDWR, 0600, 0);
    sem_tab[CUT] = sem_open(CUT_STR, O_RDWR, 0600, 0);

    if(sem_tab[BARBER] == SEM_FAILED || sem_tab[FIFO] == SEM_FAILED || sem_tab[CUT] == SEM_FAILED) {
        exit_program(EXIT_FAILURE, "Semaphore getting error");
    }

    int shm_id = shm_open(MEMORY_STR, O_RDWR, 0600);
    if(shm_id == -1) {
        exit_program(EXIT_FAILURE, "Error while creating shared memory");
    }
    shm_tab = mmap(NULL, sizeof(int), PROT_READ, MAP_SHARED, shm_id, 0);
    if(shm_tab == (void*)-1) {
        exit_program(EXIT_FAILURE, "Error while getting address of shared memory");
    }

    shm_size = shm_tab[0]; // getting the size of the shared memory

    if(munmap(shm_tab,sizeof(int)) == -1) { // detaching
        exit_program(EXIT_FAILURE,"Error while detaching from the shared memory");
    }
    shm_tab = mmap(NULL, shm_size*sizeof(int), PROT_WRITE | PROT_READ, MAP_SHARED, shm_id, 0); // attaching with correct size
    if(shm_tab == (void*)-1) {
        exit_program(EXIT_FAILURE, "Error while getting address of shared memory");
    }
}

int main(int argc, char **argv) { // N - number of clients to create and S - number of required cuts
    if(argc != 3) {
        exit_program(EXIT_FAILURE, "Bad number of arguments! Pass the number of clients to create and the number of required cuts");
    }

    N = atoi(argv[1]); S = atoi(argv[2]);

    if(set_sigint_handling() == -1) {
        exit_program(EXIT_FAILURE, "Error while setting the sigint handler");
    }
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
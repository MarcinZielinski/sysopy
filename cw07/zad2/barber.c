//
// Created by Mrz355 on 28.04.17.
//

#include "communication.h"

int N;
pid_t *shm_tab;
sem_t* sem_tab[3];

void free_res() {
    if (munmap(shm_tab, (N+4)*sizeof(int)) == -1) {
        perror("Error while detaching shared memory");
    }
    if(shm_unlink(MEMORY_STR) == -1) {
        perror("Error while deleting memory segment");
    }
    if(sem_unlink(BARBER_STR) == -1 || sem_unlink(FIFO_STR) == -1 || sem_unlink(CUT_STR) == -1) {
        perror("Error while deleting semaphores");
    }
}

void exit_handler() {
    free_res();
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

int terminate = 0;

void make_cut(int client_pid) {
    printf("%zu BARBER: Started cutting %d\n", get_time(), client_pid);
    sem_post(sem_tab[CUT]);
    printf("%zu BARBER: Finished cutting %d\n", get_time(), client_pid);
}

void init_res() {
    sem_tab[BARBER] = sem_open(BARBER_STR, O_CREAT | O_RDWR | O_EXCL, 0600, 0);
    sem_tab[FIFO] = sem_open(FIFO_STR, O_CREAT | O_RDWR | O_EXCL, 0600, 1);
    sem_tab[CUT] = sem_open(CUT_STR, O_CREAT | O_RDWR | O_EXCL, 0600, 0);

    if(sem_tab[BARBER] == SEM_FAILED || sem_tab[FIFO] == SEM_FAILED || sem_tab[CUT] == SEM_FAILED) {
        exit_program(EXIT_FAILURE, "Semaphore creating error");
    }

    int shm_id = shm_open(MEMORY_STR, O_CREAT | O_RDWR | O_EXCL, 0600);
    if(shm_id == -1) {
        exit_program(EXIT_FAILURE, "Error while creating shared memory");
    }
    if(ftruncate(shm_id, (N+4)*sizeof(pid_t)) == -1) {
        exit_program(EXIT_FAILURE, "Error while truncating shared memory");
    }
    shm_tab = mmap(NULL, (N+4)*sizeof(pid_t), PROT_READ | PROT_WRITE, MAP_SHARED, shm_id, 0);
    if(shm_tab == (void*)-1) {
        exit_program(EXIT_FAILURE, "Error while getting address of shared memory");
    }

    fifo_init(shm_tab,N);
}

int main(int argc, char** argv) { // N - amount of chairs in waiting room
    if(argc != 2) {
        exit_program(EXIT_FAILURE, "Bad number of arguments! Pass the number of chairs you want to be in waiting room");
    }
    atexit(exit_handler);
    set_sigint_handling();

    N = atoi(argv[1]);

    if(set_sigint_handling() == -1) {
        exit_program(EXIT_FAILURE, "Error while setting the sigint handler");
    }

    init_res(); // initialize every resource

    int client;

    printf("%zu BARBER: Going for a nap.. \n", get_time());
    while(!terminate) {
        sem_wait(sem_tab[BARBER]); // sleep
        sem_wait(sem_tab[FIFO]); // waiting for fifo, to take client
        client = fifo_get_client_on_chair(shm_tab);
        sem_post(sem_tab[FIFO]);
        make_cut(client);

        while(1){
            sem_wait(sem_tab[FIFO]);
            client = fifo_pop(shm_tab);
            if(client!=-1){
                make_cut(client);
                sem_post(sem_tab[FIFO]);
            }
            else{
                printf("%zu BARBER: Going for a nap.. \n", get_time());
                sem_wait(sem_tab[BARBER]);
                sem_post(sem_tab[FIFO]);
                break;
            }
        }
    }

    free_res();

    return 0;
}


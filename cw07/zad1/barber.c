//
// Created by Mrz355 on 28.04.17.
//

#include "communication.h"


int N;
int shm_id;
int *shm_tab;
int sem_id;

void free_res() {
    if (shmdt(shm_tab) == -1) {
        perror("Error while detaching shared memory");
    }
    if(shmctl(shm_id,IPC_RMID,NULL) == -1) {
        perror("Error while deleting memory segment");
    }
    if(semctl(sem_id,0,IPC_RMID) == -1) {
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
    give_semaphore(sem_id,CUT);
    //kill(client_pid, SIGRTMIN);
    printf("%zu BARBER: Finished cutting %d\n", get_time(), client_pid);
}

void init_res() {
    key_t key = ftok(FTOK_PATH,FTOK_ID);

    sem_id = semget(key, 4, IPC_CREAT | 0600);
    if(sem_id == -1) exit_program(EXIT_FAILURE, "Semaphore creating error");

    if(semctl(sem_id,BARBER,SETVAL,0) == -1 ||
       semctl(sem_id,FIFO,SETVAL,1) ||
       semctl(sem_id,CLIENT,SETVAL,1) ||
       semctl(sem_id,CUT,SETVAL,0))
        exit_program(EXIT_FAILURE, "Error while initializing the semaphores value");

    shm_id = shmget(key, (N+4)*sizeof(pid_t), IPC_CREAT | 0600);
    if(shm_id == -1)
        exit_program(EXIT_FAILURE, "Error while creating shared memory");

    shm_tab = shmat(shm_id, NULL, 0);
    if(shm_tab == (int *) -1)
        exit_program(EXIT_FAILURE, "Error while getting address of shared memory");

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
        take_semaphore(sem_id,BARBER); // sleep
        take_semaphore(sem_id,FIFO); // waiting for fifo, to take client
        client = fifo_get_client_on_chair(shm_tab);
        give_semaphore(sem_id,FIFO);
        make_cut(client);

        while(1){
            take_semaphore(sem_id,FIFO);
            client = fifo_pop(shm_tab);
            if(client!=-1){
                make_cut(client);
                give_semaphore(sem_id,FIFO);
            }
            else{
                printf("%zu BARBER: Going for a nap.. \n", get_time());
                take_semaphore(sem_id,BARBER);
                give_semaphore(sem_id,FIFO);
                break;
            }
        }
    }

    free_res();

    return 0;
}


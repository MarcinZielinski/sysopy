//
// Created by Mrz355 on 08.05.17.
//

#include "communication.h"

void give_semaphore(int sem_id, unsigned short sem_index) {
    struct sembuf smbuf;
    smbuf.sem_num = sem_index;
    smbuf.sem_op = 1;
    smbuf.sem_flg = 0;
    if (semop(sem_id, &smbuf, 1) == -1)
        exit_program(EXIT_FAILURE,"Semaphore giving error");
}

void take_semaphore(int sem_id, unsigned short sem_index) {
    struct sembuf smbuf;
    smbuf.sem_num = sem_index;
    smbuf.sem_op = -1;
    smbuf.sem_flg = 0;
    if (semop(sem_id, &smbuf, 1) == -1)
        exit_program(EXIT_FAILURE,"Semaphore taking error");
}
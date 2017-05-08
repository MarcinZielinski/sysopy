//
// Created by Mrz355 on 07.05.17.
//

#include <unistd.h>

void fifo_init(pid_t *tab, int n) {
    tab[0] = n+4; // len of whole queue
    tab[1] = 0; // actual clients
    tab[2] = 4; // actual starting index
    // tab[3] - client actually sitting on chair
}

int fifo_is_full(pid_t *tab) {
    return tab[0]-4 == tab[1] ? 1 : 0;
}

int fifo_is_empty(pid_t *tab) {
    return tab[1] == 0 ? 1 : 0;
}

int fifo_push(pid_t *tab, pid_t client_pid) {
    if(fifo_is_full(tab)) return -1;
    //printf("I'm CLIENT: %d, the fifo is not full :)\n",client_pid);
    int i = ((tab[1] + tab[2]) % tab[0]) + 4 * ((tab[1]+tab[2]) / tab[0]); // magic index :) (nah, just looping back if exceeded the range)
    //printf("I'm CLIENT: %d, The value of i: %d\n",client_pid,i);
    tab[i] = client_pid;

    ++tab[1]; // incrementing actual clients

    return 0;
}

pid_t fifo_pop(pid_t *tab) {
    if(fifo_is_empty(tab)) return -1;

    int i = tab[2];
    //printf("I'm the barber GTFO of my fifo %d! The value of i: %d\n",tab[i],i);
    tab[2] = (tab[2] + 1 + 4 * ((tab[2] + 1) / tab[0])) % tab[0]; // looping back if exceeded the range
    --tab[1]; // decrementing actual clients


    return tab[i];
}

void fifo_sit_on_chair(pid_t *tab, pid_t client_pid) {
    tab[3] = client_pid;
}

pid_t fifo_get_client_on_chair(pid_t *tab) {
    return tab[3];
}
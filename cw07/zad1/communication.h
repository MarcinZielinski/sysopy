//
// Created by Mrz355 on 28.04.17.
//

#ifndef CW07_COMMUNICATION_H
#define CW07_COMMUNICATION_H

#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include "util.c"
#include "semaphores.c"
#include "fifo.c"

#define BARBER 0
#define FIFO 1
#define CLIENT 2
#define CUT 3

static const char* FTOK_PATH = ".";  // the key
static const int FTOK_ID = 'p';   // and this id

#endif //CW07_COMMUNICATION_H

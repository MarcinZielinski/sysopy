//
// Created by Mrz355 on 28.04.17.
//

#ifndef CW07_COMMUNICATION_H
#define CW07_COMMUNICATION_H

#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <wait.h>
#include <sys/mman.h>
#include <time.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <fcntl.h>
#include "util.c"
#include "fifo.c"

#define BARBER 0
#define FIFO 1
#define CUT 2

static const char* BARBER_STR = "BARBER";
static const char* FIFO_STR = "FIFO";
static const char* CUT_STR = "CUT";

static const char* MEMORY_STR = "MEMORY";

#endif //CW07_COMMUNICATION_H

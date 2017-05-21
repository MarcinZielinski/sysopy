//
// Created by Mrz355 on 21.05.17.
//

#ifndef CW09_FIFO_SYNC_H
#define CW09_FIFO_SYNC_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include "fifo.c"

#define WRITER 0
#define READER 1

#define TAB_SIZE 20
#define BUFFER_SIZE 1024

struct reader_args {
    int divider;
    int id;
};

struct writer_args {
    int id;
};

static int tab[TAB_SIZE];

#endif //CW09_FIFO_SYNC_H

//
// Created by Mrz355 on 21.05.17.
//

#ifndef CW09_STARVING_WRITERS_H
#define CW09_STARVING_WRITERS_H

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <pthread.h>

#define TAB_SIZE 20
#define BUFFER_SIZE 1024

struct reader_args {
    int divider;
};

static int tab[TAB_SIZE];

#endif //CW09_STARVING_WRITERS_H

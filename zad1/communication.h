//
// Created by Mrz355 on 19.04.17.
//

#ifndef CW06_COMMUNICATION_H
#define CW06_COMMUNICATION_H

#endif //CW06_COMMUNICATION_H

#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#define MAX_MSG_LEN 64

typedef struct message {
    long type;
    char value[MAX_MSG_LEN];
} message1;

static const char* SERVER_PATH = "HOME";  // the key is generated through the $HOME env variable
static const int SERVER_ID = 'p';   // and this id
static const size_t MAX_MSG_SIZE = sizeof(struct message) - sizeof(long);


//
// Created by Mrz355 on 29.05.17.
//

#ifndef CW10_COMMUNICATION_H
#define CW10_COMMUNICATION_H

#define MAX_UNIX_PATH 108
#define MAX_EVENTS 16
#define MAX_NAME_LEN 108
#define MAX_CLIENTS 16

#include <stdio.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>

typedef enum operation {
    ADD, DIFF, MUL, DIV
} operation_t;

typedef enum msg_type {
    TASK, RESULT, PING, PONG, LOGIN, LOGOUT
} msg_type_t;

typedef struct task {
    int id;
    operation_t operation;
    int a;
    int b;
} task_t;

typedef struct result {
    int id;
    int result;
} result_t;

typedef struct client {
    int fd;
    char name[MAX_NAME_LEN];
    int pings;
    int pongs;
} client_t;

typedef struct msg {
    msg_type_t type;
    union {
        task_t task;
        result_t result;
        char name[MAX_NAME_LEN];
    } msg;
} msg_t;

#endif //CW10_COMMUNICATION_H

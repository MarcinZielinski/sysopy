//
// Created by Mrz355 on 19.04.17.
//

#include "communication.h"

void exit_program(int status, char *exit_message) {
    perror(exit_message);
    exit(status);
}

void exit_handler() {

}

int main() {
    atexit(exit_handler);

    key_t server_key;
    key_t private_key;

    if((server_key = ftok(getenv(SERVER_PATH),SERVER_ID))==-1) {
        exit_program(EXIT_FAILURE,"Error while generating the server key");
    }
    if((private_key = ftok(".",getpid())) == -1) {
        exit_program(EXIT_FAILURE,"Error while generating the private key");
    }

    int server_qid;
    int private_qid;

    if((server_qid = msgget(server_key,0)) == -1) {
        exit_program(EXIT_FAILURE,"Error while retrieving queue from server");
    }
    if((private_qid = msgget(private_key, IPC_CREAT | 0600)) == -1) {
        exit_program(EXIT_FAILURE,"Error while creating private queue");
    }


    struct message msg;

    msg.type = 1;
    strcpy(msg.value, "Hello world");
    msgsnd(server_qid, &msg, MAX_MSG_SIZE, 0);

    msg.type = 2;
    strcpy(msg.value, "Let's go!");
    msgsnd(server_qid, &msg, MAX_MSG_SIZE, 0);

    return 0;
}
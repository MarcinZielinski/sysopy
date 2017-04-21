//
// Created by Mrz355 on 19.04.17.
//

#include <ctype.h>
#include <time.h>
#include <signal.h>
#include "communication.h"

typedef struct client {
    pid_t pid;
    mqd_t qid;
};

int terminate = 0; // flag which defines whether the client sends termination request
mqd_t server_qid; // file descriptor of server queue
struct client clients[MAX_CLIENTS];
int client_count = 0;
int client_unique_id = 0;

void exit_program(int status, char *exit_message) {
    perror(exit_message);
    exit(status);
}

void terminate_server() {

}

void exit_handler() {
    terminate_server();
}

mqd_t create_queue() {
    struct mq_attr attr;
    attr.mq_maxmsg = MAX_QUEUE_SIZE;
    attr.mq_msgsize = MAX_MSG_SIZE;

    mqd_t res = mq_open(SERVER_NAME, O_CREAT | O_RDONLY, SERVER_QUEUE_PERM, &attr);
    if(res == -1) {
        exit_program(EXIT_FAILURE,"Couldn't create server queue");
    }

    return res;
}

void send_message(mqd_t qid, message *msg) {
    if(mq_send(qid,(char *) msg,MAX_MSG_LEN,1) == -1) {
        fprintf(stderr,"%d error while sending message to client",msg->pid);
    }
}

void login_handler(message msg) {
    pid_t client_pid = msg.pid;
    mqd_t client_qid;
    char* client_name = msg.value; // by value we pass client's queue name

    printf("%d is trying to connect...\n",client_pid);
    sleep(1);

    if((client_qid = mq_open(client_name, O_WRONLY)) == -1) {
        fprintf(stderr,"%d couldn't open client queue\n", client_pid);
    }

    struct message response;
    response.pid = getpid();

    if(client_count == MAX_CLIENTS) {
        response.type = DECLINE;
        strcpy(response.value,"-1");
        printf("%d couldn't connect: max clients number reached\n",client_pid);
    } else {
        struct client c1;
        c1.pid = client_pid;
        c1.qid = client_qid;
        clients[client_count++] = c1;

        response.type = ACCEPT;
        snprintf(response.value,MAX_MSG_LEN,"%d",client_unique_id++);
        printf("%d connected!\n",client_pid);
    }
    send_message(client_qid,&response);
}

void logout_handler(message msg) {

}
void echo_handler(message msg) {

}
void upper_handler(message msg) {

}
void time_handler(message msg) {

}
void terminate_handler(message msg) {

}

void receive_message() {
    unsigned int priority = 1;
    //char msg[MAX_MSG_LEN];
    struct message msg;
    ssize_t received_bytes = mq_receive(server_qid, (char *) &msg, MAX_MSG_LEN, &priority);
    if(received_bytes == -1) {
        perror("Error while receiving message");
    }
    switch(msg.type) {
        case LOGIN:
            login_handler(msg);
            break;
        case LOGOUT:
            logout_handler(msg);
            break;
        case ECHO:
            echo_handler(msg);
            break;
        case UPPER:
            upper_handler(msg);
            break;
        case TIME:
            time_handler(msg);
        default:
            printf("Unknown command\n");
    }
}

// every possible error handled inside functions create_queue and receive_message
int main() {
    atexit(exit_handler);

    puts("Launching server...");
    server_qid = create_queue();
    puts("Server launched!");

    while(!terminate) {
        receive_message();
        sleep(1);
    }

    return 0;
}
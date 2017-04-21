//
// Created by Mrz355 on 20.04.17.
//

#include <limits.h>
#include "communication.h"

mqd_t private_qid;
mqd_t server_qid;
char name[NAME_MAX];
int logout = 0;

void exit_program(int status, char *exit_message) {
    perror(exit_message);
    exit(status);
}

void exit_handler() {
    mq_close(private_qid);
    mq_unlink(name);
}

mqd_t create_queue() {
    struct mq_attr attr;
    attr.mq_maxmsg = MAX_QUEUE_SIZE;
    attr.mq_msgsize = MAX_MSG_SIZE;

    snprintf(name, NAME_MAX, "/client-%d", getpid()); // write at most NAME_MAX bytes

    mqd_t res = mq_open(name, O_CREAT | O_RDONLY, SERVER_QUEUE_PERM, &attr);
    if(res == -1)
        exit_program(EXIT_FAILURE, "Couldn't create private queue");

    return res;
}

mqd_t connect_to_server() {
    mqd_t res = mq_open(SERVER_NAME, O_WRONLY);
    if(res == -1)
        exit_program(EXIT_FAILURE,"Couldn't establish connection with server");
    return res;
}

int send_message(m_type type, char* str) {
    struct message message1;
    message1.type = type;
    message1.pid = getpid();
    strcpy(message1.value,str);

    if(mq_send(server_qid,(char *) &message1,MAX_MSG_LEN,1) == -1) {
        perror("Error while sending message to server");
        return -1;
    }
    return 0;
}

int send_message1(m_type type) {
    return send_message(type,"");
}

int receive_message(message *msg) {
    unsigned int priority = 1;
    ssize_t bytes_received = mq_receive(private_qid,(char *) msg,MAX_MSG_LEN,&priority);
    if(bytes_received == -1) {
        perror("Error while receiving message from server");
        return -1;
    }
    return 0;
}

int get_id_from_server() {
    message msg;
    send_message(LOGIN,name);
    receive_message(&msg);

    if(msg.type == DECLINE) {
        exit_program(EXIT_FAILURE,"Couldn't establish connection with server");
    }

    return atoi(msg.value);
}

void read_line() {
    char line[MAX_MSG_LEN];
    read(0,line,MAX_MSG_LEN);


}

int main() {
    atexit(exit_handler);

    private_qid = create_queue();

    puts("Connecting to server..");
    server_qid = connect_to_server();
    puts("Connected!");

    int id_from_server = get_id_from_server();

    printf("Received id from server: %d\n",id_from_server);
    fflush(stdout);

    while(!logout) {
        read_line();
    }

    return 0;
}
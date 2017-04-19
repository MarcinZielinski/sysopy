//
// Created by Mrz355 on 19.04.17.
//

#include <errno.h>
#include <ctype.h>
#include <time.h>
#include "communication.h"

typedef struct client {
    int qid;
    pid_t pid;
} client;

int server_qid; // id of this server queue
struct client clients[MAX_CLIENTS]; // clients connected to server
//int clients[MAX_CLIENTS];
int client_count=0;

void exit_program(int status, char *exit_message) {
    perror(exit_message);
    exit(status);
}

void exit_handler() {
    msgctl(server_qid, IPC_RMID, NULL);
}

void handle_login(struct message msg) {
    key_t client_key;
    int client_qid;
    pid_t client_pid;

    client_pid = msg.pid;
    client_key = atoi(msg.value);
    printf("%d is trying to connect...\n",client_pid);
    sleep(1);

    if((client_qid = msgget(client_key,0)) == -1) {
        fprintf(stderr,"Couldn't open queue from client with PID: %d: %s",msg.pid,strerror(errno));
        return;
    }
    printf("Received key: %d with queue %d\n",client_key,client_qid);


    struct message response;
    response.pid = getpid();

    if(client_count == MAX_CLIENTS) {
        response.type = DECLINE;
        strcpy(response.value,"-1");
        printf("%d couldn't connect: max clients number reached",client_pid);
    } else {
        struct client c1;
        c1.pid = client_pid;
        c1.qid = client_qid;
        clients[client_count] = c1;

        response.type = ACCEPT;
        sprintf(response.value,"%d",client_count);
        printf("%d connected!\n",client_pid);
        ++client_count;
    }
    if(msgsnd(client_qid,&response,MAX_MSG_SIZE,0) != 0) {
        fprintf(stderr,"Couldn't send message to %d: %s\n",client_pid,strerror(errno));
    }
}
void handle_logout(struct message msg) {

}

int get_client_qid(pid_t pid) {
    for(int i=0;i<client_count;++i) {
        if(pid == clients[i].pid) {
            return clients[i].qid;
        }
    }
    return -1; // error, can't find qid
}

void handle_echo(struct message msg) {
    int client_qid;
    if((client_qid = get_client_qid(msg.pid)) == -1) {
        fprintf(stderr,"Not connected client %d sent message, terminating action",msg.pid);
        return;
    }

    struct message response;
    strcpy(response.value,msg.value);
    response.pid = getpid();
    response.type = ECHO;
    if(msgsnd(client_qid,&response,MAX_MSG_SIZE,0) != 0) {
        fprintf(stderr,"Couldn't send message to %d: %s\n",msg.pid,strerror(errno));
    }
}
void handle_upper(struct message msg) {
    int client_qid;
    if((client_qid = get_client_qid(msg.pid)) == -1) {
        fprintf(stderr,"Can't find appropriate queue id for client %d",msg.pid);
        return;
    }

    struct message response;
    strcpy(response.value,msg.value);

    for(int i=0;response.value[i];++i) {
        response.value[i] = (char) toupper(response.value[i]);
    }

    response.pid = getpid();
    response.type = UPPER;
    if(msgsnd(client_qid,&response,MAX_MSG_SIZE,0) != 0) {
        fprintf(stderr,"Couldn't send message to %d: %s\n",msg.pid,strerror(errno));
    }
}

void handle_time(struct message msg) {
    int client_qid;
    if((client_qid = get_client_qid(msg.pid)) == -1) {
        fprintf(stderr,"Can't find appropriate queue id for client %d",msg.pid);
        return;
    }

    time_t rawtime;
    struct tm *timeinfo;
    time(&rawtime);
    timeinfo = localtime(&rawtime);

    struct message response;
    strcpy(response.value,asctime(timeinfo));
    response.pid = getpid();
    response.type = TIME;
    if(msgsnd(client_qid,&response,MAX_MSG_SIZE,0) != 0) {
        fprintf(stderr,"Couldn't send message to %d: %s\n",msg.pid,strerror(errno));
    }
}
void handle_terminate(struct message msg) {

}

void handle_message(struct message msg) {
    switch(msg.type) {
        case LOGIN:
            handle_login(msg);
            break;
        case LOGOUT:
            handle_logout(msg);
            break;
        case ECHO:
            handle_echo(msg);
            break;
        case UPPER:
            handle_upper(msg);
            break;
        case TIME:
            handle_time(msg);
            break;
        case TERMINATE:
            handle_terminate(msg);
            break;
        default:
            break;
    }
}

int main() {
    key_t server_key = ftok(getenv(SERVER_PATH), SERVER_ID); // create the special server key
    server_qid = msgget(server_key, IPC_CREAT | 0600); // create the brand-new server queue

    while(1) {
        struct msqid_ds stats;
        struct message msg;
        msgctl(server_qid, IPC_STAT, &stats);

        printf("Number of messages in queue now: %zu\nPid of process who last sent: %d\n",stats.msg_qnum,stats.msg_lspid);
        if(msgrcv(server_qid, &msg, MAX_MSG_SIZE, 0, 0) < 0) {
            exit_program(EXIT_FAILURE,"Error while receiving messages from clients");
            break;
        }
        printf("Message: \"%s\"\n\n",msg.value);
        handle_message(msg);
        sleep(1);
    }

    return 0;
}
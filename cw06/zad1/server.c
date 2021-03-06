//
// Created by Mrz355 on 19.04.17.
//

#include <ctype.h>
#include <time.h>
#include <signal.h>
#include "communication.h"

typedef struct client {
    int qid;
    pid_t pid;
} client;

int server_qid; // id of this server queue
struct client clients[MAX_CLIENTS]; // clients connected to server
void handle_message(struct message msg) ;

int client_count=0;
int client_unique_id=0;

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
        sprintf(response.value,"%d",client_unique_id++);
        printf("%d connected!\n",client_pid);
        ++client_count;
    }
    if(msgsnd(client_qid,&response,MAX_MSG_SIZE,0) != 0) {
        fprintf(stderr,"Couldn't send message to %d: %s\n",client_pid,strerror(errno));
    }
}

int get_client_qid(pid_t pid) {
    for(int i=0;i<client_count;++i) {
        if(pid == clients[i].pid) {
            return clients[i].qid;
        }
    }
    return -1; // error, can't find qid
}

void handle_logout(struct message msg) {
    pid_t client_pid = msg.pid;
    int client_qid;
    if((client_qid = get_client_qid(client_pid)) == -1) {
        fprintf(stderr,"Not connected client %d sent message, terminating action",msg.pid);
        return;
    }
    int i;
    for(i=0;i<client_count;++i) {
        if(clients[i].pid == client_pid) {
            printf("%d disconnected\n",client_pid);
            break;
        }
    }
    --client_count;
    while(i<client_count) {
        clients[i] = clients[i+1];
        ++i;
    }
    struct message response;
    response.pid = getpid();
    strcpy(response.value,"You've successfully logged out");
    response.type = LOGOUT;
    if(msgsnd(client_qid,&response,MAX_MSG_SIZE,0) != 0) {
        fprintf(stderr,"Couldn't send message to %d: %s\n",client_pid,strerror(errno));
    }
}
void handle_default(struct message msg) {
    int client_qid;
    if((client_qid = get_client_qid(msg.pid)) == -1) {
        fprintf(stderr,"Not connected client %d sent message, terminating action",msg.pid);
        return;
    }

    struct message response;
    response.value[0] = '\0';
    response.pid = getpid();
    response.type = NOC;
    if(msgsnd(client_qid,&response,MAX_MSG_SIZE,0) != 0) {
        fprintf(stderr,"Couldn't send message to %d: %s\n",msg.pid,strerror(errno));
    }
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
    time(&rawtime); // seconds since epoch
    timeinfo = localtime(&rawtime); // local time-zone

    struct message response;
    strcpy(response.value,asctime(timeinfo)); // pretty-formatted string
    response.pid = getpid();
    response.type = TIME;
    if(msgsnd(client_qid,&response,MAX_MSG_SIZE,0) != 0) {
        fprintf(stderr,"Couldn't send message to %d: %s\n",msg.pid,strerror(errno));
    }
}
void handle_terminate(struct message trigger_msg) {
    printf("Terminating server. Responding to queued messages..\n");
    while(1) {
        struct msqid_ds stats;
        struct message msg;
        msgctl(server_qid, IPC_STAT, &stats);

        printf("Number of messages left: %zu\n",stats.msg_qnum);
        if(msgrcv(server_qid, &msg, MAX_MSG_SIZE, 0, IPC_NOWAIT) < 0) {
            break;
        }
        printf("%d messages: %s\n",msg.pid,msg.value);
        handle_message(msg);
    }
    struct message response;
    response.pid = getpid();
    response.type = TERMINATE;
    strcpy(response.value,"Server terminated.");

    for(int i=0;i<client_count;++i) {
        if(msgsnd(clients[i].qid,&response,MAX_MSG_SIZE,0) != 0) {
            fprintf(stderr,"Couldn't send message to %d: %s\n",clients[i].pid,strerror(errno));
        }
        kill(clients[i].pid,SIGRTMIN);
    }

    printf("Terminating server\n");
    exit(EXIT_SUCCESS);
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
            handle_default(msg);
            break;
    }
}

void sigint_handler(int signum) {
    for(int i=0;i<client_count;++i) {
        kill(clients[i].pid,SIGRTMIN);
    }
    printf("Server terminated.\n");
    exit(EXIT_SUCCESS);
}

int main() {
    atexit(exit_handler);
    signal(SIGINT,sigint_handler);

    key_t server_key = ftok(getenv(SERVER_PATH), SERVER_ID); // create the special server key
    server_qid = msgget(server_key, IPC_CREAT | 0600); // create the brand-new server queue

    printf("Server launched.\n");

    struct message msg;
    while(1) {
        if(msgrcv(server_qid, &msg, MAX_MSG_SIZE, 0, 0) < 0) {
            exit_program(EXIT_FAILURE,"Error while receiving messages from clients");
            break;
        }
        printf("%d messages: %s %s\n", msg.pid,COMMANDS_STR[msg.type],msg.value);
        handle_message(msg);
        sleep(1);
    }

    return 0;
}
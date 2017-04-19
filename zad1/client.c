//
// Created by Mrz355 on 19.04.17.
//

#include <errno.h>
#include "communication.h"

key_t private_qid; // client's private queue for receiving messages

void exit_program(int status, char *exit_message) {
    perror(exit_message);
    exit(status);
}

void exit_handler() {
    msgctl(private_qid, IPC_RMID, NULL);
}

int main() {
    atexit(exit_handler);

    int id_from_server;

    key_t server_key;
    key_t private_key;

    if((server_key = ftok(getenv(SERVER_PATH),SERVER_ID))==-1) {
        exit_program(EXIT_FAILURE,"Error while generating the server key");
    }
    if((private_key = ftok(".",getpid())) == -1) {
        exit_program(EXIT_FAILURE,"Error while generating the private key");
    }

    int server_qid;

    if((server_qid = msgget(server_key,0)) == -1) {
        exit_program(EXIT_FAILURE,"Error while retrieving queue from server");
    }
    if((private_qid = msgget(private_key, IPC_CREAT | 0600)) == -1) {
        exit_program(EXIT_FAILURE,"Error while creating private queue");
    }


    // send private key to server
    struct message msg;
    msg.type = LOGIN;
    msg.pid = getpid();
    printf("Sending key to server: %d\nMy queue id: %d\n",private_key,private_qid);
    sprintf(msg.value,"%d",private_key);
    if(msgsnd(server_qid, &msg, MAX_MSG_SIZE, 0)!=0) {
        fprintf(stderr,"Couldn't send message to server: %s\n",strerror(errno));
    }
    // receive id from server
    if(msgrcv(private_qid, &msg, MAX_MSG_SIZE, 0, 0) < 0) {
        exit_program(EXIT_FAILURE,"Error while receiving id from server");
    }
    id_from_server = atoi(msg.value);
    printf("Received id from server: %d\n",id_from_server);

    while(1) {
        char line[MAX_MSG_LEN];
        char type[10];
        msg.type = NOC;
        msg.pid = getpid();
        fgets(line,MAX_MSG_LEN,stdin);
        char *p = NULL;
        if((p = strtok(line," ")) == NULL) {
            fprintf(stderr,"Not valid message. Please pass type (ECHO, UPPER, TIME, TERMINATE) and after white-space character the message itself.\n");
            continue;
        }
        if(p[strlen(p)-1]=='\n') p[strlen(p)-1] = '\0';  // this is a case when TIME is used as a single command
        strcpy(type,p);

        for(int i=0;i<sizeof(COMMANDS_STR)/sizeof(COMMANDS_STR[0]);++i) {
            if(strcmp(type,COMMANDS_STR[i])==0) {
                msg.type = COMMANDS_ENUM[i];
            }
        }

        if(msg.type != TIME) {
            if((p = strtok(NULL,"\n")) == NULL) {
                fprintf(stderr,"Not valid message. Please pass type (ECHO, UPPER, TIME, TERMINATE) and after white-space character the message itself.\n");
                continue;
            }
            strcpy(msg.value,p);
        }

        if(msgsnd(server_qid, &msg, MAX_MSG_SIZE, 0)!=0) {
            fprintf(stderr,"Couldn't send message to server: %s\n",strerror(errno));
        }

        if(msgrcv(private_qid, &msg, MAX_MSG_SIZE, 0, 0) < 0) {
            exit_program(EXIT_FAILURE,"Error while receiving id from server");
            break;
        }

        if(msg.type == LOGOUT) {
            break;
        }
        printf("Received from server: %s\n",msg.value);
    }

    return 0;
}
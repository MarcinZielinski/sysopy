//
// Created by Mrz355 on 29.05.17.
//

#include "communication.h"

char *addr;
char *port_str;
char *username;
int protocol_family;
int socket_fd;

void exit_program(int status, char *msg) {
    if(status == EXIT_FAILURE) {
        perror(msg);
    } else {
        printf("%s\n",msg);
    }
    exit(status);
}

void error_check(int res, int error_number, char* msg, int exit) {
    if(res == error_number) {
        if(errno != 0)
            perror(msg);
        else
            fprintf(stderr,"%s\n",msg);
        if(exit) {
            exit_program(EXIT_FAILURE,msg);
        }
    }
}

void exit_handler() {
    if (socket_fd != -1) {
        error_check(shutdown(socket_fd, SHUT_RDWR), -1, "Error shutdowning socket", 0);
        error_check(close(socket_fd), -1, "Error closing socket", 0);
        if (protocol_family == AF_UNIX) {
            error_check(unlink(addr), -1, "Error unlinking unix socket", 0);
        }
    }
}


void logout() {
    //TODO: logout procedure
    exit(EXIT_SUCCESS);
}

void sigint_handler(int signum) {
    char* msg = "Sigint received\n";
    write(STDOUT_FILENO,msg,17);
    logout();
    exit(EXIT_SUCCESS);
}


void init_inet() {
    protocol_family = AF_INET;

    socket_fd = socket(AF_INET,SOCK_STREAM,0);
    error_check(socket_fd,-1,"Error creating INET socket",1);


    struct sockaddr_in in_addr;
    in_addr.sin_family = AF_INET;
    in_addr.sin_port = htobe16(atoi(port_str));
    error_check(inet_aton(addr,&in_addr.sin_addr),0,"Error converting address IP",1);


    error_check(connect(socket_fd, (const struct sockaddr *) &in_addr, sizeof(addr)), -1, "Error connecting to INET socket", 1);
}

void init_unix() {
    if(strlen(addr) > MAX_UNIX_PATH) {
        fprintf(stderr,"The unix socket path must be shorter than %d characters.\n",MAX_UNIX_PATH);
        exit(EXIT_SUCCESS);
    }
    protocol_family = AF_UNIX;

    socket_fd = socket(AF_UNIX,SOCK_STREAM,0);
    error_check(socket_fd,-1,"Error creating UNIX socket",1);


    struct sockaddr_un un_addr;
    un_addr.sun_family = AF_UNIX;
    strcpy(un_addr.sun_path,addr);

    error_check(connect(socket_fd, (const struct sockaddr *) &un_addr, sizeof(un_addr)), -1, "Error connecting to UNIX socket", 1);
}

void login() {
    msg_t msg;
    msg.type = LOGIN;
    strcpy(msg.msg.name,username);
    error_check((int) write(socket_fd, &msg, sizeof(msg)), -1, "Error sending username to socket", 1);
}

void send_result(task_t task) {
    msg_t msg;
    msg.type = RESULT;

    result_t result;
    result.id = task.id;

    switch (task.operation) {
        case ADD:
            result.result = task.a + task.b;
            break;
        case DIFF:
            result.result = task.a - task.b;
            break;
        case MUL:
            result.result = task.a * task.b;
            break;
        case DIV:
            result.result = task.a / task.b;
            break;
    }

    msg.msg.result = result;

    error_check((int) write(socket_fd,&msg,sizeof(msg)),-1,"Error sending back result so socket",1);
    printf("Sent!\n");
}

int main(int argc, char **argv) {
    if(argc != 4 && argc != 5) {
        exit_program(EXIT_SUCCESS,"Pass the proper number of arguments: username - your unique nickname, "
                "protocol_family - INET (internet conection) or UNIX (local connection), addr - server IPv4 or path to unix socket,"
                "port - port number (in case you've choosen INET connection)\n");
    }
    if(strlen(argv[1]) > MAX_NAME_LEN) {
        fprintf(stderr,"The username must be shorter than %d characters.\n",MAX_NAME_LEN);
        exit(EXIT_SUCCESS);
    }

    username = argv[1];
    addr = argv[3];

    if(strcmp(argv[2],"INET") == 0) {
        protocol_family = AF_INET;
        port_str = argv[4];
    } else if(strcmp(argv[2],"UNIX") == 0) {
        protocol_family = AF_UNIX;
    } else {
        exit_program(EXIT_SUCCESS,"You've specified wrong protocol family! Chose one between: UNIX and INET.");
    }

    atexit(exit_handler);
    struct sigaction sa;
    sa.sa_flags = 0;
    sigemptyset(&(sa.sa_mask));
    sa.sa_handler = sigint_handler;
    sigaction(SIGINT,&sa,NULL);


    if(protocol_family == AF_INET) {
        init_inet();
    } else {
        init_unix();
    }

    login();

    while(1) {
        msg_t msg;
        ssize_t bytes_read = recv(socket_fd, &msg, sizeof(msg), MSG_WAITALL);
        if (bytes_read == -1) {
            if (errno != EAGAIN && errno != EWOULDBLOCK) {
                perror("Error receiving message from server");
                //TODO: (2) I mean here
            }
            continue; // TODO: (1) check if this shouldn't be higher
        }

        switch (msg.type) {
            case TASK:
                printf("Got task! Sending answer to server...\n");
                fflush(stdout);
                send_result(msg.msg.task);
                break;
            case PING:
                printf("Pinged!\n");
                fflush(stdout);
                break;
            case LOGOUT:
                return 0;
            default:
                printf("Unsupported message type\n");
                break;
        }
    }

}
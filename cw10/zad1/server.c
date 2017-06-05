#include "communication.h"

char* unix_path;
int unix_fd = -1; // fd == socket
int inet_fd = -1;
in_port_t inet_port;
int efd = -1;
pthread_mutex_t mutex;
pthread_attr_t attr;
pthread_t netthread_tid;
pthread_t pingthread_tid;
struct epoll_event events[MAX_EVENTS];
client_t clients[MAX_CLIENTS];
int actual_clients;

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
    int des_res = pthread_attr_destroy(&attr);
    if(des_res != 0) error_check(des_res,des_res,"Error destroying thread attributes",0);
    des_res = pthread_mutex_destroy(&mutex);
    if(des_res!=0) error_check(des_res,des_res,"Error destroying thread mutex",0);
    error_check(pthread_cancel(netthread_tid),-1,"Error cancelling net thread",0);
    error_check(pthread_cancel(pingthread_tid),-1,"Error cancelling net thread",0);

    if(efd != -1) {
        error_check(close(efd),-1,"Error closing epoll",0);
    }
    if(unix_fd != -1) {
        error_check(shutdown(unix_fd, SHUT_RDWR),-1,"Error shutdowning unix socket",0);
        error_check(close(unix_fd),-1,"Error closing unix socket",0);
        error_check(unlink(unix_path),-1,"Error unlinking unix socket",0);
    }
    if(inet_fd != -1) {
        error_check(shutdown(inet_fd, SHUT_RDWR),-1,"Error shutdowning INET socket",0);
        error_check(close(inet_fd),-1,"Error closing INET socket",0);
    }
}

int set_non_blocking(int sfd)
{
    int flags, s;

    flags = fcntl (sfd, F_GETFL, 0);
    if (flags == -1)
    {
        perror ("fcntl");
        return -1;
    }

    flags |= O_NONBLOCK;
    s = fcntl (sfd, F_SETFL, flags);
    if (s == -1)
    {
        perror ("fcntl");
        return -1;
    }

    return 0;
}

void socket_init() {
    if(((inet_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)) {
        exit_program(EXIT_FAILURE,"Error creating INET socket");
    }

    struct sockaddr_in addr_in;
    addr_in.sin_addr.s_addr = INADDR_ANY;
    addr_in.sin_family = AF_INET;
    addr_in.sin_port = htobe16(inet_port);
    error_check(bind(inet_fd, (const struct sockaddr *) &addr_in, sizeof(addr_in)),-1,"Error binding INET socket",1);

    if((unix_fd = socket(AF_UNIX, SOCK_STREAM,0))== -1){
        exit_program(EXIT_FAILURE,"Error creating UNIX socket");
    }

    struct sockaddr_un addr_un;
    addr_un.sun_family = AF_UNIX;
    strncpy(addr_un.sun_path,unix_path, MAX_UNIX_PATH);

    error_check(bind(unix_fd, (const struct sockaddr *) &addr_un, sizeof(addr_un)),-1,"Error binding UNIX socket",1);

    error_check(listen(inet_fd,SOMAXCONN),-1,"Error preparing to listen on INET socket",1);
    error_check(listen(unix_fd,SOMAXCONN),-1,"Error preparing to listen on UNIX socket",1);

    set_non_blocking(inet_fd);
    set_non_blocking(unix_fd);
}

void epoll_init() {
    efd = epoll_create1(0);
    error_check(efd,-1,"Error creating epoll",1);
    struct epoll_event ee;
    ee.events = EPOLLIN | EPOLLRDHUP | EPOLLET;
    ee.data.fd = unix_fd;
    error_check(epoll_ctl(efd,EPOLL_CTL_ADD,unix_fd,&ee),-1,"Error registering epoll to unix socket",1);
    ee.events = EPOLLIN | EPOLLRDHUP | EPOLLET;
    ee.data.fd = inet_fd;
    error_check(epoll_ctl(efd,EPOLL_CTL_ADD,inet_fd,&ee),-1,"Error registering epoll to inet socket",1);
}


void close_client(int fd) {
    pthread_mutex_lock(&mutex);
    for (int i=0, j=0;i<actual_clients;++i,++j) {
        if (clients[i].fd == fd) {
            error_check(close(fd),-1,"Error while closing client",0);
            --j;
        } else {
            clients[j] = clients[i];
        }
    }
    --actual_clients;
    printf("%d disconnected\n> ",fd);
    fflush(stdout);
    pthread_mutex_unlock(&mutex);
}

int is_event_invalid(struct epoll_event event) {
    int fd = event.data.fd;
    if(event.events & EPOLLERR) {
        fprintf(stderr,"Event error\n");
        return -1;
    }
    if(event.events & EPOLLRDHUP || event.events & EPOLLHUP) { // client disconnected
        if(fd!=unix_fd && fd!=inet_fd)
            close_client(fd);
        return -1;
    }
    return 0;
}

int add_client(struct epoll_event event) {
    struct sockaddr new_addr;
    socklen_t new_addr_len = sizeof(new_addr);
    int client_fd = accept(event.data.fd, &new_addr, &new_addr_len);

    if(client_fd == -1) {
        if(errno !=EAGAIN && errno != EWOULDBLOCK) {
            error_check(client_fd,-1,"Error accepting new connection",0);
        }
        return -1;
    }

    if(set_non_blocking(client_fd) == -1) {
        return -1;
    }

    struct epoll_event client_event;
    client_event.data.fd = client_fd;
    client_event.events = EPOLLIN | EPOLLET;
    if(epoll_ctl(efd,EPOLL_CTL_ADD,client_fd,&client_event) == -1) {
        perror("Error while adding new socket to epoll");
        return -1;
    }
    pthread_mutex_lock(&mutex);
    clients[actual_clients++].fd = client_fd;
    pthread_mutex_unlock(&mutex);

    return 0;
}

int read_message(struct epoll_event event) {
    msg_t msg;
    ssize_t bytes_read = read(event.data.fd,&msg,sizeof(msg));
    if(bytes_read == -1) {
        if(errno !=EAGAIN && errno != EWOULDBLOCK) {
            perror("Error receiving message from client");
        }
        return -1;
    } else if(bytes_read == 0) { // End of file - client closed connection
        close_client(event.data.fd);
        return -1;
    }
    else {
        switch(msg.type) {
            case RESULT:
                printf("task(%d) - result: %d\n> ",msg.msg.result.id, msg.msg.result.result);
                break;
            case PONG:
                pthread_mutex_lock(&mutex);
                for(int i=0; i<actual_clients; ++i){
                    if(clients[i].fd == event.data.fd) {
                        clients[i].pongs++;
                    }
                }
                pthread_mutex_unlock(&mutex);
                break;
            case LOGIN:
                pthread_mutex_lock(&mutex);
                for(int i =0; i<actual_clients-1; ++i) {
                    if(strcmp(clients[i].name,msg.msg.name)==0) {
                        printf("User with the same username: %s, tried to login\n> ",msg.msg.name);
                        pthread_mutex_unlock(&mutex);
                        close_client(event.data.fd);
                        return -1;
                    }
                }
                strcpy(clients[actual_clients-1].name,msg.msg.name);
                pthread_mutex_unlock(&mutex);
                printf("%d connected. Username: %s\n> ",event.data.fd,msg.msg.name);
                break;
            case LOGOUT:
                close_client(event.data.fd);
                break;
            default:
                break;
        }
        fflush(stdout);
    }
    return 0;
}

void *sockets_handler(void *args) {
    socket_init();
    epoll_init();

    while(1) {
        int n = epoll_wait(efd,events,MAX_EVENTS,-1); // -1 = no timeout
        error_check(n,-1,"Error while waiting for events",0);

        for(int i = 0;i<n;++i) { // handle every event
            struct epoll_event event = events[i];

            if(is_event_invalid(event)) continue;

            if(event.data.fd == inet_fd || event.data.fd == unix_fd) {// if the data came from inet or unix socket, we've got new connection
                while(1) {
                    if(actual_clients == MAX_CLIENTS) {
                        fprintf(stderr,"Maximum number of clients reached");
                        break;
                    }
                    if(add_client(event) == -1) break;
                }
            } else { // if the data came from some other socket it must've been some connected user's socket
                while(1) {
                    if(read_message(event) == -1) break;
                }
            }
        }
    }

    return NULL;
}

void *ping_handler(void *args) {
    while(unix_fd == -1 && inet_fd == -1);
    while(1){
        pthread_mutex_lock(&mutex);
        for(int i=0; i<actual_clients; i++) {
            msg_t msg;
            msg.type = PING;

            error_check((int) write(clients[i].fd, &msg, sizeof(msg)), -1, "Error sending PING to client", 0);
            ++(clients[i].pings);
            pthread_mutex_unlock(&mutex);
            sleep(1);
            pthread_mutex_lock(&mutex);

            if (clients[i].pings != clients[i].pongs) {
                printf("%d has not responded to PING. Disconnecting...\n> ", clients[i].fd);
                fflush(stdout);
                for (int k=0, j=0;i<actual_clients;++i,++j) {
                    if (clients[k].fd == clients[i].fd) {
                        error_check(close(clients[i].fd),-1,"Error while closing client",0);
                        --j;
                    } else {
                        clients[j].fd = clients[k].fd;
                    }
                }
                --actual_clients;
                printf("%d disconnected\n> ",clients[i].fd);
                fflush(stdout);
            }
        }
        pthread_mutex_unlock(&mutex);
        sleep(2);
    }
    return NULL;
}

int random_client() {
    pthread_mutex_lock(&mutex);
    if (actual_clients == 0) return -1;
    int fd = clients[rand() % actual_clients].fd;
    return fd;
}

int send_task(task_t task) {
    int fd = random_client();
    if (fd == -1) {
        printf("No clients to send task to\n");
        pthread_mutex_unlock(&mutex);
        return -1;
    }
    pthread_mutex_unlock(&mutex);

    msg_t msg;
    msg.type = TASK;
    msg.msg.task = task;
    error_check((int) send(fd, &msg, sizeof(msg), 0), -1, "Error sending task to client", 0);
    return 0;
}

void sigint_handler(int signum) {
    char* msg = "Sigint received\n";
    write(STDOUT_FILENO,msg,17);
    exit(EXIT_SUCCESS);
}

int main(int argc, char **argv) {
    if(argc != 3) {
        exit_program(EXIT_SUCCESS,"Bad number of arguments - needed 2: port - UDP port number, socket_path - path to the UNIX socket.");
    }
    if(strlen(argv[2]) > MAX_UNIX_PATH) {
        fprintf(stderr,"Unix path too long. Pass sth smaller than %d characters",MAX_UNIX_PATH);
        exit(EXIT_SUCCESS);
    }

    atexit(exit_handler);
    struct sigaction sa;
    sa.sa_flags = 0;
    sigemptyset(&(sa.sa_mask));
    sa.sa_handler = sigint_handler;
    sigaction(SIGINT,&sa,NULL);

    srand((unsigned int) time(NULL));

    inet_port = (in_port_t) atoi(argv[1]);
    unix_path = argv[2];

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED);

    pthread_mutex_init(&mutex,NULL);

    pthread_create(&netthread_tid,&attr,sockets_handler,NULL);
    pthread_create(&pingthread_tid,&attr,ping_handler,NULL);


    puts("Pass the task to one of the clients. Format of the input stands as follows: a operation b. For example: 2 * 4. Possible operations (+, -, *, /). Press Ctrl+C to exit.\n");
    int task_id = 1;
    char symbol;
    int a,b,c;
    while(1) {
        printf("> ");

        int res = scanf("%d %c %d%*c", &a, &symbol, &b);

        if (res == EOF) {
            perror("Error reading from stdin");
            continue;
        }
        else if(res != 3) {
            while((c = getchar()) != '\n' && c != EOF);
            puts("Bad amount of parameters! Pass sth like: 2 + 3 (including whitespaces!!)");
            continue;
        }

        task_t task;
        task.id = task_id;
        task.a = a;
        task.b = b;
        switch (symbol) {
            case '+':
                task.operation = ADD;
                break;
            case '-':
                task.operation = DIFF;
                break;
            case '*':
                task.operation = MUL;
                break;
            case '/':
                task.operation = DIV;
                break;
            default:
                while((c = getchar()) != '\n' && c != EOF);
                printf("Wrong symbol!\n");
                continue;
        }
        if(send_task(task) == -1) continue;
        ++task_id;
    }
}
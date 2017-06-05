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
        error_check(close(unix_fd),-1,"Error closing unix socket",0);
        error_check(unlink(unix_path),-1,"Error unlinking unix socket",0);
    }
    if(inet_fd != -1) {
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
    if(((inet_fd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)) {
        exit_program(EXIT_FAILURE,"Error creating INET socket");
    }

    struct sockaddr_in addr_in;
    addr_in.sin_addr.s_addr = INADDR_ANY;
    addr_in.sin_family = AF_INET;
    addr_in.sin_port = htobe16(inet_port);
    error_check(bind(inet_fd, (const struct sockaddr *) &addr_in, sizeof(addr_in)),-1,"Error binding INET socket",1);

    if((unix_fd = socket(AF_UNIX, SOCK_DGRAM,0))== -1){
        exit_program(EXIT_FAILURE,"Error creating UNIX socket");
    }

    struct sockaddr_un addr_un;
    addr_un.sun_family = AF_UNIX;
    strncpy(addr_un.sun_path,unix_path, MAX_UNIX_PATH);

    error_check(bind(unix_fd, (const struct sockaddr *) &addr_un, sizeof(addr_un)),-1,"Error binding UNIX socket",1);

    set_non_blocking(inet_fd);
    set_non_blocking(unix_fd);
}

void epoll_init() {
    efd = epoll_create1(0);
    error_check(efd,-1,"Error creating epoll",1);
    struct epoll_event ee;
    ee.events = EPOLLIN | EPOLLET;
    ee.data.fd = unix_fd;
    error_check(epoll_ctl(efd,EPOLL_CTL_ADD,unix_fd,&ee),-1,"Error registering epoll to unix socket",1);
    ee.events = EPOLLIN | EPOLLET;
    ee.data.fd = inet_fd;
    error_check(epoll_ctl(efd,EPOLL_CTL_ADD,inet_fd,&ee),-1,"Error registering epoll to inet socket",1);
}


void close_client(char *name) {
    pthread_mutex_lock(&mutex);
    for (int i=0, j=0;i<actual_clients;++i,++j) {
        if (strcmp(clients[i].name, name)==0) {
            printf("%s disconnected\n> ",clients[i].name);
            fflush(stdout);
            free(clients[i].addr);
            --j;
        } else {
            clients[j] = clients[i];
        }
    }
    --actual_clients;
    pthread_mutex_unlock(&mutex);
}

int is_event_invalid(struct epoll_event event) {
    //int fd = event.data.fd;
    if(event.events & EPOLLERR) {
        fprintf(stderr,"Event error\n");
        return -1;
    }
//    if(event.events & EPOLLRDHUP || event.events & EPOLLHUP) { // client disconnected
//        if(fd!=unix_fd && fd!=inet_fd)
//            close_client(fd);
//        return -1;
//    }

    return 0;
}

client_t *find_client(char* name){
    for(int i=0; i<actual_clients; i++){
        if(strcmp(clients[i].name, name) == 0){
            return &clients[i];
        }
    }
    return NULL;
}

int add_client(struct sockaddr *addr, socklen_t addr_size, msg_t msg) {
    if(actual_clients == MAX_CLIENTS) return -1;
    if(find_client(msg.msg.name) != NULL) return -1;

    pthread_mutex_lock(&mutex);
    clients[actual_clients].addr = addr;
    strcpy(clients[actual_clients].name, msg.msg.name);
    clients[actual_clients].pings = 0;
    clients[actual_clients].pongs = 0;
    clients[actual_clients].s_type = msg.s_type;
    clients[actual_clients++].addr_size = addr_size;
    pthread_mutex_unlock(&mutex);

    return 0;
}

int read_message(struct epoll_event event) {
    struct sockaddr *addr = malloc(sizeof(struct sockaddr));
    socklen_t addr_size =  sizeof(struct sockaddr);
    msg_t msg;
    ssize_t bytes_read = recvfrom(event.data.fd,&msg,sizeof(msg),0,addr,&addr_size);
    if(bytes_read == -1) {
        if(errno !=EAGAIN && errno != EWOULDBLOCK) {
            perror("Error receiving message from client");
        }
        return -1;
    } else if(bytes_read == 0) {
        printf("Read 0 bytes\n> ");
        fflush(stdout);
        return -1;
    }
    else {
        switch(msg.type) {
            case RESULT:
                printf("task(%d) - result: %d\n> ",msg.msg.result.id, msg.msg.result.result);
                break;
            case PONG:
                pthread_mutex_lock(&mutex);
                    for(int i=0; i<actual_clients; i++){
                    if(strcmp(clients[i].name,msg.msg.name)==0) {
                        clients[i].pongs++;
                    }
                }
                pthread_mutex_unlock(&mutex);
                break;
            case LOGIN:
                printf("%d connected. Username: %s\n> ",event.data.fd,msg.msg.name);
                if(add_client(addr, addr_size, msg) == -1) {
                    free(addr);
                }
                break;
            case LOGOUT:
                close_client(msg.msg.name);
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

            while(1)
                 if(read_message(event) == -1) break;

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

            error_check((int) sendto(clients[i].s_type == AF_UNIX ? unix_fd : inet_fd, &msg, sizeof(msg), 0, clients[i].addr, clients[i].addr_size), -1, "Error sending PING to client", 0);
            ++(clients[i].pings);
            pthread_mutex_unlock(&mutex);
            sleep(1);
            pthread_mutex_lock(&mutex);

            if (clients[i].pings != clients[i].pongs) {
                printf("%s has not responded to PING. Disconnecting...\n> ", clients[i].name);
                fflush(stdout);
                for (int k=0, j=0;i<actual_clients;++i,++j) {
                    if (strcmp(clients[k].name, clients[i].name)==0) {
                        free(clients[i].addr);
                        --j;
                    } else {
                        clients[j] = clients[k];
                    }
                }
                --actual_clients;
                printf("%s disconnected\n> ",clients[i].name);
                fflush(stdout);
            }
        }
        pthread_mutex_unlock(&mutex);
        sleep(2);
    }
    return NULL;
}

int random_client_id() {
    pthread_mutex_lock(&mutex);
    if (actual_clients == 0) return -1;
    return rand() % actual_clients;
}

int send_task(task_t task) {
    int client_id = random_client_id();
    if (client_id == -1) {
        printf("No clients to send task to\n");
        pthread_mutex_unlock(&mutex);
        return -1;
    }
    pthread_mutex_unlock(&mutex);

    msg_t msg;
    msg.type = TASK;
    msg.msg.task = task;

    client_t client = clients[client_id];
    error_check((int) sendto(client.s_type == AF_UNIX ? unix_fd : inet_fd, &msg, sizeof(msg), 0, client.addr, client.addr_size), -1, "Error sending task to client", 0);
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

        int res = scanf("%d %c %d", &a, &symbol, &b);

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
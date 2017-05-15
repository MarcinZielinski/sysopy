#include "finals.h"

size_t k;
int file;
char* word;
int N;
pthread_t* threads;
pthread_mutex_t mutex;
pthread_t main_thread_id;

struct thread_args {
    int id;
} **args;


void exit_program(int status, char* message) {
    if(status == EXIT_SUCCESS) {
        printf("%s\n",message);
    } else {
        perror(message);
    }
    exit(status);
}

int seek_for_word(char *buffer) {
    char id_str[sizeof(int)], text[RECORDSIZE+1]; // +1 cause of null given in strcpy
    char *strtok_pointer;
    char strtok_buf[RECORDSIZE*k];
    strtok_pointer = strtok_buf;
    for(int i =0;i<k;++i) {
        char *p = strtok_r(buffer, SEPARATOR,&strtok_pointer);

        strcpy(id_str, p);
        int id = atoi(id_str);  // in id_str we've got the id, let's change it to int

        p = strtok_r(NULL, "\n",&strtok_pointer); // in p now we have the rest of the record, so called - text
        if(p!=NULL) {
            strncpy(text, p, RECORDSIZE+1);
            if (strstr(text, word) != NULL) {
                return id;
            }
        }
    }
    return -1;
}

void *parallel_reader(void *arg) {
    int id;
    char buffer[1024*k];
    struct thread_args *tmp = arg;
    int jump = tmp->id;
    long multiplier = RECORDSIZE*jump*k;

    while(pread(file,buffer,RECORDSIZE*k,multiplier) > 0) {
        if((id = seek_for_word(buffer)) != -1) {
            printf("Found the word %s! Record id: %d, thread id: %zu\n",word,id,pthread_self());
        }
        multiplier += (N*RECORDSIZE*k);
    }
    printf("End of thread life.\n");
    while(1);
}

void exit_handler() {
    if(file!=0) {
        close(file);
    }
    if(threads != NULL) {
        free(threads);
    }
    if(args !=NULL) {
        for (int i = 0; i < N; ++i) {
            if (args[i] != NULL)
                free(args[i]);
        }
        free(args);
    }
}

void init_mask() {
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGUSR1);
    sigaddset(&mask, SIGTERM);
    sigprocmask(SIG_BLOCK,&mask,NULL);
}

int get_signal(int n) {
    switch(n) {
        case 1:
            return SIGUSR1;
        case 2:
            return SIGTERM;
        case 3:
            return SIGKILL;
        default:
            return SIGSTOP;
    }
}
char *get_signal_str(int n) {
    switch(n) {
        case 1:
            return "SIGUSR1";
        case 2:
            return "SIGTERM";
        case 3:
            return "SIGKILL";
        default:
            return "SIGSTOP";
    }
}

int main(int argc, char ** argv) {
    if(argc != 6) {
        exit_program(EXIT_SUCCESS, "Pass 4 arguments: N - the number of threads, filename - the name of the file to read records from"
                "k - the number of records read by a thread in a single access, word - the word we seek in the file for, signal - the type of signal to send "
                "1 - SIGUSR1, 2 - SIGTERM, 3 - SIGKILL, 4 - SIGSTOP\n");
    }
    atexit(exit_handler);

    N = atoi(argv[1]);
    char *filename = argv[2];
    k = (size_t) atoi(argv[3]);
    if(k<=0 || N <= 0) {
        exit_program(EXIT_SUCCESS,"Pass the N and k parameters > 0");
    }
    word = argv[4]; // eg. "pellentesque";

    //init_signals();

    if((file = open(filename, O_RDONLY)) == -1) {
        exit_program(EXIT_FAILURE, "Couldn't open the file to read records from");
    }

    main_thread_id = pthread_self();
    pthread_mutex_init(&mutex,NULL);

    threads = malloc(sizeof(int)*N);
    args = malloc(sizeof(struct thread_args*)*N);

    for(int i=0;i<N;++i) {
        args[i] = malloc(sizeof(struct thread_args));
        args[i]->id = i;
        if(pthread_create(&threads[i],NULL,parallel_reader,args[i])) {
            exit_program(EXIT_FAILURE,"Failed to create thread");
        }
    }

    init_mask();

    int signal = atoi(argv[5]);
    printf("Sending %s to process...\n",get_signal_str(signal));
    kill(getpid(),get_signal(signal));
    printf("Sent!\n");

    pthread_mutex_destroy(&mutex);
    pthread_exit(0);
}

#include "fifo_sync.h"

int verbose;
int shutdown;
pthread_t *go_ahead;
struct fifo* fifo;
pthread_mutex_t parent, mutex, end_of_work_mutex, fifo_mutex;
pthread_cond_t end_of_work_cond, cond, parent_cond;
int readers, writer_busy, go_parent;

void init_mutexes_and_conds() {
    mutex = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
    end_of_work_mutex = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
    parent = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
    fifo_mutex = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
    cond = (pthread_cond_t)PTHREAD_COND_INITIALIZER;
    end_of_work_cond = (pthread_cond_t)PTHREAD_COND_INITIALIZER;
    parent_cond = (pthread_cond_t)PTHREAD_COND_INITIALIZER;
}

void exit_program(int status, char *msg) {
    if(status == EXIT_FAILURE) {
        perror(msg);
    } else {
        printf("%s\n",msg);
    }
    exit(status);
}

void exit_handler() {
    fifo_destroy(fifo);
}

void reading_l(int divider) {
    int count = 0;
    char *indexes_str = malloc(sizeof(char) * BUFFER_SIZE);
    char *numbers_str = malloc(sizeof(char) * BUFFER_SIZE);
    char *indexes = indexes_str;
    char *numbers = numbers_str;

    for(int i = 0;i<TAB_SIZE;++i) {
        if(tab[i] % divider == 0) {
            int to_move = sprintf(indexes_str,"%d ",i);
            indexes_str += to_move;
            to_move = sprintf(numbers_str,"%d ", tab[i]);
            numbers_str += to_move;
            ++count;
        }
    }
    printf("READER id: %zu, finished. Found numbers: %s at the indexes: %s (total: %d)\n",pthread_self(),numbers,indexes,count);
    fflush(stdin);
    free(indexes);
    free(numbers);
}

void writing_l() {
    char *indexes_str = malloc(sizeof(char) * BUFFER_SIZE);
    char *numbers_str = malloc(sizeof(char) * BUFFER_SIZE);
    char *indexes = indexes_str;
    char *numbers = numbers_str;

    for(int n = 0 ; n < TAB_SIZE ; ++n) {
        int index = rand()%2 ? n : -1;
        if(index == -1) continue;
        int value = rand()%TAB_SIZE;
        tab[index] = value;

        int to_move =sprintf(indexes_str,"%d ",index);
        indexes_str += to_move;
        to_move = sprintf(numbers_str,"%d ",value);
        numbers_str += to_move;
    }
    printf("WRITER id: %zu, finished. Changed to numbers: %s at the indexes: %s\n",pthread_self(), numbers,indexes);
    fflush(stdin);
    free(indexes);
    free(numbers);
}

void reading(int divider) {
    if(verbose) {
        reading_l(divider);
        return;
    }
    int count = 0;
    for(int i = 0;i<TAB_SIZE;++i) {
        if(tab[i] % divider == 0) {
            ++count;
        }
    }
    printf("READER, finished. Found numbers: %d\n",count);
    fflush(stdin);
}

void writing() {
    if(verbose) {
        writing_l();
        return;
    }
    for(int n = rand()%TAB_SIZE ; n >= 0 ; --n) {
        tab[rand()%TAB_SIZE] = rand();
    }
    printf("WRITER, finished.\n");
    fflush(stdin);
}

void wait_for_permission(struct thread_info info) {
    // Push myself to the fifo
    pthread_mutex_lock(&fifo_mutex);
    fifo_push(fifo, info);
    pthread_mutex_unlock(&fifo_mutex);

    // Lock the waiting-for-handle mutex
    pthread_mutex_lock(&mutex);

    // Signal the parent: "I'm entering the waiting state, handle me parent, after I release waiting-for-handle mutex!"
    pthread_mutex_lock(&parent);
    ++go_parent;
    pthread_cond_signal(&parent_cond);
    pthread_mutex_unlock(&parent);

    while (!go_ahead[info.tid]) {
        // Enter the waiting state, free the waiting-for-handle mutex for other threads and parent
        pthread_cond_wait(&cond, &mutex);
    }
    // You were released from the waiting state, you're free to work!
    pthread_mutex_unlock(&mutex);
}

void inform_about_finish(struct thread_info info) {
    // Work finished signal the parent about that
    pthread_mutex_lock(&end_of_work_mutex);
    if(info.function == WRITER) {
        writer_busy = 0;
    } else {
        --readers;
        if(readers == 0) writer_busy = 0;
    }
    go_ahead[info.tid] = 0;
    pthread_cond_signal(&end_of_work_cond);
    pthread_mutex_unlock(&end_of_work_mutex);
}

void *reader_handler(void *args) {
    struct reader_args *r_args = (struct reader_args*) args;

    struct thread_info info;
    info.tid = r_args->id;
    info.function = READER;

    int divider = r_args->divider;

    while(1) {
        wait_for_permission(info);
        reading(divider);
        inform_about_finish(info);
    }
}

void *writer_handler(void *args) {
    struct writer_args *w_args = (struct writer_args*) args;

    struct thread_info info;
    info.tid = w_args->id;
    info.function = WRITER;

    while(1) {
        wait_for_permission(info);
        writing();
        inform_about_finish(info);
    }
}

const char* sigint_received = "SIGINT received.\n";
void sigint_handler(int signum) {
    write(STDOUT_FILENO,sigint_received, strlen(sigint_received));
    shutdown = 1;
}

int main(int argc, char **argv) {
    atexit(exit_handler);
    srand((unsigned int)time(NULL));

    int R = rand()%TAB_SIZE+1;
    int W = rand()%TAB_SIZE+1;

    if(argc == 2) {
        if(strcmp(argv[1],"-i") == 0) {
            verbose = 1;
        }
    } else if(argc == 3) {
        R = atoi(argv[1]);
        W = atoi(argv[2]);
    } else if(argc == 4) {
        if(strcmp(argv[1],"-i") == 0) {
            verbose = 1;
        }
        R = atoi(argv[2]);
        W = atoi(argv[3]);
    }

    struct sigaction sa;
    sigemptyset(&(sa.sa_mask));
    sa.sa_flags=0;
    sa.sa_handler = sigint_handler;
    if(sigaction(SIGINT,&sa,NULL) == -1) {
        exit_program(EXIT_FAILURE, "Error while setting SIGINT handler");
    }

    go_ahead = malloc(sizeof(pthread_t) * (R+W));

    fifo = fifo_init();

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED);

    init_mutexes_and_conds();

    pthread_t *thread_ids = malloc(sizeof(pthread_t)*(R+W));
    struct reader_args **r_args = malloc(sizeof(struct reader_args*) * R);
    struct writer_args **w_args = malloc(sizeof(struct writer_args*) * W);

    int r=R,w=W;

    for(int i = 0;i<(R+W);++i) {
        if(r) {
            r_args[i] = malloc(sizeof(struct reader_args));
            r_args[i]->id = i;
            r_args[i]->divider = 4;
            if(pthread_create(&thread_ids[i], &attr, reader_handler, r_args[i])) {
                exit_program(EXIT_FAILURE,"Error while creating reader thread");
            }
            --r;
        }
        if(w) {
            w_args[i] = malloc(sizeof(struct writer_args));
            w_args[i]->id = i + R;
            if(pthread_create(&thread_ids[i+R], &attr, writer_handler, w_args[i])) {
                exit_program(EXIT_FAILURE,"Error while creating writing thread");
            }
            --w;
        }
    }
    struct thread_info info;
    while(!shutdown) {
        // Wait for the signal from worker threads
        pthread_mutex_lock(&parent);
        while (!go_parent) {
            pthread_cond_wait(&parent_cond, &parent);
        }
        // Some thread signals you, to begin to handle it
        --go_parent;
        pthread_mutex_unlock(&parent);

        // Some thread signaled you, so it must be waiting now in the queue, pop it
        pthread_mutex_lock(&fifo_mutex);
        info = fifo_pop(fifo);
        pthread_mutex_unlock(&fifo_mutex);

        // Check the function of the thread
        if (info.function == WRITER) {
            pthread_mutex_lock(&end_of_work_mutex);
            // Okay so we've got a writer here. Wait for all the threads to finish work, so the writer can go.
            while (writer_busy) {
                pthread_cond_wait(&end_of_work_cond, &end_of_work_mutex);
            }
            // Writer can go work, signal it about that
            writer_busy = 1;
            pthread_mutex_unlock(&end_of_work_mutex);

            pthread_mutex_lock(&mutex);
            go_ahead[info.tid] = 1;
            pthread_cond_broadcast(&cond);
            pthread_mutex_unlock(&mutex);

        } else {
            pthread_mutex_lock(&end_of_work_mutex);
            // We've got a reader. Wait for writer to finish writing. Otherwise if no one is writing, launch the reader.
            while (writer_busy) {
                pthread_cond_wait(&end_of_work_cond, &end_of_work_mutex);
            }
            ++readers;
            writer_busy = 1; // Writer cannot write right now! Because someone will be reading the data
            pthread_mutex_unlock(&end_of_work_mutex);

            pthread_mutex_lock(&mutex);
            go_ahead[info.tid] = 1;
            pthread_cond_broadcast(&cond);
            pthread_mutex_unlock(&mutex);
        }
    }

    for(int i =0;i<(W+R); ++i) {
        pthread_cancel(thread_ids[i]);
    }
    free(thread_ids);
    for(int i = 0; i<W; ++i) {
        free(w_args[i]);
    } for(int i = 0 ; i<R; ++i) {
        free(r_args[i]);
    } free(w_args);
    free(r_args);

    exit(0);
}
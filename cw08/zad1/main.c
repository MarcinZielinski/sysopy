#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <bits/signum.h>
#include <signal.h>
#include "finals.h"

size_t k;
int file;
char* word;
//FILE *file;
int N;
pthread_mutex_t mutex;

struct thread_args {
    int id;
};

struct data {
    int id;
    char test[1024];
};

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
    char **strtok_pointer = malloc(sizeof(char*));
    char strtok_buf[RECORDSIZE*k];
    *strtok_pointer = strtok_buf;
    for(int i =0;i<k;++i) {
        char *p = strtok_r(buffer, SEPARATOR,strtok_pointer);

        strcpy(id_str, p);
        int id = atoi(id_str);  // in id_str we've got the id, let's change it to int

        p = strtok_r(NULL, "\n",strtok_pointer); // in p now we have the rest of the record, so called - text
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
    printf("%zu, file_desc= %d, arg_passed = %d\n",pthread_self(),file,jump);
    //lseek(file,jump,SEEK_SET);

    int bytes_read;
    while((bytes_read=pread(file,buffer,RECORDSIZE*k,multiplier) > 0)) {
    //while(1) {
        //printf("Czytam: %ld\n",multiplier);
        //fseek(file,multiplier,SEEK_SET);
        //if((bytes_read = fread(buffer,RECORDSIZE,k,file)) <= 0) {
        //    break;
       //}

        //printf("%zu, I've read %zu bytes\n",pthread_self(),bytes_read);
        //pthread_mutex_lock(&mutex);
        if((id = seek_for_word(buffer)) != -1) {
            printf("Found the word %s! Record id: %d, thread id: %zu\n",word,id,pthread_self());//TODO: consider using pthread_getthreadid_np();
        }
        //pthread_mutex_unlock(&mutex);

        multiplier += (N*RECORDSIZE*k);
        //lseek(file,jump,SEEK_CUR);
    }
    //printf("%s\n",buffer);
    //printf("The end of my life :(\n");
    free(tmp);
    return 0;
}

void sigint_handler(int signum) {

}

int main(int argc, char ** argv) {
    if(argc != 5) {
        //TODO: uncomment exit_program(EXIT_FAILURE, "Pass 4 arguments: N - the number of threads, filename - the name of the file to read records from
        //TODO: k - the number of records read by a thread in a single access, word - the word we seek in the file for");
    }
    N = 3;
    char *filename = "../zad1/sample_file.db";
    k = 2;
    word = "pellentesque";

    if((file = open(filename, O_RDONLY)) == -1) {
        exit_program(EXIT_FAILURE, "Couldn't open the file to read records from");
    }
    //if((file = fopen(filename,"r"))==NULL)
     //   exit_program(EXIT_FAILURE, "Couldn't open the file to read records from");

    //char buffer[1024*k];
    //size_t bytes_read = fread(buffer,RECORDSIZE,k,file);
    //printf("Read: %zu, \n%s\n",bytes_read,buffer);

    //char *p = strtok(buffer,SEPARATOR);
    //p = strtok(NULL,"\n");
    signal(SIGINT,sigint_handler);

    pthread_mutex_init(&mutex,NULL);
    printf("Let's create some threads...\n");
    pthread_t tid;
    for(int i=0;i<N;++i) {
        struct thread_args *args= malloc(sizeof(struct thread_args));
        args->id = i;
        if(pthread_create(&tid,NULL,parallel_reader,args)) {
            free(args);
        }
    }

    pause();
    pthread_mutex_destroy(&mutex);

    return 0;
}
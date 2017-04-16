//
<<<<<<< HEAD
// Created by Mrz355 on 23.03.17.
//

#define _XOPEN_SOURCE 1000
#include <ftw.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <unistd.h>
#include <wait.h>
#include <sys/resource.h>
#include <errno.h>

extern char ** environ; // array of all the environment variables
char * PATH;    // $PATH variable
char * currentPath; // path to the file we currently process

int fn(const char *fpath, const struct stat *sb, int tflag, struct FTW *ftwbuf) {
    if(tflag == FTW_F) { // we have file
        if(strcmp(fpath,currentPath)==0) {
            return -15;
        }
    } else {
        return 0;
    }
    return 0;
}

char* nftwSearch(char* dirPath) {
    int result = nftw(dirPath,fn,50,FTW_PHYS);
    if (result == -1) {
        perror("Nftw not successful");
        exit(EXIT_FAILURE);
    } else if (result == -15) {
        return currentPath;
    } else {
        return NULL;
    }
}

char ** splitArguments(char* str) {
    int argumentsNumber = 1; // always one argument when executing a program
    if(str!=NULL) {
        while (str[0] == ' ') ++str;
    }
    size_t charSize = sizeof(char*);
    char** res = malloc(charSize);
    char* env = NULL;
    char* save1, *save2;
    char* p = strtok_r(str," ",&save1);
    while(p != NULL) {

        if(p[0]=='$') {
            env = strdup(p+1);
            if ((env = getenv(env)) == NULL) {
                p = strtok_r(NULL," ",&save1);

            } else {
                env = strtok_r(env, " ", &save2);
                while (env != NULL) {
                    ++argumentsNumber;
                    res = realloc(res, argumentsNumber * charSize);
                    res[argumentsNumber - 1] = strdup(env);
                    env = strtok_r(NULL, " ", &save2);
                }
            }
            p = strtok_r(NULL," ",&save1);
            continue;
        }

        ++argumentsNumber;
        res = realloc(res,argumentsNumber*charSize);
        res[argumentsNumber-1] = strdup(p);
        p = strtok_r(NULL," ",&save1);
    }
    res = realloc(res,(argumentsNumber+1)*charSize);
    res[argumentsNumber] = NULL;

    return res;
}
char *searchFile(char *programName) {
    if(programName==NULL) return NULL;

    //search the folder containing first
    if(programName[0] == '/' || programName[0]=='.') {
        char* absPath = malloc(PATH_MAX);
        realpath(programName,absPath); // programName is sth like ls for example
        absPath = strtok(absPath,"\n");
        if(access(absPath, F_OK) != -1 ) {
            if (access(absPath, X_OK) != -1) {
                return absPath;
            }
        }
        fprintf(stderr,"Couldn't find file: %s",programName);
        exit(EXIT_FAILURE);
    }

    // searching PATH variable
    char* str = strdup(PATH);
    char *p = strtok(str,":");
    while(p) {
        currentPath = strdup(p);
        strcat(currentPath,"/");
        strcat(currentPath,programName);

        if(nftwSearch(p) != NULL) {
            return currentPath;
        }


        p = strtok(NULL,":");
    }

    return NULL;
}

void displayChildUsage(pid_t pid,char* programName) {
    struct rusage* usage = malloc(sizeof(struct rusage));
    if(getrusage(RUSAGE_CHILDREN,usage) == 0) {
        printf("\nProces PID: %d, program: %s",pid,programName);
        printf("\nSystem time usage: %zu ms", usage->ru_stime.tv_usec);
        printf("\nUser time usage: %zu ms\n", usage->ru_utime.tv_usec);
    } else {
        perror("Couldn't get the usage of child process");
    }
}

void parseLine(char *line, rlim_t virtual_limit, rlim_t cpu_limit) { // line > 1
    char *str;

    while(line[0]==' ') { // delete the spaces before command
        line++;
        if(line[0]=='\0') {
            return; // line filled with spaces, nothing to do here - return
        }
    }

    str = strtok(line, " ");
    if(line[0] == '#') { // str = #variable (in line we have the left side of the variable)
        str+=1;
        char* variableName = str;

        if((str=strtok(NULL,"\n")) == NULL) {
            unsetenv(variableName);
        } else {
            setenv(variableName,str,1); // non zero = override

            PATH = getenv("PATH"); // path may changed, so we need to update it in our program
        }
    } else {
        char* programName = str;

        str = strtok(NULL,"\n");

        char* programPath = searchFile(line);
        if(programPath != NULL) {
            char **argv = splitArguments(str);
            argv[0] = programName;

            pid_t child = fork();
            if(child == 0) {
                int result1, result2;
                struct rlimit *limit = malloc(sizeof(struct rlimit));
                limit->rlim_max = virtual_limit;
                limit->rlim_cur = virtual_limit;
                result1 = setrlimit(RLIMIT_AS,limit);
                limit->rlim_max = cpu_limit;
                limit->rlim_cur = cpu_limit;
                result2 = setrlimit(RLIMIT_CPU,limit);
                if(result1 != 0 || result2 != 0) {
                    perror("Cannot set given limits to the PC resources");
                    exit(EXIT_FAILURE);
                }
                free(limit);
                if(execv(programPath, argv) == -1) {
                    perror("\nProgram failed to execute");
                    exit(EXIT_FAILURE);
                }
            }
            int status;
            wait(&status);
            if(status != 0) {
                fprintf(stderr,"\nProgram \"%s\" execution failed\n",programName);
                exit(EXIT_FAILURE);
            }

            displayChildUsage(child,programName);

        } else {
            fprintf(stderr,"\nCouldn't find program: %s",programName);
            exit(EXIT_FAILURE);
        }
    }
}

long stol(char* str) {
    char *endptr;
    errno = 0;

    long res = strtol(str,&endptr,10);

    if ((errno == ERANGE && (res == LONG_MAX || res == LONG_MIN))
        || (errno != 0 && res == 0)) {
        perror("Cannot convert argument to string\n");
        exit(EXIT_FAILURE);
    }
    if (endptr == str) {
        fprintf(stderr, "No digits were found\n");
        exit(EXIT_FAILURE);
    }
    return res;
}

int main(int argc, char **argv) {
    if(argc!=4) {
        fprintf(stderr,"Not a valid number of arguments! 3 required - Path to file to interpret, maximum virtual memory usage (MB) and maximum CPU time usage (seconds).\n");
        exit(EXIT_FAILURE);
    }
    char* dirPath = argv[1];

    long virtual_limit = stol(argv[2]);
    long cpu_limit = stol(argv[3]);

    virtual_limit*=1e6; // user passed the value in MegaBytes

    FILE * file1;
    if((file1 = fopen(dirPath,"r")) == NULL) {
        printf("\nCannot open file %s\n",dirPath);
        exit(EXIT_FAILURE);
    }

    fseek(file1,0,SEEK_END);
    if(ftell(file1) <= 0) {
        printf("Empty File");
        return 0;
    }
    fseek(file1,0,SEEK_SET);

    ssize_t read;
    char * line;
    size_t len;

    while ((read = getline(&line, &len, file1)) != -1) {
        if(read > 1) {  // > 1, because of the endline character
            parseLine(line, (rlim_t) virtual_limit, (rlim_t) cpu_limit);
        }
    }

    return 0;
=======
// Created by Mrz355 on 30.03.17.
//
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <values.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#include <wait.h>

int N; // number of forked processes
int K; // number of needed requests
pid_t parent; // parent (main program) pid
volatile sig_atomic_t permission = 0; // the permission to grant
volatile sig_atomic_t requests = 0; // number of requests

struct childStruct {
    pid_t pid;
    int usr1;
    int rt;
} *childs;



int stoi(char* str) {
    char *endptr;
    errno = 0;

    int res = (int)strtol(str,&endptr,10);

    if ((errno == ERANGE && (res == LONG_MAX || res == LONG_MIN))
        || (errno != 0 && res == 0)) {
        perror("Cannot convert argument to int\n");
        exit(EXIT_FAILURE);
    }
    if (endptr == str) {
        fprintf(stderr, "No digits were found\n");
        exit(EXIT_FAILURE);
    }
    return res;
}

void killAll(int signum) {
    int pid = getpid();
    if(pid != parent) {
        for(int i=0;i<N;++i) {
            if(childs[i].pid == pid) {
                childs[i].usr1 = 2;
            }
        }
        exit(255);
    }
    else {
        printf("\nExiting program.\n");
        sleep(1);
        printf("\nEXIT\n");
        exit(EXIT_SUCCESS);
    }
}
void sigHandler(int signum, siginfo_t *value, void *extra) {
    ++requests;

    childs[requests-1].pid = value->si_value.sival_int;
    childs[requests-1].usr1 = signum;
    if (requests > K) {
        kill(childs[requests-1].pid,SIGALRM);
    } else if (requests == K) {
        for (int i = 0; i < requests; ++i) {
            kill(childs[i].pid,SIGALRM);
        }
    }
    return;
}
void childHandler(int signum) {
    permission = 1;
}
volatile sig_atomic_t realReceived = 0;
void sigRealHandler(int signum, siginfo_t *value, void *extra) {
    childs[realReceived].rt=signum;
    ++realReceived;
    //printf("\nReal-time signal received: %d from %zu\n", signum, value->si_value);
    return;
}
int main(int argc, char **argv) {
    if(argc != 3) {
        fprintf(stderr,"\nYou must specify two arguments. N - number of child processes and K - number of request needed.\n");
        exit(EXIT_FAILURE);
    }
    N = stoi(argv[1]); // number of children
    K = stoi(argv[2]); // number of needed requests

    if(K > N) {
        fprintf(stderr, "\nThe number of requests mustn't be greater than the processes number.\n");
        exit(EXIT_FAILURE);
    }
    childs = calloc((size_t) N, sizeof(childs));

    //USR1 signal handle
    struct sigaction actionStruct;
    actionStruct.sa_flags = SA_SIGINFO | SA_NODEFER;
    actionStruct.sa_sigaction = &sigHandler;
    sigfillset(&actionStruct.sa_mask);
    sigdelset(&actionStruct.sa_mask,SIGUSR1);
    if(sigaction(SIGUSR1, &actionStruct, NULL) == 0) {
        printf("Sigaction completed.\n");
    }
    else {
        perror("\nError with sigaction\n");
    }

    //real-time signal handle
    struct sigaction realActionStruct;
    realActionStruct.sa_flags = SA_SIGINFO;
    realActionStruct.sa_sigaction = &sigRealHandler;
    sigfillset(&realActionStruct.sa_mask);
    for(int i = SIGRTMIN; i < SIGRTMIN+32;++i) {
        sigdelset(&realActionStruct.sa_mask,i);
        sigaction(i,&realActionStruct,NULL);
        sigaddset(&realActionStruct.sa_mask,i);
    }

    //interrupt signal handle
    signal(SIGINT,killAll);
    signal(SIGALRM,childHandler); // listen for alarm

    parent = getpid();
    pid_t child = 1;
    for(int i=0; i<N; ++i) {
        if((child = fork())==0) {
            break;
        }
    }
    if(child == 0) {
        srand((unsigned int) (time(NULL) ^ (getpid() << 16)));
        unsigned int sleepTime = (unsigned int) (rand() % 11);
        sleep(sleepTime);

        union sigval *value = malloc(sizeof(union sigval));

        value -> sival_int = getpid();

        sigqueue(parent, SIGUSR1, *value); // send SIGUSR1 with own PID

        clock_t before = clock();

        while(permission == 0) { // wait for permission
            if((clock()-before)/CLOCKS_PER_SEC > 10) {
                printf("\nNo response from parent. Signal probably overwriten.\n");
                raise(SIGINT);
            }
        }

        //send realtimesignal
        int realTimeSignal = SIGRTMIN + (rand() % 32);
        value -> sival_int = getpid();
        sigqueue(parent,realTimeSignal, *value);

        int diff = (int)((clock() - before) / (CLOCKS_PER_SEC));
        free(value);

        exit(diff);
    }

    int status = 0;
    int pid = 0;
    int counter = 0;
    while(counter<N) { // wait for signals
        if((pid = wait(&status))!=-1) {
            if(WEXITSTATUS(status)==255) {
                printf("\nChildren finished working with SIGINT. PID: %d, EXIT CODE: %d\n", pid, WEXITSTATUS(status));
            } else {
                printf("\nChildren finished working. PID: %d, 1st signal: %d, 2nd signal: %d, EXIT CODE: %d\n", pid,
                       childs[counter].usr1, childs[counter].rt, WEXITSTATUS(status));
            }
            ++counter;
        }
    }
    printf("\nAll processes finished job, exiting program\n");
    sleep(1);
    printf("\nEXIT\n");
    exit(EXIT_SUCCESS);
>>>>>>> cw04/master
}
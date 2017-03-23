//
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

extern char ** environ;

char * PATH;


char * currentPath;
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

    char* p = strtok(str," ");
    while(p != NULL) {
        ++argumentsNumber;
        res = realloc(res,argumentsNumber*charSize);
        res[argumentsNumber-1] = strdup(p);
        p = strtok(NULL," ");
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
        printf(stderr,"Couldn't find file: %s",programName);
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
char **replaceArgumentsWithEnvironmentVariables(char **argv) {
    if(argv[1] == NULL) return argv; // argv always end with NULL arg

    int i = 1;
    while(argv[i] != NULL) {
        if(argv[i][0] == '$') { // out string coinains at least one character
            if(argv[i][1] != '\0') { // if this isn't only $ we can change it to environment variable
                char* tmp = argv[i];
                ++tmp;

                char* variable = getenv(tmp);
                argv[i] = variable;
            }
        }
        ++i;
    }
    return argv;
}

void displayChildUsage(pid_t pid,char* programName) {
    struct rusage* usage = NULL;
    if(getrusage(RUSAGE_CHILDREN,usage) == 0) {
        printf("\nProces PID: %d, program: %s",pid,programName);
        printf("\nSystem time usage: %zu", usage->ru_stime.tv_sec);
        printf("\nUser time usage: %zu\n", usage->ru_utime.tv_sec);
    } else {
        perror("Couldn't get the usage of child process.");
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
            printf("\nUnsetting variable: %s",variableName);
        } else {
            setenv(variableName,str,1); // non zero = override
            printf("\nSetting variable: %s=%s",variableName,str);

            PATH = getenv("PATH");
        }
    } else {
        char* programName = str;

        str = strtok(NULL,"\n");

        char* programPath = searchFile(line);
        if(programPath != NULL) {
            char **argv = splitArguments(str);
            argv[0] = programName;
            argv = replaceArgumentsWithEnvironmentVariables(argv);
            pid_t child = fork();
            if(child == 0) {
                int result1, result2;
                struct rlimit *limit;
                limit->rlim_max = virtual_limit;
                result1 = setrlimit(RLIMIT_AS,limit);
                limit->rlim_max = cpu_limit;
                result2 = setrlimit(RLIMIT_CPU,limit);
                if(result1 != 0 || result2 != 0) {
                    perror("Cannot set given limits to the PC resources");
                    exit(EXIT_FAILURE);
                }
                free(limit);
                execve(programPath, argv, environ);
            }
            int status;
            wait(&status);
            if(status != 0) {
                printf(stderr,"\nProgram \"%s\" execution failed\n",programName);
                exit(EXIT_FAILURE);
            }

            displayChildUsage(child,programName);

        } else {
            printf(stderr,"\nCouldn't find program: %s",programName);
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
        printf(stderr,"Not a valid number of arguments! 3 required - Path to file to interpret, maximum virtual memory usage and maximum cpu time usage.\n");
        exit(EXIT_FAILURE);
    }
    char* dirPath = argv[1];

    long virtual_limit = stol(argv[2]);
    long cpu_limit = stol(argv[3]);

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
}
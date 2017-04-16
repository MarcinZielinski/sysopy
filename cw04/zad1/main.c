<<<<<<< HEAD
#define _XOPEN_SOURCE 1000
#include <ftw.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <unistd.h>
#include <wait.h>


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
        perror("Nftw non successful");
        exit(EXIT_FAILURE);
    } else if (result == -15) {
        return currentPath;
    } else {
        return NULL;
    }
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
        printf("Couldn't find file: %s",programName);
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
void parseLine(char *line) { // line > 1
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
            unsetenv(variableName); // this function doesn't do anything to environment, when variable with given name doesn't exist
        } else {
            setenv(variableName,str,1); // non zero = override
            PATH = getenv("PATH");
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
                execve(programPath, argv, environ);
            }
            int status;
            wait(&status);
            if(status != 0) {
                printf("\nProgram \"%s\" execution failed\n",programName);
                exit(EXIT_FAILURE);
            }
        } else {
            printf("\nCouldn't find program: %s",programName);
            exit(EXIT_FAILURE);
        }
    }
}

int main(int argc, char **argv) {
    if(argc!=2) {
        printf("Not a valid number of arguments! 1 required - Path to file to interpret.\n");
        exit(EXIT_FAILURE);
    }
    char* dirPath = argv[1];

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
            parseLine(line);
        }
    }
=======
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>

int reverse = 0;

void printLetters() {
    char c = 65;
    int counter = 0;
    while(1) {
        if (reverse) {
            if (--c == 64) c = 90;
            putchar(c);
        } else {
            putchar(c);
            if (++c == 91) c = 65;
        }
        if(++counter==26) {
            putchar('\n');
            counter = 0;
        }
    }
}

void tstpHandler(int signal) {
    reverse = reverse == 0 ? 1 : 0;
}

void intHandler(int signal) {
    printf("\nReceived SIGINT signal\n");
    exit(0);
}

int main() {
    struct sigaction actionStruct;
    actionStruct.sa_handler = tstpHandler;
    sigemptyset(&actionStruct.sa_mask);
    actionStruct.sa_flags = 0;
    if(sigaction(SIGTSTP, &actionStruct, NULL) == -1){
        perror("\nError while receiving the signal with sigaction() function\n");
    }

    if(signal(SIGINT,intHandler) == SIG_ERR) {
        perror("\nError while receiving the signal with signal() function\n");
    }

    printLetters();

>>>>>>> cw04/master

    return 0;
}
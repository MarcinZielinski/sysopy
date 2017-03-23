#define _XOPEN_SOURCE 1000
#include <ftw.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <unistd.h>
#include <wait.h>


extern char ** environ;

char * PATH;

void searchPath() {
    int i = 0;
    while(environ[i]!=NULL) {
        char * tmp = strstr(environ[i],"PATH=");
        if(environ[i]==tmp) { // if the string begins with "PATH=..." then we've got it
            tmp+=5;
            PATH = tmp;
        }
        ++i;
    }
}
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
        perror("Nftw non successful");
        exit(EXIT_FAILURE);
    } else if (result == -15) {
        return currentPath;
    } else {
        return NULL;
    }
}

void skipSpaces(char** str) {
    int i,x;
    for(i=x=0; str[i]; ++i)
        if(*str[i] != ' ' || (i>0 && *str[i-1]!=' '))
            str[x++] = str[i];
    str[x] = '\0';
}

char ** splitArguments(char* str) {
    if(!str) return NULL;

    int argumentsNumber = 1; // always one argument when executing a program (
    while(str[0] == ' ') ++str;

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
        if(access(absPath, F_OK) != -1 ) {
            if (access(absPath, X_OK) != -1) {
                return absPath;
            }
        }
        printf("Couldn't find file: %s",programName);
        exit(EXIT_FAILURE); // TODO: Zrobic tutaj zeby potomek zwracal zly status czy cos
    }

    // searching PATH variable
    char* str = strdup(PATH);
    char *p = strtok(str,":");
    char *res = NULL;
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

void parseLine(char *line, int read) {
    char *str;

    while(line[0]==' ') { // delete the spaces before command
        line++;
        if(line[0]=='\0') {
            return; // line filled with spaces, nothing to do here - return
        }
    }

    str = strtok(line, " ");
    if(line[0] == '#') {
        // str = #variable (in line we have the left side of the variable)

        str+=1;
        char* variableName = str;

        if((str=strtok(NULL,"\n")) == NULL) {
            unsetenv(variableName);
            printf("\nUnsetting variable: %s",variableName);
        } else {
            setenv(variableName,str,1); // non zero = override
            printf("\nSetting variable: %s=%s",variableName,str);

            searchPath();
        }
    } else {
        char* programName = str;

        str = strtok(NULL,"\n");





        //pid_t childPID = fork();
        char* programPath = searchFile(line);
        if(programPath != NULL) {
            char **argv = splitArguments(str);
            argv[0] = programName;
            pid_t child = fork();
            if(child == 0) {
                execve(programPath, argv, environ);
            }
            int status = wait(&status);
            if(status != 0) {
                printf("\nProgram \"%s\" execution failed\n",programName);
                //exit(EXIT_FAILURE);
            }
        } else {
            printf("\nCouldn't find program: %s",programName);
            exit(EXIT_FAILURE);
        }
    }
}

int main(int argc, char **argv) {
    char* dirPath = "../zad1/andrzejek";

    FILE * file1;
    if((file1 = fopen(dirPath,"r")) == NULL) {
        printf("Cannot open file %s",dirPath);
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
            parseLine(line, (int) read);
        }
    }

    searchPath();
    nftwSearch(dirPath);

    return 0;
}
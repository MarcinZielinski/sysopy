//
// Created by Mrz355 on 19.03.17.
//

#include <string.h>
#include <stdio.h>
#include <dirent.h>
#include <stdlib.h>
#include <time.h>
#include <linux/limits.h>


char* getPriviliages(__mode_t priviliages, int isFile) {
    int k = (int) priviliages;
    int length = 12;
    char* res = malloc(sizeof(char)*length);
    res+=length;
    *res = NULL;
    res++;
    int i = 0;
    while(k>0) {
        int priv = k%8;
        int counter = 0;
        while(priv>0) {
            int bit = priv%2;
            switch(counter) {
                case 0:
                    if(bit==1) *res = 'x'; else *res = '-';
                    break;
                case 1:
                    if(bit==1) *res = 'w'; else *res = '-';
                    break;
                case 2:
                    if(bit==1) *res = 'r'; else *res = '-';
                    break;
                default:
                    printf("Program failure");
                    exit(EXIT_FAILURE);
            }
            res-=1;
            ++i;
            counter++;
            priv/=2;
        }
        k/=8;
    }
    if(isFile != 0) {
        *res = '-';
    } else {
        *res = 'd';
    }
    return res;
}

char *getDate(__time_t timespec) {
    char *res = malloc(sizeof(char)*30);
    time_t timeToConvert = timespec;
    struct tm *timeinfo;
    timeinfo = localtime(&timeToConvert); // applying time to structure
    strftime(res,30,"%c",timeinfo);

    return res;
}

void sysSearch(char* dirPath, int bytes) {
    DIR* dir1 = opendir(dirPath);
    if(dir1 == NULL) {
        printf("Couldn't open directory %s",dirPath);
        exit(EXIT_FAILURE);
    }

    struct dirent* next;

    while((next = readdir(dir1)) != NULL) {
        struct stat* statistics = malloc(sizeof(struct stat));
        char* fileName = next->d_name;

        if(strcmp(fileName,".")==0 || strcmp(fileName,"..") == 0) {
            continue;
        }

        char* absPath = malloc((strlen(dirPath)+255)*sizeof(char));

        strcpy(absPath,dirPath); // eg. /home/dir
        strcat(absPath,"/");     // eg. /home/dir/
        strcat(absPath,fileName); // eg. /home/dir/file

        int res = stat(absPath,statistics);
        if(res>=0 && S_IFREG(statistics->st_mode)!=0) { // if found a file
            int fileSize = (int) statistics->st_size;
            if(fileSize <= bytes) {     //with less size than specified, go and printf it
                __mode_t priviliages = ((statistics->st_mode) & 07777);
                char* prettyPrivil = getPriviliages(priviliages,1);
                char* date = getDate(statistics->st_mtime);
                printf("Path: %s\n",absPath);
                printf("%s %d bytes, %s\n",prettyPrivil,fileSize,date);
            }
        }

        if(res>=0 && S_IFDIR(statistics->st_mode)!=0 && S_IFLNK(statistics->st_mode)==0) { // if found dir, not a symbolic link, go into
            sysSearch(absPath,bytes);
        }

        free(statistics);
    }

    closedir(dir1);
}
int stoi(char *s) {
    int res = 0;
    int n;
    for(n=0;s[n]!='\0';++n);
    for(int i=n-1,multiplier=1;i>=0;--i) {
        res += (s[i]-48)*multiplier;
        multiplier*=10;
    }
    return res;
}
int main(int argc, char** argv) {
    if(argc != 3) {
        printf("Wrong number of parameters. (eg. ./program ./documents/dir 5000)");
        exit(EXIT_FAILURE);
    }

    char* absPath = malloc(PATH_MAX);
    realpath(argv[1],absPath);

    int bytes = stoi(argv[2]);
    sysSearch(absPath,bytes);

    return 0;
}
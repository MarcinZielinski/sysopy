//
// Created by Mrz355 on 19.03.17.
//

#define _XOPEN_SOURCE 500
#include <ftw.h>
#include <string.h>
#include <stdio.h>
#include <dirent.h>
#include <stdlib.h>
#include <time.h>


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

void sysSearch(char* dirPath, int bytes, int tabsNumber) {
    DIR* dir1 = opendir(dirPath);
    if(dir1 == NULL) {
        printf("Couldn't open directory sys");
        exit(EXIT_FAILURE);
    }

    char* tabs = malloc(sizeof(char)*tabsNumber);
    for(int i=0;i<tabsNumber;++i) tabs[i]='\t';

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
        if(res>=0 && ((statistics->st_mode) & S_IFMT)==S_IFREG) { // if found a file
            int fileSize = (int) statistics->st_size;
            if(fileSize <= bytes) {     //with less size than specified, go and printf it
                __mode_t priviliages = ((statistics->st_mode) & 07777);
                char* prettyPrivil = getPriviliages(priviliages,1);
                char* date = getDate(statistics->st_mtime);
                printf("%sPath: %s\n",tabs,absPath);
                printf("%s%s %d bytes, %s\n",tabs,prettyPrivil,fileSize,date);
            }
        }

        if(res>=0 && ((statistics->st_mode) & S_IFMT)==S_IFDIR && ((statistics->st_mode) & S_IFMT)!=S_IFLNK) { // if found dir, not a symbolic link, go into
            sysSearch(absPath,bytes,tabsNumber+1);
        }

        free(statistics);
    }

    closedir(dir1);
}

int bytes_limit;

int fn(const char *fpath, const struct stat *sb, int tflag, struct FTW *ftwbuf) {
    if(tflag == FTW_F) { // we have file
        int fileSize = (int) sb->st_size;
        if(fileSize <= bytes_limit) {
            printf("Path: %s\n",fpath);
            __mode_t priviliages = ((sb->st_mode) & 07777);
            char* prettyPrivil = getPriviliages(priviliages,1);
            char* date = getDate(sb->st_mtime);
            printf("%s %d bytes, %s\n",prettyPrivil,fileSize,date);
        }
    }

    return 0;
}

void nftwSearch(char* dirPath) {
    int result = nftw(dirPath,fn,50,FTW_PHYS);
    if (result != 0) {
        printf("Nftw non successful");
        exit(EXIT_FAILURE);
    }
}

int main() {
    char* dirPath = "../zad1";
    int bytes = 5000;
    bytes_limit = bytes;
    nftwSearch("/home/Mrz355/CLionProjects/ZielinskiMarcin/cw02/zad1");

    return 0;
}
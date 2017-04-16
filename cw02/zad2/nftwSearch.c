//
// Created by Mrz355 on 19.03.17.
//

#define _XOPEN_SOURCE 500
#include <ftw.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <limits.h>


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
    bytes_limit = bytes;
    nftwSearch(absPath);

    return 0;
}
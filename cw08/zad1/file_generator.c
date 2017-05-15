//
// Created by Mrz355 on 10.05.17.
//

#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include "finals.h"


int main(int argc, char** argv) {
    char* filename;
    char* datafilename;
    if(argc == 3) {
        filename = argv[2];
        datafilename = argv[3];
    } else {
        filename = "../zad1/sample_file.db";
        datafilename = "../zad1/lorem.txt";
    }

    int in = open(datafilename,O_RDONLY);
    int out = open(filename,O_CREAT | O_WRONLY | O_TRUNC, 0600);
    if(in == -1) {perror("Error opening the file to generate data from"); exit(EXIT_FAILURE);}
    if(out == -1) {perror("Error opening the file to generate data into"); exit(EXIT_FAILURE);}


    char buf[1024];
    char text[1024];
    for(int i = 1 ; i<=RECORDSNUMBER; ++i) {
        read(in, text, (size_t) (RECORDSIZE - (int)log10(i) - 2)); // we take ceiling of log10 so we have to subtract additional 1 -(minus) length of id and separator

        sprintf(buf,"%d%s%s",i,SEPARATOR,text);
        write(out,buf,RECORDSIZE);
    }

    return 0;
}
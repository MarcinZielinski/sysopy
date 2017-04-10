//
// Created by Mrz355 on 10.04.17.
//

#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

int R; // dimension of square plot

void scale_complex(double *re, double *im) {
    *re = R * fabs(-2 - *re) / 3.0;
    *im = R * fabs(1 - *im) / 2.0;
}

void write_graph(int **T) {
    FILE* file = fopen("data","w");
    if(!file) {
        perror("Error creating the data file\n");
        exit(EXIT_FAILURE);
    }
    for(int i=0;i<R;++i) {
        for(int j=0;j<R;++j) {
            fprintf(file,"%d %d %d\n",i,j,T[i][j]);
        }
    }
    fclose(file);

    FILE* gnuplot = popen("gnuplot","w");
    fprintf(gnuplot,"set view map\n");
    fprintf(gnuplot, "set xrange [0:%d]\n",R-1);
    fprintf(gnuplot, "set yrange [0:%d]\n",R-1);
    fprintf(gnuplot, "plot 'data' with image\n");

    fflush(gnuplot);
    getc(stdin);

    pclose(gnuplot);
}

int main(int argc, char **argv) {
    if(argc!=3) {
        fprintf(stderr,"Wrong number of arguments. Required 2: path to FIFO, R - size of graph in pixels \n");
        exit(EXIT_FAILURE);
    }

    char *path = argv[1];
    R = atoi(argv[2]);
    mkfifo(path,0666);

    int fd = open(path, O_RDONLY);
    if((fd <= 0)) {
        perror("Error opening the FIFO\n");
        unlink(path);
        exit(EXIT_FAILURE);
    }

    size_t line_size = 49;
    char buf[line_size];

    int **T = malloc(sizeof(int*)*R);
    for(int i=0;i<R;++i) {
        T[i] = calloc((size_t)R,sizeof(int));
    }


    double re, im;
    int iters;
    while(read(fd,buf,line_size) > 0) {
        sscanf(buf,"%lf %lf %d",&re,&im,&iters);
        scale_complex(&re,&im);
        T[(int)re][(int)im] = iters;
    }
    close(fd);
    unlink(path);
    write_graph(T);

    for(int i=0;i<R;++i) {
        free(T[i]);
    }
    free(T);


    return 0;
}
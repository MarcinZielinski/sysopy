//
// Created by Mrz355 on 10.04.17.
//

#include <fcntl.h>
#include <unistd.h>
#include <complex.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <string.h>

double rand_re() {
    return -2*(double)rand()/(double)RAND_MAX + (double)rand()/(double)RAND_MAX;
}

double rand_im() {
    return -(double)rand()/(double)RAND_MAX + (double)rand()/(double)RAND_MAX;
}

int main(int argc, char ** argv) {
    if(argc != 4) {
        fprintf(stderr,"Wrong number of arguments. Required 3: path to FIFO, N - number of points to generate, K - number of iterations\n");
        exit(EXIT_FAILURE);
    }
    srand((unsigned int) (time(NULL) ^ (getpid() << 16)));

    sleep(1); // give master some time to create fifo

    char *path = argv[1];
    int N = atoi(argv[2]); // number of points to generate
    int K = atoi(argv[3]); // number of max iterations

    int fd = open(path,O_WRONLY | S_IFIFO);


    int iters = 0;
    double complex c;
    double complex z0;

    size_t line_size = 49;
    char buf[line_size]; // double approximation: 2* minus, 0, ., 16 precision , int approximation (int)log10(2e32) + 1 and +1 for null

    for(int i=0;i<N;i++){
        c = rand_re() + rand_im()*I;
        z0 = 0;
        iters=0;
        while(cabs(z0)<=2 && iters<K){
            z0 = z0*z0 + c;
            iters++;
        }

        sprintf(buf,"%lf %lf %d", creal(c), cimag(c), iters);
        write(fd,buf,line_size);
    }

    close(fd);
}
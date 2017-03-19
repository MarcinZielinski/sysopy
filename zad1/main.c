#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <sys/times.h>

#define CLK sysconf(_SC_CLK_TCK)

void generate(FILE **file1, size_t size, size_t nmemb) { // size - size of records, nmemb - number of records
    if((*file1) == NULL) {
        puts("Plik niezainicjalizowany poprawnie");
        exit(EXIT_FAILURE);
    }

    FILE *file2 = fopen("/dev/random","r");
    if(file2 == NULL) {
        puts("Couldn't open /dev/random file");
        exit(EXIT_FAILURE);
    }

    void *ptr = malloc(size*nmemb); // points to the place where data will be stored; size*nmemb, because we need the number of records*size of records memory cells

    size_t readRandomRecordsNumber = fread(ptr, size, nmemb, file2);
    if(readRandomRecordsNumber != nmemb) {
        puts("Couldn't read the data from /dev/random file");
        exit(EXIT_FAILURE);
    }



    size_t savedRecordsNumber = fwrite(ptr,size,nmemb,*file1); //fwrite returns the number of records created
    if(savedRecordsNumber != nmemb) {
        puts("Bład przy zapisie (generowaniu danych)");
        exit(EXIT_FAILURE);
    }

    free(ptr);
}

void printFile(FILE *file1, size_t size, size_t nmemb) {
    void *ptr = malloc(size*nmemb);

    fseek(file1,0,SEEK_SET); // going back to the beggining of the file

    size_t readRecords = fread(ptr,size,nmemb,file1);

    if(readRecords != nmemb) {
        puts("Cannot read from file in print function");
        exit(EXIT_FAILURE);
    }

    unsigned char* start = (unsigned char*) ptr;

    printf("\n\nWhole file: %s\n",start);
    for(int i=0;i<nmemb;++i) {
        printf("\n%d. Block of code: ",i+1);
        for(int j=0;j<size;++j) {
            printf(" [%x] ",start[j]);
        }
        start+=size;
    }



    free(ptr);
}

void shufflesys(int fd, size_t size, size_t nmemb) {
    void *ptr = malloc(size);
    void *qtr = malloc(size);

    int n = (int) nmemb;
    for(int i=n-1;i>=1;--i) {
        lseek(fd,i*size,SEEK_SET);
        ssize_t bytesRead = read(fd,ptr,size);
        if(bytesRead != size) {
            puts("Failed to read data in shufflesys (i)");
            exit(EXIT_FAILURE);
        }

        int j = rand()%(i+1); // 0 <= j <= i

        lseek(fd,j*size,SEEK_SET);
        bytesRead = read(fd,qtr,size);
        if(bytesRead != size) {
            puts("Failed to read data in shufflesys (j)");
            exit(EXIT_FAILURE);
        }

        lseek(fd,j*size,SEEK_SET);
        ssize_t bytesWrite = write(fd,ptr,size);
        if(bytesWrite != size) {
            puts("Failed to save data in shufflesys (i)");
            exit(EXIT_FAILURE);
        }
        lseek(fd,i*size,SEEK_SET);
        bytesWrite = write(fd,qtr,size);
        if(bytesWrite != size) {
            puts("Failed to save data in shufflesys (i)");
            exit(EXIT_FAILURE);
        }
    }

    free(ptr);
    free(qtr);
}

void shufflelib(FILE **file1, size_t size, size_t nmemb) {
    void *ptr = malloc(size);
    void *qtr = malloc(size);

    int n = (int) nmemb;
    for(int i=n-1;i>=1;--i) {
        fseek(*file1,i*size,SEEK_SET); //going to i record in the file
        size_t readRecord = fread(ptr,size,1,*file1);

        if(readRecord != 1 ) {
            puts("Failed to read data in shufflelib (i)");
            exit(EXIT_FAILURE);
        }

        int j = rand()%(i+1); // 0 <= j <= i
        fseek(*file1,j*size,SEEK_SET); //going to j record in the file
        readRecord = fread(qtr,size,1,*file1);
        if(readRecord != 1) {
            puts("Failed to read data in shufflelib (j)");
            exit(EXIT_FAILURE);
        }

        fseek(*file1,size*j,SEEK_SET);
        size_t recordWriten = fwrite(ptr,size,1,*file1);
        if(recordWriten != 1) {
            puts("Failed to save data in shufflelib (i)");
        }

        fseek(*file1,size*i,SEEK_SET);
        recordWriten = fwrite(qtr,size,1,*file1);
        if(recordWriten != 1) {
            puts("Failed to save data in shufflelib (j)");
        }
    }

    free(ptr);
    free(qtr);
}

void sortlib(FILE **file1, size_t size, size_t nmemb) {
    void *ptr = malloc(size);
    void *qtr = malloc(size);

    int n = (int) nmemb;
    for(int i=n;i>1;--i) {
        for(int j=0;j<n-1;++j) {
            fseek(*file1,j*size,SEEK_SET);
            size_t recordRead = fread(ptr,size,1,*file1);
            if(recordRead != 1){
                puts("Failed to read data in sortlib (1)");
                exit(EXIT_FAILURE);
            }
            fseek(*file1,(j+1)*size,SEEK_SET);
            recordRead = fread(qtr,size,1,*file1);
            if(recordRead != 1) {
                puts("Failed to read data in sortlib (2)");
                exit(EXIT_FAILURE);
            }
            unsigned char* a = (unsigned char*) ptr;
            unsigned char* b = (unsigned char*) qtr;
            if(a[0] > b[0]) { // then swap
                fseek(*file1,size*j,SEEK_SET);
                size_t recordWriten = fwrite(qtr,size,1,*file1);
                if(recordWriten != 1) {
                    puts("Failed to save data in sortlib (1)");
                    exit(EXIT_FAILURE);
                }

                fseek(*file1,size*(j+1),SEEK_SET);
                recordWriten = fwrite(ptr,size,1,*file1);
                if(recordWriten != 1) {
                    puts("Failed to save data in sortlib (2)");
                    exit(EXIT_FAILURE);
                }
            }
        }
    }
    free(ptr);
    free(qtr);
}

void sortsys(int fd, size_t size, size_t nmemb) {
    void *ptr = malloc(size);
    void *qtr = malloc(size);

    int n = (int) nmemb;
    for(int i=n;i>1;--i) {
        for(int j=0;j<n-1;++j) {
            lseek(fd,j*size,SEEK_SET);
            ssize_t bytesRead = read(fd,ptr,size);
            if(bytesRead != size){
                puts("Failed to read data in sortsys (1)");
                exit(EXIT_FAILURE);
            }
            lseek(fd,(j+1)*size,SEEK_SET);
            bytesRead = read(fd,qtr,size);
            if(bytesRead != size) {
                puts("Failed to read data in sortsys (2)");
                exit(EXIT_FAILURE);
            }
            unsigned char* a = (unsigned char*) ptr;
            unsigned char* b = (unsigned char*) qtr;
            if(a[0] > b[0]) { // then swap
                lseek(fd,size*j,SEEK_SET);
                ssize_t bytesWriten = write(fd,qtr,size);
                if(bytesWriten != size) {
                    puts("Failed to save data in sortsys (1)");
                    exit(EXIT_FAILURE);
                }

                lseek(fd,size*(j+1),SEEK_SET);
                bytesWriten = write(fd,ptr,size);
                if(bytesWriten != size) {
                    puts("Failed to save data in sortsys (2)");
                    exit(EXIT_FAILURE);
                }
            }
        }
    }
    free(ptr);
    free(qtr);
}

typedef struct timeStruct {
    double r; // real
    double u; // user
    double s; // system
} timeStruct;

void loadTimes(struct timeStruct **time1) {
    if((*time1) == NULL) {
        (*time1) = malloc(sizeof(timeStruct));
    }

    struct tms userSystemTimeStruct;
    times(&userSystemTimeStruct); // struct loaded;

    (*time1)->s = (double)userSystemTimeStruct.tms_stime / CLK;
    (*time1)->u = (double)userSystemTimeStruct.tms_utime / CLK;
    (*time1)->r = (double)clock() / CLOCKS_PER_SEC;
}

void printTimes(char* title, struct timeStruct *prevTime, struct timeStruct *afterTime) {
    double real = afterTime->r - prevTime->r;
    double user = afterTime->u - prevTime->u;
    double system = afterTime->s - prevTime->s;
    printf("\n%s \nReal: \t\t User: \t\t System:\n", title);
    printf("%lf \t %lf \t %lf\n",real,user,system);
}

void printAverage(char* title, double realSum, double userSum, double systemSum) {
    printf("\n%s \nReal: \t\t User: \t\t System:\n", title);
    printf("%lf \t %lf \t %lf\n",realSum,userSum,systemSum);
}

int main(int argc, char **argv) {
    if(argc == 5 && strcmp(argv[1],"generate") == 0) {
        FILE *file1 = fopen(argv[2],"w+");
        if(file1 == NULL) {
            puts("Couldn't create/open file to generate");
            exit(EXIT_FAILURE);
        }
        generate(&file1,(size_t)(((int)*argv[3])-48),(size_t)(((int)*argv[4]) - 48));
        puts("Generated file with random data");
        exit(EXIT_SUCCESS);
    }

    if(argc != 5 && argc !=6) {
        puts("Wrong number of parameters");
        exit(EXIT_FAILURE);
    }
    char* type = argv[1];
    char* operation = argv[2];
    char* filePath = argv[3];
    char* recordSize = argv[4];
    char* recordNumber = argv[5];

    int _size = (int) *recordSize -48;
    int _nmemb = (int) *recordNumber -48;

    size_t size = (size_t) _size;
    size_t nmemb = (size_t) _nmemb;

    struct timeStruct* prevTime = NULL;
    struct timeStruct* afterTime = NULL;

    if(strcmp(type,"sys") == 0) {
        int fd = open(filePath,O_RDWR);
        if(fd<0) {
            puts("Couldn't open file in sys mode");
            exit(EXIT_FAILURE);
        }
        if(strcmp(operation,"shuffle")==0) {
            loadTimes(&prevTime);
            shufflesys(fd,size,nmemb);
            loadTimes(&afterTime);
            printTimes("Sys functions shuffle time:",prevTime,afterTime);
        } else if (strcmp(operation, "sort") == 0) {
            loadTimes(&prevTime);
            sortsys(fd,size,nmemb);
            loadTimes(&afterTime);
            printTimes("Sys functions sort time:",prevTime,afterTime);
        } else {
            puts("Wrong parameters. (eg. ./program sys shuffle datafile 100 512");
        }
        close(fd);
    } else if (strcmp(type,"lib") == 0) {
        FILE *file1 = fopen(filePath, "w+"); // w+ - read and write + create/wipe out the file
        if(file1 == NULL) {
            puts("Couldn't open file in lib mode");
            exit(EXIT_FAILURE);
        }
        if(strcmp(operation,"shuffle")==0) {
            loadTimes(&prevTime);
            shufflelib(&file1,size,nmemb);
            loadTimes(&afterTime);
            printTimes("Lib functions shuffle time:",prevTime,afterTime);
        } else if(strcmp(operation,"sort") == 0) {
            loadTimes(&prevTime);
            sortlib(&file1,size,nmemb);
            loadTimes(&afterTime);
            printTimes("Lib functions sort time:",prevTime,afterTime);
        } else {
            puts("Wrong parameters. (eg. ./program sys shuffle datafile 100 512");
        }
        fclose(file1);
    }
    return 0;
}
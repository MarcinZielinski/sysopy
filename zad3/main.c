//
// Created by Mrz355 on 19.03.17.
//

#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <linux/limits.h>

void lockCharW(int fd) {
    int place;
    puts("Na jaki znak ustawić rygiel do zapisu?");
    scanf("%d",&place);

    struct flock *lockStruct = malloc(sizeof(struct flock));
    lockStruct->l_type=F_WRLCK;
    lockStruct->l_start=place;
    lockStruct->l_len=1;
    lockStruct->l_whence=SEEK_SET;

    if(fcntl(fd,F_SETLK,lockStruct) == -1) { // set lock

        if (errno == EACCES || errno == EAGAIN) {
            puts("Nie udało się założyć rygla z powodu innych blokad.\n");
        } else {
            printf("Error: %s, errno id: %d\n\n",strerror(errno),errno);
        }
    } else {
        puts("Udało się założyć rygiel\n");
    }

    free(lockStruct);
}

void lockCharR(int fd) {
    int place;
    puts("Na jaki znak ustawić rygiel do odczytu?");
    scanf("%d",&place);

    struct flock *lockStruct= malloc(sizeof(struct flock));
    lockStruct->l_type=F_RDLCK; // what type of lock (Read lock)
    lockStruct->l_start=place;  // what byte to lock
    lockStruct->l_len=1;    // how many bytes to lock
    lockStruct->l_whence=SEEK_SET;  // where to start (beginning)

    if(fcntl(fd,F_SETLK,lockStruct) == -1) { // set lock

        if (errno == EACCES || errno == EAGAIN) {
            puts("Nie udało się założyć rygla z powodu innych blokad.\n");
        } else {
            printf("Error: %s, errno id: %d\n\n",strerror(errno),errno);
        }
    } else {
        puts("Udało się założyć rygiel\n");
    }

    free(lockStruct);
}

void printLocks(int fd) {
    int end = (int) lseek(fd, 0, SEEK_END);
    int pos = 0;

    struct flock *lockStruct= malloc(sizeof(struct flock));
    while(pos<end) {
        lockStruct->l_type=F_RDLCK;
        lockStruct->l_start=pos;
        lockStruct->l_len=1;
        lockStruct->l_whence=SEEK_SET;
        fcntl(fd,F_GETLK,lockStruct); // trying to set potential lock
        if (lockStruct->l_type != F_UNLCK){
            printf("Read lock at: %d byte. PID: %d\n",pos,lockStruct->l_pid);
        }
        lockStruct->l_type=F_WRLCK;
        lockStruct->l_start=pos;
        lockStruct->l_len=1;
        lockStruct->l_whence=SEEK_SET;
        fcntl(fd,F_GETLK,lockStruct);
        if(lockStruct->l_type != F_UNLCK) {
            printf("Write lock at: %d byte. PID: %d\n",pos,lockStruct->l_pid);
        }

        ++pos; // go to the next byte
    }
    puts("");
    free(lockStruct);
}

void unlockByte(int fd) {
    int pos;
    puts("Z którego bajtu pliku chcesz ściągnąć rygiel?");
    scanf("%d",&pos);

    struct flock *lockStruct= malloc(sizeof(struct flock));
    lockStruct->l_type=F_UNLCK; // what type of lock (Read lock)
    lockStruct->l_start=pos;  // what byte to lock
    lockStruct->l_len=1;    // how many bytes to lock
    lockStruct->l_whence=SEEK_SET;  // where to start (beginning)
    if(fcntl(fd,F_SETLK,lockStruct)==-1) {
        if (errno == EACCES || errno == EAGAIN) {
            printf("Nie udało się usunąć rygla ze bajtu: %d z powodu innych blokad\n\n",pos);
        } else {
            printf("Error: %s, errno id: %d\n\n",strerror(errno),errno);
        }
    } else {
        printf("Pomyślnie usunięto rygiel z bajtu: %d\n\n",pos);
    }

    free(lockStruct);
}

void readByte(int fd) {
    int pos;
    puts("Który znak chcesz odczytać?");
    scanf("%d",&pos);
    if(pos>lseek(fd,0,SEEK_END)) {
        puts("Przekroczono rozmiar pliku!\n");
        return;
    }

    lseek(fd,pos,SEEK_SET);
    char c;
    read(fd,&c,1);

    printf("Odczytany znak to: %c\n\n",c);
}

void writeByte(int fd) {
    int pos;
    puts("Który znak chcesz podmienić?");
    scanf("%d",&pos);
    if(pos>lseek(fd,0,SEEK_END)) {
        puts("Przekroczono rozmiar pliku!\n");
        return;
    }

    char c;
    puts("Na jaki?");
    scanf(" %c",&c);

    lseek(fd,pos,SEEK_SET);
    if(write(fd,&c,1) == 1) {
        puts("Pomyślnie zapisano bajt do pliku!\n");
    } else {
        puts("Nie udało się zapisać bajtu do pliku\n");
    }
}

void lockCharRWait(int fd) {
    int place;
    puts("Na jaki znak ustawić rygiel do odczytu?");
    scanf("%d",&place);

    struct flock *lockStruct= malloc(sizeof(struct flock));
    lockStruct->l_type=F_RDLCK; // what type of lock (Read lock)
    lockStruct->l_start=place;  // what byte to lock
    lockStruct->l_len=1;    // how many bytes to lock
    lockStruct->l_whence=SEEK_SET;  // where to start (beginning)

    fcntl(fd,F_SETLKW,lockStruct); // set lock

    puts("Założono rygiel\n");

    free(lockStruct);
}

void lockCharWWait(int fd) {
    int place;
    puts("Na jaki znak ustawić rygiel do zapisu?");
    scanf("%d",&place);

    struct flock *lockStruct = malloc(sizeof(struct flock));
    lockStruct->l_type=F_WRLCK;
    lockStruct->l_start=place;
    lockStruct->l_len=1;
    lockStruct->l_whence=SEEK_SET;

    fcntl(fd,F_SETLKW,lockStruct);

    puts("Założono rygiel\n");

    free(lockStruct);
}

void menu(char* filePath) {
    char c;

    int fd;
    if((fd = open(filePath, O_RDWR) ) < 0) {
        puts("Couldn't open given file");
        exit(EXIT_FAILURE);
    }

    while(1) {
        puts("1. Ustawienie rygla do odczytu na wybrany znak pliku");
        puts("2. Ustawienie rygla do zapisu na wybrany znak pliku");
        puts("3. Ustawienie rygla do odycztu na wybrany znak pliku (oczekuje aż będzie się dało)");
        puts("4. Ustawienie rygla do zapisu na wybrany znak pliku (oczekuje aż będzie się dało)");
        puts("5. Wyświetlenie listy zaryglowanych znaków pliku (z informacją o numerze PID procesu będącego właścicielem rygla oraz jego typie - odczyt/zapis)");
        puts("6. Zwolnienie wybranego rygla");
        puts("7. Odczyt (funkcją read) wybranego znaku pliku");
        puts("8. Zmiana (funkcją write) wybranego znaku pliku");
        puts("9. Wyjście");

        while(scanf(" %c", &c)!=1);

        switch(c) {
            case '1':
                lockCharR(fd);
                break;
            case '2':
                lockCharW(fd);
                break;
            case '3':
                lockCharRWait(fd);
                break;
            case '4':
                lockCharWWait(fd);
                break;
            case '5':
                printLocks(fd);
                break;
            case '6':
                unlockByte(fd);
                break;
            case '7':
                readByte(fd);
                break;
            case '8':
                writeByte(fd);
                break;
            case '9':
                close(fd);
                return;
            default:
                break;
        }
    }
}

int main(int argc, char ** argv) {
    if(argc!=2) {
        printf("Błędna ilosc argumentow!");
        exit(EXIT_FAILURE);
    }
    char* absPath = malloc(PATH_MAX);
    realpath(argv[1],absPath);
    printf("%s\n",absPath);
    menu(absPath);


    return 0;
}
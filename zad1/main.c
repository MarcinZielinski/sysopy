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


    return 0;
}
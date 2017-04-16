//
// Created by Mrz355 on 10.04.17.
//

#include <unistd.h>
#include <stdio.h>
#include <wait.h>
#include <stdlib.h>

int main(int argc, char **argv) {
    pid_t master = fork();
    if (master ==0) {
        if(execl("./master","master","./myfifo","600", NULL) == -1) {
            perror("Error while executing master");
	    exit(EXIT_FAILURE);
        }
    }pid_t child;
    for(int i=0;i<10;++i) {
        child = fork();
        if (child == 0) {
            if (execl("./slave", "slave", "./myfifo", "1000000", "100", NULL) ==
                -1) {
                perror("Error while executing slave");
		exit(EXIT_FAILURE);
            }
        }
    }
    int status;
    waitpid(master,&status, 0);

    return 0;
}

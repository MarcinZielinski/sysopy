#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

typedef struct command
{
    const char **argv;
} command;
// 0 (stdin) - read, 1 (stdout) - write
int spawn_proc (int in, int out, struct command *cmd)
{
    pid_t pid;

    if ((pid = fork ()) == 0)
    {
        if (in != STDIN_FILENO) // we do not want to close the standard input
        {
            dup2 (in, STDIN_FILENO); // instead of STDIN we want to read from the previous pipe
            close (in);
        }

        if (out != STDOUT_FILENO)
        {
            dup2 (out, STDOUT_FILENO); // instead of STDOUT we want to write to the brand new pipe
            close (out);
        }

        return execvp (cmd->argv [0], (char * const *)cmd->argv);
    }

    return pid;
}

void fork_pipes (int n, struct command *cmd)
{
    int i;
    int in, fd [2];

    in = STDIN_FILENO; // first child will read from console (nothing will be read)

    for (i = 0; i < n -1; ++i)
    {
        pipe (fd); // brand-new PIPE!

        if(spawn_proc (in, fd [1], cmd + i) == -1) { // new child will read from the old pipe, write to the new pipe
            perror("Invalid command");
            exit(EXIT_FAILURE);
        }
        close (fd [1]); // we can close this write end, because child will write here (he copied the pipe for himself)

        in = fd [0]; // the next child will read from here (old child wrote to this pipe something)
    }
    if(spawn_proc(in,STDOUT_FILENO,cmd + n - 1) == -1) { // last child writes down on stdout
        perror("Invalid command");
        exit(EXIT_FAILURE);
    }
}

struct command* splitArguments(char * str, int *n) {
    *n = 0;
    if(str == NULL) return NULL;
    if(strlen(str)==0) return NULL;

    while(str[0] == ' ') ++str;

    int commands = 1;
    for(int i=0;str[i]!='\0';++i) { // we calculate the number of pipes to acknowledge the number of commands to execute
        if(str[i] == '|') ++commands;
    }
    *n = commands;
    command *res = malloc((commands)*sizeof(struct command));

    char **argv = malloc(sizeof(char*));
    char* p = strtok(str, " ");
    int currCommandIndex = 0;
    int currCommandLen = 0;
    while(p!=NULL) {
        ++currCommandLen;

        argv = realloc(argv,currCommandLen*sizeof(char*));
        argv[currCommandLen-1] = strdup(p);

        if(strlen(p) == 1 && p[0]=='|') {
            argv[currCommandLen-1] = NULL;
            res[currCommandIndex].argv = (const char **) argv;
            argv = malloc(sizeof(char*));

            ++currCommandIndex;
            currCommandLen = 0;
        }

        p = strtok(NULL, " ");
    }
    argv = realloc(argv,(currCommandLen+1)*sizeof(char*));
    argv[currCommandLen] = NULL;
    res[currCommandIndex].argv = (const char **) argv;

    return res;
}
command *cmd = NULL;
void sigintHandler(int signum) {
    printf("\nSigint received. Exiting program.\n");
    free(cmd);
    exit(EXIT_SUCCESS);
}

int main ()
{
    char argv[1024];
    int n = 0;
    signal(SIGINT,sigintHandler);
    while((1 == scanf("%[^\n]%*c", argv))) {
        command *cmd = splitArguments(argv, &n);
        fork_pipes(n,cmd);
        free(cmd);
    }
    return 0;
}
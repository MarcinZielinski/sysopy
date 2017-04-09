#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

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

int fork_pipes (int n, struct command *cmd)
{
    int i;
    int in, fd [2];

    in = STDIN_FILENO; // first child will read from console (nothing will be read)

    for (i = 0; i < n - 1; ++i)
    {
        pipe (fd); // brand-new PIPE!

        spawn_proc (in, fd [1], cmd + i); // new child will read from the old pipe, write to the new pipe

        close (fd [1]); // we can close this write end, because child will write here (he copied the pipe for himself)

        in = fd [0]; // the next child will read from here (old child wrote to this pipe something)
    }

    if (in != 0)
        dup2 (in, STDIN_FILENO);

    return execvp (cmd [i].argv [0], (char * const *)cmd [i].argv);
}

struct command* splitArguments(int *argc, char **argv) {
    ++argv; // the program name (argv[0]) arguments is not needed
    --(*argc); // therefore change the number of arguments --

    int pipes = 0;
    for(int i=0;i<*argc;++i) { // we calculate the number of pipes to acknowledge the number of commands to execute
        if(strlen(argv[i]) == 1 && argv[i][0] == '|') {
            ++pipes;
        }
    }
    command* res = malloc((pipes+1)*sizeof(struct command));
    int cur_command_len = 0; // current command amount of arguments
    int commands = 0; // total amount of commands
    for(int k = 0;k<=*argc;++k) {
        ++cur_command_len;
        if(k == *argc || (strlen(argv[k]) == 1 && argv[k][0] == '|')) {
            res[commands].argv = malloc((cur_command_len)*sizeof(char*));
            res[commands].argv[--cur_command_len] = NULL;

            for (int i = 0; cur_command_len > 0; ++i, --cur_command_len) {
                res[commands].argv[i] = strdup(argv[k - cur_command_len]);
            }
            ++commands;
        }
    }
    *argc = commands;
    return res;
}

int main (int argc, char ** argv)
{
    if(argc == 1) {
        fprintf(stderr,"Pass arguments as a commands separated with the pipe (\"|\") operator");
        return EXIT_FAILURE;
    }

    command* cmd = splitArguments(&argc, argv);

    return fork_pipes (argc, cmd);
}
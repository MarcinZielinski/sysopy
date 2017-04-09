#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

struct command
{
    const char **argv;
};
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
    pid_t pid;
    int in, fd [2];

    in = STDIN_FILENO; // first child will read from console (nothing will be read)

    for (i = 0; i < n - 1; ++i)
    {
        pipe (fd); // brand-new PIPE!

        /* f [1] is the write end of the pipe, we carry `in` from the prev iteration.  */
        spawn_proc (in, fd [1], cmd + i); // new child will read from the old pipe, write to the new pipe

        /* No need for the write end of the pipe, the child will write here.  */
        close (fd [1]); // we can close this write end, because child will write here (he copied the pipe for himself)

        /* Keep the read end of the pipe, the next child will read from there.  */
        in = fd [0]; // the next child will read from here (old child wrote to this pipe something)
    }

    /* Last stage of the pipeline - set stdin be the read end of the previous pipe
       and output to the original file descriptor 1. */
    if (in != 0)
        dup2 (in, STDIN_FILENO);

    /* Execute the last stage with the current process. */
    return execvp (cmd [i].argv [0], (char * const *)cmd [i].argv);
}



int main ()
{
    const char *ls[] = { "ls", "-l", 0 };
    const char *awk[] = { "awk", "{print $1}", 0 };
    const char *sort[] = { "sort", 0 };
    const char *uniq[] = { "uniq", 0 };

    struct command cmd [] = { {ls}, {awk}, {sort}, {uniq} };

    return fork_pipes (4, cmd);
}
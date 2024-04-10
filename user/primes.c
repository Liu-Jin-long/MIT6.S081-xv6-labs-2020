#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
int child_process_handle(int pipes[2]);
int main(int argc, char **argv)
{
    int pipes[2];
    pipe(pipes);
    int ret = fork();
    if (ret > 0)
    {
        for (int i = 2; i <= 35; i++)
        {
            write(pipes[1], &i, sizeof(int));
        }
    }
    else if (ret == 0)
    {
        child_process_handle(pipes);
    }
    else
    {
        close(pipes[0]);
        close(pipes[1]);
        exit(1);
    }
    close(pipes[0]);
    close(pipes[1]);
    wait(0);
    exit(0);
}

int child_process_handle(int left_pipes[2])
{
    close(left_pipes[1]);
    int p, n;
    if (read(left_pipes[0], &p, sizeof(int)) == 0) 
    {
        close(left_pipes[0]);
        exit(1);
    }
    fprintf(1, "prime %d\n", p);
    int right_pipes[2];
    pipe(right_pipes);
    int ret = fork();
    if (ret > 0)
    {
        while (read(left_pipes[0], &n, sizeof(int)))
        {
            if (n % p != 0)
            {
                write(right_pipes[1], &n, sizeof(int));
            }
        }
    }
    else if (ret == 0)
    {
        child_process_handle(right_pipes);
    }
    else
    {
        close(left_pipes[0]);
        close(right_pipes[0]);
        close(right_pipes[1]);
        exit(1);
    }
    close(left_pipes[0]);
    close(right_pipes[0]);
    close(right_pipes[1]);
    wait(0);
    exit(0);
}

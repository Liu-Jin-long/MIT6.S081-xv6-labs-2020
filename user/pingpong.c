#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
int main(int argc, char **argv)
{
    int pipes[2];
    pipe(pipes);
    int ret = fork();
    char send_byte;
    if (ret > 0)
    {
        write(pipes[1],&send_byte,1);
        sleep(1);//防止父进程执行太快写完就读
        read(pipes[0],&send_byte,1);
        fprintf(2,"%d: received pong\n",getpid());
        close(pipes[0]);
        close(pipes[1]);
        exit(0);
    }
    else if (ret == 0)
    {
        read(pipes[0],&send_byte,1);
        fprintf(2,"%d: received ping\n",getpid());
        write(pipes[1],&send_byte,1);
        close(pipes[0]);
        close(pipes[1]);
        exit(0);
    }
    else
    {
        fprintf(2, "exec fork failed!\n");
        exit(1);
    }
}
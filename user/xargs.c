#include "kernel/types.h"
#include "kernel/stat.h"
#include "user.h"
int main(int argc, char **argv)
{
    if (argc < 2)
    {
        printf("Usage Error!\n");
        exit(1);
    }
    char args[32][8] = {0}; // kernel/param.h中 MAXARG=32
    char c;
    int ret;
    int i;
    for (i = 0; i < 32; i++)
    {
        for (int j = 0; j < 8; j++)
        {
            ret = read(0, &c, 1);
            if (ret == 0)
            {
                break;
            }
            if (c != '\n')
            {
                args[i][j] = c;
            }
            else
            {
                break;
            }
        }
        if (ret == 0)
        {
            break;
        }
        //printf("%d  %s \n", i, args[i]);
    }
    char *cmd[32] = {0};
    //去除首位的xargs
    for (int q = 1; q < argc; q++)
    {
        cmd[q-1]=argv[q];
    }
    for (int j = 0; j < i; j++)
    {
        cmd[argc-1] = args[j];
        cmd[argc] = 0;
        int ret=fork();
        if(ret==0)
        {
            exec(argv[1],cmd);
            exit(1);
        }else if(ret>0)
        {
            wait(0);
        }
    }
    exit(0);
}

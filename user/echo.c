#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
int main(int argc, char *argv[])
{
  int i;

  for (i = 1; i < argc; i++)
  {
    write(1, argv[i], strlen(argv[i]));
    if (i + 1 < argc)
    {
      write(1, " ", 1);
    }
    else
    {
      write(1, "\n", 1);
    }
  }
  // volatile int j=0;
  // while(j!=1<<28)
  // {
  //   j++;
  // };
  // sigalarm(2,han);
  // while(j!=0)
  // {
  //   j--;
  // };
  exit(0);
}

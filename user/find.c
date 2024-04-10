#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

void find(const char *name, char *path);
int main(int argc, char **argv)
{
    if (argc != 3)
    {
        printf("Usage: find dir_name file_name\n");
        exit(1);
    }
    find(argv[2], argv[1]);
    exit(0);
}
//每次执行 查找文件名 查找路径(path/name)
void find(const char *name, char *path)
{
    struct dirent dirent_;
    struct stat stat_;
    int fd;
    if ((fd = open(path, 0)) < 0)
    {
        printf("find: cannot open %s\n", path);
        return;
    }
    if (fstat(fd, &stat_) < 0)
    {
        printf("find: cannot stat %s\n", path);
        close(fd);
        return;
    }
    char *p = path + strlen(path);
    while (p >= path && *p != '/')
        p--;
    p++;
    if (0 == strcmp(name, p))
    {
        printf("%s\n", path);
    }
    if (stat_.type == T_DIR)
    {
        char current_path[64] = {0};
        strcpy(current_path, path);
        current_path[strlen(current_path)] = '/';
        while (read(fd, &dirent_, sizeof dirent_) == sizeof dirent_)
        {
            if (dirent_.inum == 0 || strcmp(dirent_.name, ".") == 0 || strcmp(dirent_.name, "..") == 0)
            {
                continue;
            }
            char next_path[64] = {0};
            strcpy(next_path, current_path);
            strcpy(next_path + strlen(next_path), dirent_.name);
            find(name, next_path);
        }
    }

    close(fd);
    return;
}
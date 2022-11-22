#include "kernel/types.h"
#include "user/user.h"
#include "kernel/stat.h"

/**
 * @brief 
 * pipe(p)会产生两个单向管道，无论是子进程还是父进程，f0都是读,f1都是写，默认pipe的read都是阻塞的
 * 如果读不到数据就会一直等下去，直到对该fd的写方全部close，注意自身的f1并不是f0的写方。
 */
int main(int argc, char *argv[])
{
    char temp[10];
    int p[2];
    pipe(p);
    if (fork() == 0) {
        if (read(p[0], temp, 1) == 1) {
            close(p[0]);
            fprintf(1, "%d: received ping\n", getpid());
            write(p[1], "1", 1);
            close(p[1]);
            exit(0);
        } else {
            fprintf(2, "pingpong: child read pipe error");
            exit(1);
        }
    } else {
        write(p[1], "1", 1);
        close(p[1]);
        wait(0);
        if (read(p[0], temp, 1) == 1) {
            close(p[0]);
            fprintf(1, "%d: received pong\n", getpid());
            exit(0);
        } else {
            fprintf(2, "pingpong: father read pipe error");
            exit(1);
        }
    }
    return 0;
}
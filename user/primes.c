#include "kernel/types.h"
#include "user/user.h"
#include "kernel/stat.h"


void subsequent(int n, int father_pipe_read, int father_pipe_write) {
    if (fork() == 0) {
        close(father_pipe_write);
        printf("prime %d\n", n);
        int p[2];
        pipe(p);
        int num;
        int last = 1;
        while(read(father_pipe_read, &num, 4) > 0) {
            if (num % n == 0) {
                continue;
            }
            if (last == 1) {
                last = 0;
                subsequent(num, p[0], p[1]);
            } else if (write(p[1], &num, 4) != 4) {
                fprintf(2, "pid=[%d] write error\n", getpid());
                exit(1);
            }
        }
        close(p[1]);
        int res;
        wait(&res);
        if (res == 1) {
            exit(1);
        }
        exit(0);
    } else {
        close(father_pipe_read);
    }
}


int main(int argc, char *argv[])
{
    int p[2];
    pipe(p);
    subsequent(2, p[0], p[1]);
    for (int i = 3; i < 35; i++) {
        if (write(p[1], &i, 4) != 4) {
            fprintf(2, "pid=[%d] write error\n", getpid());
        }
    }
    close(p[1]);
    int res;
    wait(&res);
    if (res == 1) {
        return -1;
    }
    return 0;
}
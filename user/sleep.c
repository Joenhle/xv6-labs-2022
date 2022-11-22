#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv[])
{
    if (argc != 2) {
        fprintf(2, "sleep: at least need one args\n");
        exit(1);
    }
    for (int i = 0; i < strlen(argv[1]); i++) {
        if (argv[1][i] < '0' || argv[1][i] > '9') {
            fprintf(2, "sleep: arg should be a number\n");
            exit(1);
        }
    }
    int sleep_time = atoi(argv[1]);
    sleep(sleep_time);
    exit(0);
}
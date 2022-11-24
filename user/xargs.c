#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

void do_exec(char* command, char** argv, int n) {
    char* temp[n+2];
    temp[0] = command;
    for (int i = 0; i < n; i++) {
        temp[i+1] = argv[i];
    }
    temp[n+1] = 0;
    if (fork() == 0) {
        exec(command, temp);
    } else {
        wait(0);
    }
}

int main(int argc, char *argv[])
{
    // fprintf(1, "argc = %d\n", argc);
    // for (int i = 0; i < argc; i++) {
    //     fprintf(1, "argv[%d] = %s\n", i, argv[i]);
    // }
    if (argc < 2) {
        fprintf(2, "xargs: invalid argument number\n");
        exit(1);
    }
    char buf[512];
    memset(buf, 0, 512);
    int offset = 0;
    int bytes = 0;
    //注意这里如果用if的话可能会读不全，因为管道送来有延迟
    while((bytes = read(0, buf+offset, 512)) > 0) {
        offset += bytes;
    }

    char* ptr[100];
    int pre = -1;
    int num = 0;
    int len = strlen(buf);
    for (int i = 0; i < len; i++) {
        if (buf[i] == '\n') {
            ptr[num++] = &buf[pre+1];
            buf[i] = 0;
            pre = i;
        } else if (i == len-1) {
            ptr[num++] = &buf[pre+1];
        }
    }
    int group_size = -1;
    int command_num = 2;
    if (strcmp(argv[1], "-n") == 0) {
        group_size = atoi(argv[2]);
        command_num = 4;
    }
    int totle_param_num = argc - command_num + num;
    char* exec_argv[totle_param_num];
    for (int i = command_num; i < argc; i++) {
        exec_argv[i-command_num] = argv[i];
    }
    for (int i = 0; i < num; i++) {
        exec_argv[argc - command_num + i] = ptr[i];
    }
    group_size = group_size == -1 ? (totle_param_num) : group_size;
    int i;


    // fprintf(1, "command = %s, totle = %d, group_size = %d\n", argv[command_num-1], totle_param_num, group_size);
    // for (int i = 0; i < totle_param_num; i++) {
    //     fprintf(1, "exec_argv[%d] = %s\n", i, exec_argv[i]);
    // }

    for (i = 0; i < totle_param_num/group_size; i++) {
        do_exec(argv[command_num-1], exec_argv + i*group_size, group_size);
    }
    if (totle_param_num > i*group_size) {
        do_exec(argv[command_num-1], exec_argv + i*group_size, totle_param_num-i*group_size);
    }
    return 0;
}
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

short ftype(char* path) {
    struct stat st;
    if (stat(path, &st) < 0) {
        fprintf(2, "find: stat get path info error, path = %sn", path);
        exit(1);
    }
    return st.type;
}

void find(char* prefix_path, char* key) {
    int type = ftype(prefix_path);
    if (type == T_FILE) {
        int m = strlen(prefix_path);
        int n = strlen(key);
        if (m > n && strcmp(prefix_path + m - n, key) == 0) {
            fprintf(1, "%s\n", prefix_path);
        }
    } else if (type == T_DIR) {
        struct dirent de;
        int fd;
        fd = open(prefix_path, 0);
        char buf[100];
        memset(buf, 0, 100);
        strcpy(buf, prefix_path);
        char* p = buf + strlen(prefix_path);
        *p++ = '/';
        while(read(fd, &de, sizeof(de)) == sizeof(de)) {
            if (de.inum == 0 || strcmp(de.name, ".") == 0 || strcmp(de.name, "..") == 0) {
                continue;
            }
            memmove(p, de.name, DIRSIZ);
            p[DIRSIZ] = 0;
            find(buf, key);
        }
        close(fd);
    }
}

int main(int argc, char *argv[])
{
    if (argc == 3) {
        find(argv[1], argv[2]);
    } else {
        fprintf(2, "find: invalid arguments\n");
    }
    return 0;
}
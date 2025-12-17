#include <stdio.h> 
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>

void print_uids(const char *label){
    printf("%s: real UID = %d, effective UID = %d\n", label, (int)getuid(), (int)geteuid());
}

int try_open_write(const char *path){
    FILE *f = fopen(path, "w");
    if (f == NULL){
        perror("fopen (w) failed");
        return -1;
    }
    printf("fopen/fclose succeeded\n");
    return 0; 
}

int main(int argc, char *argv[]){
    if (argc != 2){
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
    }
    const char *filename = argv[1];

    //1. печатаем uid и пробуем открыть 
    print_uids("Before setuid");
    try_open_write(filename);

    //2. делаем реал==эффектив, то есть сбрасываем привилегия 
    uid_t r = getuid();
    if (setuid(r) == -1){
        perror("setuid failed");
        //возможно euid уже == r или у процесса нет права
    }

    //3. снова печатаем uid и пробуме открыть 
    print_uids("after setuid(getuid())");
    if (try_open_write(filename) != 0){
        //если не получилось открыть это ожидаемое поведение в случае сброса euid
    }
    return 0; 
}
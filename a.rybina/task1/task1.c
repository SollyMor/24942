#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/resource.h>
#include <sys/param.h>
#include <limits.h>
#include <string.h>
#include <errno.h>
#include <ulimit.h>

//-i  Печатает реальные и эффективные идентификаторы пользователя и группы.
//-s  Процесс становится лидером группы. Подсказка: смотри setpgid(2).
// -p  Печатает идентификаторы процесса, процесса-родителя и группы процессов.
// -u  Печатает значение ulimit
// -Unew_ulimit  Изменяет значение ulimit. Подсказка: смотри atol(3C) на странице руководства strtol(3C)
// -c  Печатает размер в байтах core-файла, который может быть создан.
// -Csize  Изменяет размер core-файла
// -d  Печатает текущую рабочую директорию
// -v  Распечатывает переменные среды и их значения
// -Vname=value  Вносит новую переменную в среду или изменяет значение существующей переменной.

char *strdup(const char *s) {
    size_t len = strlen(s) + 1;
    char *dup = malloc(len);
    if (dup != NULL) {
        strcpy(dup, s);
    }
    return dup;
}

void print_ids(){
    printf("\n");
    printf("Real UID: %d\n", getuid());
    printf("Effective UID: %d\n", geteuid());
    printf("Real GID: %d\n", getgid());
    printf("Effective GID: %d\n", getegid());
}

void become_group_leader(){
    printf("\n");
    if (setpgid(0, 0) == -1){
        perror("setpgid");
        return;
    }
    printf("Process became group leader\n");
}

void print_process_ids(){
    printf("\n");
    printf("Process ID: %d\n", getpid());
    printf("Parent Process ID: %d\n", getppid());
    printf("Process Group ID: %d\n", getpgrp());
}

void print_ulimit(){
    printf("\n");
    printf("ulimit: %ld\n", ulimit(0));
}

void change_ulimit(const char *value){
    printf("\n");
    if (ulimit(0, atol(value)) == -1){
        perror("ulimit");
        return;
    }
    printf("ulimit changed to %ld\n", ulimit(0));
}

void print_core_size(){
    printf("\n");
    struct rlimit rlim;
    if (getrlimit(RLIMIT_CORE, &rlim) == 0){
        printf("core size: %ld bytes\n", rlim.rlim_cur);
    } else {
        perror("getrlimit");
    }
    
}

void change_core_size(const char *value){
    printf("\n");
    struct rlimit rlim;
    rlim.rlim_cur = atol(value);
    rlim.rlim_max = rlim.rlim_cur;
    if (setrlimit(RLIMIT_CORE, &rlim) == -1){
        perror("setrlimit");
        return;
    }
    printf("core size changed to %ld bytes\n", rlim.rlim_cur);
}

void print_current_directory(){
    printf("\n");
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) == NULL){
        perror("getcwd");
        return;
    }
    printf("current directory: %s\n", cwd);
}

void print_environment(){
    printf("\n");
    extern char **environ;
    char **env = environ;

    printf("environment variables:\n");
    while (*env != NULL){
        printf("%s\n", *env);
        env++;
    }
}

void set_environment_variable(const char *name, const char *value){
    printf("\n");
    if (setenv(name, value, 1) == -1){
        perror("setenv");
        return;
    }
    printf("environment variable %s set to %s\n", name, value);
}

int parse_env_name(const char *input, char **name, char **value){
    char *equals = strchr(input, '=');
    if (equals == NULL) {
        perror("Invalid format for -V. Use: name=value\n");
        return -1;
    }

    size_t name_len = equals - input;
    *name = malloc(name_len + 1);
    *value = strdup(equals + 1);

    if (*name == NULL || *value == NULL){
        perror("malloc/strdup");
        free(*name);
        *name = NULL;
        free(*value);
        *value = NULL;
        return -1;
    }

    strncpy(*name, input, name_len);
    (*name)[name_len] = '\0';

    return 0;
}


int main(int argc, char *argv[]) {
    int param;

    while ((param = getopt(argc, argv, "ispuU:cC:dvV:")) != -1){
        switch (param){
            case 'i':
                print_ids();
                break;
            case 's':
                become_group_leader();
                break;
            case 'p':
                print_process_ids();
                break;
            case 'u':
                print_ulimit();
                break;
            case 'U':
                change_ulimit(optarg);
                break;
            case 'c':
                print_core_size();
                break;
            case 'C':
                change_core_size(optarg);
                break;
            case 'd':
                print_current_directory();
                break;
            case 'v':
                print_environment();
                break;
            case 'V': {
                char *name, *value;
                if (parse_env_name(optarg, &name, &value) == 0) {
                    set_environment_variable(name, value);
                    free(name);
                    free(value);
                }
                break;
            }
            default:
                exit(EXIT_FAILURE);
        }
    }    
    return EXIT_SUCCESS; 
}
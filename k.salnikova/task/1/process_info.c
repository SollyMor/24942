#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/resource.h>
#include <string.h>
#include <errno.h>
#include <limits.h>

void print_user_group_ids() {
    printf("=== User and Group IDs ===\n");
    printf("Real User ID (RUID): %d\n", getuid());
    printf("Effective User ID (EUID): %d\n", geteuid());
    printf("Real Group ID (RGID): %d\n", getgid());
    printf("Effective Group ID (EGID): %d\n", getegid());
}

void become_process_group_leader() {
    printf("=== Process Group Leadership ===\n");
    if (setpgid(0, 0) == 0) {
        printf("Стал лидером новой группы процессов. Новый PGID: %d\n", getpgrp());
    } else {
        perror("Ошибка при установке лидера группы");
    }
}

void print_process_ids() {
    printf("=== Process Identification ===\n");
    printf("Process ID (PID): %d\n", getpid());
    printf("Parent Process ID (PPID): %d\n", getppid());
    printf("Process Group ID (PGID): %d\n", getpgrp());
}

void print_process_limit() {
    printf("=== Process Limit (ulimit -u) ===\n");
    struct rlimit rlim;
    
    if (getrlimit(RLIMIT_NPROC, &rlim) == 0) {
        printf("Максимальное количество процессов: ");
        if (rlim.rlim_cur == RLIM_INFINITY) {
            printf("неограничен\n");
        } else {
            printf("%ld\n", (long)rlim.rlim_cur);
        }
    } else {
        perror("Ошибка при получении лимита процессов");
    }
}

void print_file_size_limit() {
    printf("=== File Size Limit (ulimit -f) ===\n");
    struct rlimit rlim;
    
    if (getrlimit(RLIMIT_FSIZE, &rlim) == 0) {
        printf("Максимальный размер файла: ");
        if (rlim.rlim_cur == RLIM_INFINITY) {
            printf("неограничен\n");
        } else {
            printf("%ld байт\n", (long)rlim.rlim_cur);
        }
    } else {
        perror("Ошибка при получении ulimit");
    }
}

void print_data_size_limit() {
    printf("=== Data Size Limit (ulimit -d) ===\n");
    struct rlimit rlim;
    
    if (getrlimit(RLIMIT_DATA, &rlim) == 0) {
        printf("Максимальный размер сегмента данных: ");
        if (rlim.rlim_cur == RLIM_INFINITY) {
            printf("неограничен\n");
        } else {
            printf("%ld байт\n", (long)rlim.rlim_cur);
        }
    } else {
        perror("Ошибка при получении лимита данных");
    }
}

void print_stack_size_limit() {
    printf("=== Stack Size Limit (ulimit -s) ===\n");
    struct rlimit rlim;
    
    if (getrlimit(RLIMIT_STACK, &rlim) == 0) {
        printf("Максимальный размер стека: ");
        if (rlim.rlim_cur == RLIM_INFINITY) {
            printf("неограничен\n");
        } else {
            printf("%ld байт\n", (long)rlim.rlim_cur);
        }
    } else {
        perror("Ошибка при получении лимита стека");
    }
}

int set_file_size_limit(const char *value) {
    printf("=== Setting File Size Limit ===\n");
    struct rlimit rlim;
    long new_limit;
    char *endptr;
    
    new_limit = strtol(value, &endptr, 10);
    
    if (endptr == value) {
        fprintf(stderr, "Ошибка: '%s' не является числом\n", value);
        return -1;
    }
    if (*endptr != '\0') {
        fprintf(stderr, "Ошибка: нечисловые символы в '%s'\n", value);
        return -1;
    }
    if (new_limit < 0) {
        fprintf(stderr, "Ошибка: ulimit не может быть отрицательным\n");
        return -1;
    }
    
    if (getrlimit(RLIMIT_FSIZE, &rlim) != 0) {
        perror("Ошибка при получении текущего ulimit");
        return -1;
    }
    
    rlim.rlim_cur = (rlim_t)new_limit;
    if (setrlimit(RLIMIT_FSIZE, &rlim) != 0) {
        perror("Ошибка при установке ulimit");
        return -1;
    }
    
    printf("Лимит размера файла установлен: %ld байт\n", new_limit);
    return 0;
}


int set_process_limit(const char *value) {
    printf("=== Setting Process Limit ===\n");
    struct rlimit rlim;
    long new_limit;
    char *endptr;
    
    new_limit = strtol(value, &endptr, 10);
    
    if (endptr == value) {
        fprintf(stderr, "Ошибка: '%s' не является числом\n", value);
        return -1;
    }
    if (*endptr != '\0') {
        fprintf(stderr, "Ошибка: нечисловые символы в '%s'\n", value);
        return -1;
    }
    if (new_limit < 0) {
        fprintf(stderr, "Ошибка: лимит процессов не может быть отрицательным\n");
        return -1;
    }
    
    if (getrlimit(RLIMIT_NPROC, &rlim) != 0) {
        perror("Ошибка при получении текущего лимита процессов");
        return -1;
    }
    
    rlim.rlim_cur = (rlim_t)new_limit;
    if (setrlimit(RLIMIT_NPROC, &rlim) != 0) {
        perror("Ошибка при установке лимита процессов");
        return -1;
    }
    
    printf("Лимит процессов установлен: %ld\n", new_limit);
    return 0;
}

void print_core_size() {
    printf("=== Core File Size Limit (ulimit -c) ===\n");
    struct rlimit rlim;
    
    if (getrlimit(RLIMIT_CORE, &rlim) == 0) {
        printf("Максимальный размер core-файла: ");
        if (rlim.rlim_cur == RLIM_INFINITY) {
            printf("неограничен\n");
        } else {
            printf("%ld байт\n", (long)rlim.rlim_cur);
        }
    } else {
        perror("Ошибка при получении размера core-файла");
    }
}

int set_core_size(const char *value) {
    printf("=== Setting Core File Size ===\n");
    struct rlimit rlim;
    long new_size;
    char *endptr;
    
    new_size = strtol(value, &endptr, 10);
    
    if (endptr == value) {
        fprintf(stderr, "Ошибка: '%s' не является числом\n", value);
        return -1;
    }
    if (*endptr != '\0') {
        fprintf(stderr, "Ошибка: нечисловые символы в '%s'\n", value);
        return -1;
    }
    if (new_size < 0) {
        fprintf(stderr, "Ошибка: размер core-файла не может быть отрицательным\n");
        return -1;
    }
    
    if (getrlimit(RLIMIT_CORE, &rlim) != 0) {
        perror("Ошибка при получении текущего размера core-файла");
        return -1;
    }
    
    rlim.rlim_cur = (rlim_t)new_size;
    if (setrlimit(RLIMIT_CORE, &rlim) != 0) {
        perror("Ошибка при установке размера core-файла");
        return -1;
    }
    
    printf("Размер core-файла установлен: %ld байт\n", new_size);
    return 0;
}

void print_current_directory() {
    printf("=== Current Working Directory ===\n");
    char cwd[PATH_MAX];
    
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("Текущая директория: %s\n", cwd);
    } else {
        perror("Ошибка при получении текущей директории");
    }
}

void print_environment() {
    printf("=== Environment Variables ===\n");
    extern char **environ;
    
    if (environ == NULL || environ[0] == NULL) {
        printf("Нет переменных окружения\n");
        return;
    }
    
    char **env = environ;
    int count = 0;
    
    while (*env != NULL && count < 10) {
        printf("%d: %s\n", ++count, *env);
        env++;
    }
    if (*env != NULL) {
        printf("... (и еще переменных)\n");
    }
}

int set_environment_variable(const char *name_value) {
    printf("=== Setting Environment Variable ===\n");
    
    if (name_value == NULL || strlen(name_value) == 0) {
        fprintf(stderr, "Ошибка: пустая строка для переменной окружения\n");
        return -1;
    }
    
    char *equals = strchr(name_value, '=');
    if (equals == NULL || equals == name_value) {
        fprintf(stderr, "Ошибка: формат должен быть 'name=value', получено: '%s'\n", 
                name_value ? name_value : "NULL");
        return -1;
    }
    
    size_t name_len = equals - name_value;
    char *name = malloc(name_len + 1);
    if (name == NULL) {
        perror("Ошибка выделения памяти");
        return -1;
    }
    
    strncpy(name, name_value, name_len);
    name[name_len] = '\0';
    char *value = equals + 1;
    
    if (setenv(name, value, 1) != 0) {
        perror("Ошибка при установке переменной окружения");
        free(name);
        return -1;
    }
    
    printf("Установлена переменная окружения: %s=%s\n", name, value);
    free(name);
    return 0;
}

typedef struct {
    char option;
    char *argument;
} command_option_t;

int parse_options_right_to_left(int argc, char *argv[], command_option_t **options) {
    *options = malloc(argc * sizeof(command_option_t));
    if (*options == NULL) {
        perror("Ошибка выделения памяти");
        return -1;
    }
    
    int count = 0;
    
    for (int i = argc - 1; i >= 1; i--) {
        if (argv[i][0] != '-' || strlen(argv[i]) != 2) {
            continue;
        }
        
        char opt = argv[i][1];
        
        if (strchr("ispucdvf", opt) != NULL) {
            (*options)[count].option = opt;
            (*options)[count].argument = NULL;
            count++;
        }
        else if (opt == 'U' || opt == 'C' || opt == 'V' || opt == 'P' || opt == 'F' || opt == 'D' || opt == 'S') {
            if (i + 1 >= argc || argv[i + 1][0] == '-') {
                fprintf(stderr, "Ошибка: опция -%c требует аргумент\n", opt);
                continue;
            }
            
            (*options)[count].option = opt;
            (*options)[count].argument = argv[i + 1];
            count++;
            i--;
        }
        else {
            fprintf(stderr, "Предупреждение: неизвестная опция -%c\n", opt);
        }
    }
    
    return count;
}

int main(int argc, char *argv[]) {
    printf("=== Process Information Tool ===\n");
    
    if (argc == 1) {
        printf("Использование: %s [опции...]\n", argv[0]);
        printf("Опции (обрабатываются справа налево):\n");
        printf("  -i      Показать ID пользователя и группы\n");
        printf("  -s      Стать лидером группы процессов\n");
        printf("  -p      Показать ID процесса\n");
        printf("  -u      Показать лимит процессов (ulimit -u)\n");
        printf("  -f      Показать лимит размера файлов (ulimit -f)\n");
        printf("  -d      Показать лимит данных (ulimit -d)\n");
        printf("  -c      Показать размер core-файла (ulimit -c)\n");
        printf("  -v      Показать переменные окружения\n");
        printf("  -U size Установить лимит процессов\n");
        printf("  -F size Установить лимит размера файлов\n");
        printf("  -D size Установить лимит данных\n");
        printf("  -C size Установить размер core-файла\n");
        printf("  -S size Установить лимит стека\n");
        printf("  -V var=value Установить переменную окружения\n");
        return 0;
    }

    command_option_t *options = NULL;
    int option_count = parse_options_right_to_left(argc, argv, &options);
    
    if (option_count < 0) {
        fprintf(stderr, "Ошибка при разборе опций\n");
        return 1;
    }
    
    printf("Обработка %d опций справа налево...\n\n", option_count);

    for (int i = 0; i < option_count; i++) {
        switch (options[i].option) {
            case 'i':
                print_user_group_ids();
                break;
            case 's':
                become_process_group_leader();
                break;
            case 'p':
                print_process_ids();
                break;
            case 'u':
                print_process_limit();
                break;
            case 'f':
                print_file_size_limit();
                break;
            case 'd':
                print_data_size_limit(); 
                break;
            case 'c':
                print_core_size();     
                break;
            case 'U':
                if (options[i].argument != NULL) {
                    set_process_limit(options[i].argument);
                } else {
                    fprintf(stderr, "Ошибка: опция -U требует значение\n");
                }
                break;
            case 'F':
                if (options[i].argument != NULL) {
                    set_file_size_limit(options[i].argument);
                } else {
                    fprintf(stderr, "Ошибка: опция -F требует значение\n");
                }
                break;
            case 'D':
                if (options[i].argument != NULL) {
                    printf("Установка лимита данных: %s\n", options[i].argument);
                } else {
                    fprintf(stderr, "Ошибка: опция -D требует значение\n");
                }
                break;
            case 'C':
                if (options[i].argument != NULL) {
                    set_core_size(options[i].argument);
                } else {
                    fprintf(stderr, "Ошибка: опция -C требует значение\n");
                }
                break;
            case 'S':
                if (options[i].argument != NULL) {

                    printf("Установка лимита стека: %s\n", options[i].argument);
                } else {
                    fprintf(stderr, "Ошибка: опция -S требует значение\n");
                }
                break;
            case 'v':
                print_environment();
                break;
            case 'V':
                if (options[i].argument != NULL) {
                    set_environment_variable(options[i].argument);
                } else {
                    fprintf(stderr, "Ошибка: опция -V требует значение\n");
                }
                break;
            default:
                fprintf(stderr, "Неизвестная опция: -%c\n", options[i].option);
                break;
        }
        printf("\n");
    }

    free(options);
    return 0;
}

/* 
gcc -o process_info process_info.c
./process_info -u
*/
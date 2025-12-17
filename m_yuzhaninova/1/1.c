#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/resource.h>
#include <getopt.h>
#include <errno.h>

extern char **environ;

void show_limit(const char *title, int res)
{
    struct rlimit limit;
    if (getrlimit(res, &limit) != 0)
    {
        perror("getrlimit");
        return;
    }
    printf("%s: soft = %ld, hard = %ld\n", title, (long)limit.rlim_cur, (long)limit.rlim_max);
}

int main(int argc, char *argv[])
{
    int opt;
    struct
    {
        int flag;
        char *param;
    } flags[argc];
    int count = 0;

    pid_t pid;
    struct rlimit limit;
    long num = 0;
    char dir[1000];
    char *param = NULL;
    char *sep = NULL;
    char *name = NULL;
    char *value = NULL;
    char param_copy[1000];
    char **env = NULL;
    FILE *file = NULL;
    char cmd[256];
    char buf[256];

    struct option long_opts[] =
    {
        {"Unew_ulimit", required_argument, 0, 'U'},
        {"Csize", required_argument, 0, 'C'},
        {"Vname", required_argument, 0, 'V'},
        {0, 0, 0, 0}
    };

    opterr = 0;

    while ((opt = getopt_long(argc, argv, ":is:puU:cC:dvV:", long_opts, NULL)) != -1)
    {
        flags[count].flag = opt;
        flags[count].param = optarg;
        count++;
    }

    // обрабатываем в обратном порядке
    for (int i = count - 1; i >= 0; i--)
    {
        opt = flags[i].flag;
        param = flags[i].param;

        switch (opt)
        {
            case 'i':
                printf("Реальный UID: %d\n", getuid());
                printf("Эффективный UID: %d\n", geteuid());
                printf("Реальный GID: %d\n", getgid());
                printf("Эффективный GID: %d\n", getegid());
                break;

            case 's':
                num = strtol(param, NULL, 10);
                pid = (pid_t)num;

                if (setpgid(pid, pid) != 0)
                {
                    perror("setpgid");
                }
                else
                {
                    printf("Процесс %d стал лидером группы: %d\n", pid, getpgid(pid));
                }
                break;

            case 'p':
                printf("PID: %d\nPPID: %d\nPGID: %d\n", getpid(), getppid(), getpgid(0));
                break;

            case 'u':
                #ifdef __sun
                    file = popen("ulimit -u 2>/dev/null", "r");
                    if (file && fgets(buf, sizeof(buf), file))
                    {
                        printf("Ограничение через ulimit: %s", buf);
                    }
                    if (file)
                        pclose(file);
                #else
                    show_limit("Ограничение на количество процессов", RLIMIT_NPROC);
                #endif
                break;

            case 'U':
                num = strtol(param, NULL, 10);
                #ifdef __sun
                    if (num <= 0)
                    {
                        fprintf(stderr, "Неверное значение для лимита процессов: %s\n",num);
                        break;
                    }

                    snprintf(cmd, sizeof(cmd), "ulimit -u %ld 2>/dev/null", num);
                    int ret = system(cmd);
                    if (ret != 0)
                    {
                        fprintf(stderr, "Ошибка установки лимита процессов (Solaris)\n");
                    }
                #else
                    if (getrlimit(RLIMIT_NPROC, &limit) == 0)
                    {
                        limit.rlim_cur = num;
                        if (setrlimit(RLIMIT_NPROC, &limit) != 0)
                        {
                            perror("setrlimit");
                        }
                    }
                    else
                    {
                        perror("getrlimit");
                    }
                #endif
                break;

            case 'c':
                show_limit("Ограничение на размер core-файла", RLIMIT_CORE);
                break;

            case 'C':
                num = strtol(param, NULL, 10);
                if (getrlimit(RLIMIT_CORE, &limit) == 0)
                {
                    limit.rlim_cur = num;
                    if (setrlimit(RLIMIT_CORE, &limit) != 0)
                    {
                        perror("setrlimit");
                    }
                }
                else
                {
                    perror("getrlimit");
                }

                break;

            case 'd':
                if (getcwd(dir, sizeof(dir)))
                {
                    printf("Текущая директория: %s\n", dir);
                }
                else
                {
                    perror("getcwd");
                }
                break;

            case 'v':
                for (env = environ; *env; env++)
                {
                    printf("%s\n", *env);
                }
                break;

            case 'V':
                if (!param || !strchr(param, '='))
                {
                    fprintf(stderr, "Неверный формат, нужно name=value\n");
                    break;
                }
                strncpy(param_copy, param, sizeof(param_copy) - 1);
                param_copy[sizeof(param_copy) - 1] = '\0';
                sep = strchr(param_copy, '=');
                *sep = '\0';
                name = param_copy;
                value = sep + 1;

                if (setenv(name, value, 1) != 0)
                {
                    perror("setenv");
                }
                break;

            case ':':
                fprintf(stderr, "Опция -%c требует аргумента\n", optopt);
                break;

            default:
                printf("Неизвестная опция\n");
                break;
        }
    }

    return 0;
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/resource.h>
#include <getopt.h>
#include <errno.h>

extern char **environ;

void print_limit(const char *name, int resource)
{
    struct rlimit rl;
    if (getrlimit(resource, &rl) != 0)
    {
        fprintf(stderr, "Ошибка getrlimit: %s\n", strerror(errno));
        return;
    }
    printf("%s: soft = %ld, hard = %ld\n", name, (long)rl.rlim_cur, (long)rl.rlim_max);
}

int main(int argc, char* argv[])
{
    int opt;
    struct 
    {
        int opt;
        char *arg;
    } ops[argc];
    int ops_count = 0;

    pid_t pid;
    struct rlimit rl; 
    long new = 0, val = 0;
    char path[1000];
    char *arg = NULL;
    char *eq = NULL;
    char *name = NULL;
    char *value = NULL;
    char *wrong = NULL;
    char arg_copy[1000];
    char **env = NULL;
    FILE *fp = NULL;
    char cmd[256];
    char buf[256];

    struct option long_options[] = 
    {
        {"Unew_ulimit", required_argument, 0, 'U'},
        {"Csize", required_argument, 0, 'C'},
        {"Vname", required_argument, 0, 'V'},
        {0, 0, 0, 0}
    };

    opterr = 0;

    // Сохраняем все опции
    while((opt = getopt_long(argc, argv, ":is:puU:cC:dvV:", long_options, NULL)) != -1)
    {
        ops[ops_count].opt = opt;
        ops[ops_count].arg = optarg;
        ops_count++;
    }

    // Обрабатываем справа налево
    for(int i = ops_count - 1; i >= 0; i--)
    {
        opt = ops[i].opt;
        arg = ops[i].arg;

        switch (opt)
        {
            case 'i':
                printf("Реальный UID: %d\n", (int)getuid());
                printf("Эффективный UID: %d\n", (int)geteuid());
                printf("Реальный GID: %d\n", (int)getgid());
                printf("Эффективный GID: %d\n", (int)getegid());
                break;

            case 's':
                errno = 0;
                val = strtol(arg, &wrong, 10);
                if (errno != 0 || *wrong != '\0' || val < 0)
                {
                    fprintf(stderr, "Неверный PID: %s\n", arg);
                    break;
                }
                pid = (pid_t)val;

                if (setpgid(pid, pid) != 0)
                {
                    fprintf(stderr, "Ошибка setpgid: %s\n", strerror(errno));
                }
                else
                {
                    printf("Процесс %d стал лидером группы: %d\n", pid, getpgid(pid));
                }
                break;

            case 'p':
                printf("Текущий процесс: %d\nРодитель текущего процесса: %d\nИдентификатор группы процессов: %d\n",
                       getpid(), getppid(), getpgid(0));
                break;

            case 'u':
                #ifdef __sun
                    fp = popen("ulimit -u 2>/dev/null", "r");
                    if (fp && fgets(buf, sizeof(buf), fp))
                    {
                        printf("Ограничение через ulimit: %s", buf);
                    }
                    if (fp)
                        pclose(fp);
                #else
                    print_limit("Ограничение на количество процессов", RLIMIT_NPROC);
                #endif
                break;

            case 'U':
                #ifdef __sun
                    errno = 0;
                    val = strtol(arg, &wrong, 10);
                    if (errno != 0 || *wrong != '\0' || val <= 0)
                    {
                        fprintf(stderr, "Неверное значение для лимита процессов: %s\n", arg);
                        break;
                    }

                    snprintf(cmd, sizeof(cmd), "ulimit -u %ld 2>/dev/null", val);
                    int ret = system(cmd);
                    if (ret != 0)
                    {
                        fprintf(stderr, "Ошибка установки лимита процессов (Solaris)\n");
                    }
                #else
                    errno = 0;
                    val = strtol(arg, &wrong, 10);
                    if (errno != 0 || *wrong != '\0' || val < 0)
                    {
                        fprintf(stderr, "Неверное значение для ulimit: %s\n", arg);
                        break;
                    }

                    if (getrlimit(RLIMIT_NPROC, &rl) != 0)
                    {
                        fprintf(stderr, "Ошибка getrlimit: %s\n", strerror(errno));
                        break;
                    }

                    if (val > rl.rlim_max)
                    {
                        fprintf(stderr, "Значение слишком большое");
                        break;
                    }
                    else
                    {
                        rl.rlim_cur = val;
                    }
                    
                    if (setrlimit(RLIMIT_NPROC, &rl) != 0)
                    {
                        fprintf(stderr, "Ошибка setrlimit: %s\n", strerror(errno));
                    }
                #endif
                break;

            case 'c':
                print_limit("Ограничение на размер core-файла", RLIMIT_CORE);
                break;

            case 'C':
                errno = 0;
                val = strtol(arg, &wrong, 10);
                if (errno != 0 || *wrong != '\0' || val < 0)
                {
                    fprintf(stderr, "Неверное значение для core size: %s\n", arg);
                    break;
                }
                if (getrlimit(RLIMIT_CORE, &rl) != 0)
                {
                    fprintf(stderr, "Ошибка getrlimit: %s\n", strerror(errno));
                    break;
                }

                if (val > rl.rlim_max)
                {
                    fprintf(stderr, "Значение слишком большое");
                    break;
                }
                else
                {
                    rl.rlim_cur = val;
                }

                if (setrlimit(RLIMIT_CORE, &rl) != 0)
                {
                    fprintf(stderr, "Ошибка setrlimit: %s\n", strerror(errno));
                }
                break;

            case 'd':
                if (getcwd(path, sizeof(path)) != NULL)
                    printf("Текущая директория: %s\n", path);
                else
                    fprintf(stderr, "Ошибка getcwd: %s\n", strerror(errno));
                break;

            case 'v':
                env = environ;
                while (*env)
                {
                    printf("%s\n", *env);
                    env++;
                }
                break;

            case 'V':
                if (!arg || !strchr(arg, '='))
                {
                    fprintf(stderr, "Неверный формат, нужно name=value\n");
                    break;
                }
                strncpy(arg_copy, arg, sizeof(arg_copy)-1);
                arg_copy[sizeof(arg_copy)-1] = '\0';
                eq = strchr(arg_copy, '=');
                *eq = '\0';
                name = arg_copy;
                value = eq + 1;
                if (strlen(name) == 0)
                {
                    fprintf(stderr, "Имя переменной не может быть пустым\n");
                    break;
                }
                if (setenv(name, value, 1) != 0)
                    fprintf(stderr, "Ошибка setenv: %s\n", strerror(errno));
                break;

            case ':':
                fprintf(stderr, "Опция -%c требует аргумента\n", optopt);
                break;

            default:
                printf("Такой опции нет\n");
                break;
        }
    }

    return 0;
}
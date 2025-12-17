#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>

volatile sig_atomic_t timeout = 0;

void on_alarm(int sig)
{
    (void)sig;
    timeout = 1;
}

int main()
{
    int fd = open("text.txt", O_RDONLY);
    if (fd == -1)
    {
        perror("Ошибка открытия файла");
        return 1;
    }

    long offsets[1000] = {0};
    int lengths[1000];
    int count = 0, len = 0;
    long pos = 0;
    char ch;

    while (read(fd, &ch, 1) == 1)
    {
        pos++;
        len++;
        if (ch == '\n')
        {
            lengths[count] = len;
            offsets[++count] = pos;
            len = 0;
        }
    }

    if (len > 0)
    {
        lengths[count++] = len;
    }

    printf("Таблица строк (позиция, длина):\n");
    for (int i = 0; i < count; i++)
    {
        printf("Строка %d: позиция = %ld, длина = %d\n", i + 1, offsets[i], lengths[i]);
    }

    struct sigaction sa = {0};
    sa.sa_handler = on_alarm;
    sigaction(SIGALRM, &sa, NULL);
    fflush(stdout);
    alarm(5);
    while (1)
    {
        printf("\nВведите номер строки (0 — выход): ");
        fflush(stdout);

        timeout = 0;

        char buf[100];
        int idx = 0;
        char c;
        while (!timeout && read(STDIN_FILENO, &c, 1) == 1 && c != '\n' && idx < 100)
        {
            buf[idx++] = c;
        }
        buf[idx] = '\0';
        alarm(0);

        if (timeout)
        {
            printf("\nВремя вышло! Выводим весь файл:\n\n");
            lseek(fd, 0, SEEK_SET);
            char dump[512];
            int r;
            while ((r = read(fd, dump, sizeof(dump))) > 0)
            {
                write(STDOUT_FILENO, dump, r);
            }
            write(1, "\n", 1);
            break;
        }

        if (idx == 0)
        {
            printf("Некорректный ввод.\n");
            continue;
        }

        int num, t;
        if (sscanf(buf, "%d%n", &num, &t) != 1) 
        {
            fprintf(stderr, "Некорректный ввод\n");
            continue;
        }
        else
        {
            while (buf[t] == ' ' || buf[t] == '\t') 
            {
                t++;
            }
            if (buf[t] != '\n' && buf[t] != '\0')
            {
                fprintf(stderr, "Некорректный ввод\n");
                continue;
            }
        }

        int n = atoi(buf);
        if (n == 0)
        {
            break;
        }

        if (n < 1 || n > count)
        {
            printf("Такой строки нет.\n");
            continue;
        }

        lseek(fd, offsets[n - 1], SEEK_SET);

        char *line = malloc(lengths[n - 1] + 1);
        if (!line)
        {
            perror("malloc");
            continue;
        }

        int bytes = read(fd, line, lengths[n - 1]);
        line[bytes] = '\0';
        if (bytes > 0 && line[bytes - 1] == '\n')
        {
            line[bytes - 1] = '\0';
        }

        printf("Строка %d: %s\n", n, line);
        free(line);
    }

    close(fd);
    return 0;
}

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>

volatile sig_atomic_t timeout;

void new_alarm(int sig)
{
    (void)sig;
    timeout = 1;
}

int main()
{
    int fd=open("data.txt", O_RDONLY);
    if (fd == -1) 
    {
        fprintf(stderr, "Ошибка открытия файла\n");
        return 1;
    }

    long text[1000];  
    long pos = 0; 
    int len = 0;
    char symbol;
    int lengths[1000];
    int count = 0;

    text[0] = 0;

    while (read(fd, &symbol, 1) == 1)
    {
        pos++;
        len++;
        if (symbol == '\n')
        {
            if (count >= 1000) 
            {
                fprintf(stderr, "Превышено максимальное количество строк\n");
                break;
            }
            lengths[count] = len;
            count++;
            text[count] = pos;
            len = 0;
        }
    }

    if (len > 0 && count < 1000)
    {
        lengths[count] = len;
        text[count + 1] = pos;
        count++;
    }

    printf("Таблица строк (отступ, длина):\n");
    for (int i = 0; i < count; i++)
    {
        printf("Строка %d: отступ = %ld, длина = %d\n", i + 1, text[i], lengths[i]);
    }

    struct sigaction s;
    memset(&s, 0, sizeof(s));

    s.sa_handler = new_alarm;
    sigaction(SIGALRM, &s, NULL);
    fflush(stdout);

    alarm(5);
    while (1)
    {
        int n;
        char buffer[1000];

        timeout = 0;
        printf("Введите номер строки (0 для выхода): ");
	fflush(stdout);
        int i = 0;
        while (i < sizeof(buffer) - 1)
        {
            char c;
            long r = read(STDIN_FILENO, &c, 1);
            if (r == -1)
            {
                if (errno == EINTR)
                {
                    if (timeout) 
                    {
                        break;
                    }
                    continue; 
                }
                printf("Ошибка ввода\n");
                i = 0;
                break;
            }
            if (r == 0) 
            {
                break;
            }
            buffer[i++] = c;
            if (c == '\n') 
            {
                break;
            }
        }
        buffer[i] = '\0';

        alarm(0);

        if (i == 0 && timeout)
        {
            printf("\nВремя вышло! Содержимое файла:\n\n");
            char dump[1000];
            lseek(fd, 0, SEEK_SET);
            int r;
            while ((r = read(fd, dump, sizeof(dump))) > 0)
            {
                write(1, dump, r);
            }
            write(1, "\n", 1);
            break;
        }
        else if (i == 0)
        {
            printf("Ошибка ввода.\n");
            continue;
        }

        if (sscanf(buffer, "%d", &n) != 1)
        {
            printf("Некорректный ввод числа.\n");
            continue;
        }

        if (n == 0)
        {
            break;
        }

        if (n < 1 || n > count)
        {
            printf("Такой строки нет\n");
            continue;
        }

        if (lseek(fd, text[n - 1], SEEK_SET) == -1) 
        {
            fprintf(stderr, "Ошибка lseek");
            continue;
        }

        char *line = malloc(lengths[n-1] + 1);
        if (!line) 
        {
            fprintf(stderr, "Ошибка malloc");
            continue;
        }

        int bytes_read = read(fd, line, lengths[n-1]);
        if (bytes_read != lengths[n-1]) 
        {
            fprintf(stderr, "Ошибка чтения");
            free(line);
            continue;
        }

        line[bytes_read] = '\0';
        if (bytes_read > 0 && line[bytes_read - 1] == '\n') 
        {
            line[bytes_read - 1] = '\0';
        }

        printf("Строка %d: %s\n", n, line);
        free(line);
    }

    close(fd);
    return 0;
}

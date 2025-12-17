#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/stat.h>

volatile sig_atomic_t timeout = 0;

void on_alarm(int sig)
{
    (void)sig;
    timeout = 1;
}

int main(void)
{
    int fd = open("text.txt", O_RDONLY);
    if (fd == -1)
    {
        perror("Ошибка открытия файла");
        return 1;
    }

    struct stat st;
    if (fstat(fd, &st) == -1)
    {
        perror("Ошибка fstat");
        close(fd);
        return 1;
    }

    if (st.st_size == 0)
    {
        printf("Файл пустой.\n");
        close(fd);
        return 0;
    }

    long filesize = st.st_size;
    char *data = mmap(NULL, filesize, PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);

    if (data == MAP_FAILED)
    {
        perror("Ошибка mmap");
        return 1;
    }

    long offsets[1000] = {0};
    int lengths[1000];
    int count = 0, len = 0;

    for (long i = 0; i < filesize; i++)
    {
        len++;
        if (data[i] == '\n')
        {
            lengths[count] = len;
            offsets[++count] = i + 1;
            len = 0;
            if (count >= 999)
            {
                break;
            }
        }
    }
    if (len > 0 && count < 1000)
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
            printf("\nВремя вышло! Содержимое файла:\n\n");
            write(STDOUT_FILENO, data, filesize);
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

        long start = offsets[n - 1];
        long length = lengths[n - 1];
        if (start + length > filesize)
        {
            length = filesize - start;
        }
        if (length > 0 && data[start + length - 1] == '\n')
        {
            length--;
        }

        printf("Строка %d: ", n);
        fwrite(data + start, 1, length, stdout);
        printf("\n");
    }

    munmap(data, filesize);
    return 0;
}

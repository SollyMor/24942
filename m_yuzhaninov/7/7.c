#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/stat.h>

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

    struct stat st;
    if (fstat(fd, &st) == -1)
    {
        fprintf(stderr, "Ошибка fstat");
        close(fd);
        return 1;
    }

    long filesize = st.st_size;
    if (filesize == 0)
    {
        fprintf(stderr, "Файл пустой");
        close(fd);
        return 0;
    }

    char *data = mmap(NULL, filesize, PROT_READ, MAP_PRIVATE, fd, 0);
    if (data == MAP_FAILED)
    {
        fprintf(stderr, "Ошибка mmap");
        close(fd);
        return 1;
    }

    close(fd);

    long text[1000];  
    int len = 0;
    int lengths[1000];
    int count = 0;

    text[0] = 0;

    for (long i = 0; i < filesize; i++)
    {
        len++;
        if (data[i] == '\n')
        {
            if (count >= 1000) 
            {
                fprintf(stderr, "Превышено максимальное количество строк\n");
                break;
            }
            lengths[count] = len;
            count++;
            text[count] = i + 1;
            len = 0;
        }
    }

    if (len > 0 && count < 1000)
    {
        lengths[count] = len;
        text[count + 1] = filesize;
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
            write(STDOUT_FILENO, data, filesize);
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

        long start = text[n - 1];
        long length = lengths[n - 1];
        if (start + length > (long)filesize)
            length = filesize - start;

        // Убираем \n, если есть
        if (length > 0 && data[start + length - 1] == '\n')
            length--;

        printf("Строка %d: ", n);
        fwrite(data + start, 1, length, stdout);
        printf("\n");
    }

    munmap(data, filesize);
    return 0;
}

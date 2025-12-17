#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>


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

    while (1)
    {
        int n;
        printf("Введите номер строки (0 для выхода): ");
        if (scanf("%d", &n) != 1) 
        {
            fprintf(stderr, "Некорректный ввод\n");
            while (getchar() != '\n');
            continue;
        }
        if (n == 0)
        {
            break;
        }

        if (n < 1 || n > count)
        {
            printf("Нет такой строки.\n");
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
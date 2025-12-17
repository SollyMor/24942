#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>

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
            count++;
            offsets[count] = pos;
            len = 0;
        }
    }

    if (len > 0)
    {
        lengths[count] = len;
        count++;
    }

    printf("Таблица строк (позиция, длина):\n");
    for (int i = 0; i < count; i++)
    {
        printf("%2d: %5ld %5d\n", i + 1, offsets[i], lengths[i]);
    }

    while (1)
    {
        int n, t;
        printf("Введите номер строки (0 для выхода): ");

        if (scanf("%d%n", &n, &t) != 1) 
        {
            fprintf(stderr, "Некорректный ввод\n");
            while (getchar() != '\n');
            continue;
        }
        else
        {
            int c = getchar();
            if (c != '\n')
            {
                fprintf(stderr, "Некорректный ввод\n");
                while (c != '\n')
                {
                    c = getchar();
                }
                continue;
            }
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

        lseek(fd, offsets[n - 1], SEEK_SET);

        char *line = malloc(lengths[n - 1] + 1);
        if (!line)
        {
            perror("Ошибка памяти");
            continue;
        }

        int bytes = read(fd, line, lengths[n - 1]);
        line[bytes] = '\0';
        if (line[bytes - 1] == '\n')
        {
            line[bytes - 1] = '\0';
        }

        printf("Строка %d: %s\n", n, line);
        free(line);
    }

    close(fd);
    return 0;
}

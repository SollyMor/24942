#define _POSIX_C_SOURCE 200809L
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <limits.h>

typedef struct
{
    off_t offset;
    off_t length;
} Line;

typedef struct
{
    Line *array;
    int cnt;
    int cap;
} Array;

static void die(const char *msg)
{
    perror(msg);
    exit(1);
}

static void initArray(Array *a)
{
    a->cap = 16;
    a->cnt = 0;
    a->array = (Line *)malloc(a->cap * sizeof(Line));
    if (!a->array)
        die("malloc");
}

static void insertArray(Array *a, Line element)
{
    if (a->cnt == a->cap)
    {
        int newcap = a->cap * 2;
        Line *p = (Line *)realloc(a->array, newcap * sizeof(Line));
        if (!p)
            die("realloc");
        a->array = p;
        a->cap = newcap;
    }
    a->array[a->cnt++] = element;
}

static void freeArray(Array *a)
{
    free(a->array);
    a->array = NULL;
    a->cnt = a->cap = 0;
}

// Строим таблицу по отображённому файлу. Учитываем CRLF: если перед '\n' был '\r', длину уменьшаем на 1.
static void build_table_from_map(const char *data, size_t size, Array *table)
{
    off_t line_off = 0;
    int last_was_cr = 0;

    for (size_t i = 0; i < size; ++i)
    {
        char c = data[i];
        if (c == '\n')
        {
            off_t cur = (off_t)i;       // позиция '\n'
            off_t len = cur - line_off; // длина до '\n'
            if (last_was_cr && len > 0)
                --len; // убрать '\r' перед '\n', если был
            Line L = (Line){line_off, len};
            insertArray(table, L);

            line_off = (off_t)i + 1;
            last_was_cr = 0;
        }
        else
        {
            last_was_cr = (c == '\r');
        }
    }

    // Последняя строка без завершающего '\n'
    if ((off_t)size > line_off)
    {
        off_t len = (off_t)size - line_off;
        if (last_was_cr && len > 0)
            --len;
        Line L = (Line){line_off, len};
        insertArray(table, L);
    }
}

static void printLine_from_map(const char *data, Line line)
{
    // Безопасная печать ровно length байт и '\n'
    const char *p = data + line.offset;
    size_t n = (size_t)line.length;
    if (n)
        fwrite(p, 1, n, stdout);
    fputc('\n', stdout);
    fflush(stdout);
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <path>\n", argv[0]);
        return 1;
    }
    const char *path = argv[1];

    // --- Открываем и отображаем файл ---
    int fd = open(path, O_RDONLY);
    if (fd == -1)
        die("open");

    struct stat st;
    if (fstat(fd, &st) == -1)
    {
        close(fd);
        die("fstat");
    }

    if (st.st_size < 0)
    {
        close(fd);
        fprintf(stderr, "negative file size\n");
        return 1;
    }
    if ((unsigned long long)st.st_size > (unsigned long long)SIZE_MAX)
    {
        close(fd);
        fprintf(stderr, "file too large to map in this process\n");
        return 1;
    }

    size_t fsize = (size_t)st.st_size;

    // Пустой файл: просто пустая таблица
    char *map = NULL;
    if (fsize > 0)
    {
        map = (char *)mmap(NULL, fsize, PROT_READ, MAP_PRIVATE, fd, 0);
        if (map == MAP_FAILED)
        {
            close(fd);
            die("mmap");
        }
    }
    // Дескриптор больше не нужен
    close(fd);

    // --- Строим таблицу строк ---
    Array table;
    initArray(&table);
    if (fsize > 0)
        build_table_from_map(map, fsize, &table);

    // --- Диалог: первый запрос с таймаутом 5с, далее без ограничений ---
    int first_prompt = 1;
    char ibuf[64];

    for (;;)
    {
        printf("Enter the line number: ");
        fflush(stdout);

        if (first_prompt)
        {
            fd_set fdset;
            struct timeval timeout;
            FD_ZERO(&fdset);
            FD_SET(STDIN_FILENO, &fdset);
            timeout.tv_sec = 5;
            timeout.tv_usec = 0;

            int sel = select(STDIN_FILENO + 1, &fdset, NULL, NULL, &timeout);
            if (sel == 0)
            {
                // Таймаут: распечатать весь файл построчно и выйти
                printf("\n\n");
                for (int i = 0; i < table.cnt; ++i)
                {
                    printLine_from_map(map, table.array[i]);
                }
                if (map && fsize)
                    munmap(map, fsize);
                freeArray(&table);
                return 0;
            }
            else if (sel < 0)
            {
                perror("select");
                break;
            }
            first_prompt = 0;
        }

        if (!fgets(ibuf, sizeof ibuf, stdin))
            break;
        char *end = NULL;
        errno = 0;
        long num = strtol(ibuf, &end, 10);
        if (errno || end == ibuf)
        {
            printf("Please enter a valid integer.\n");
            continue;
        }
        if (num == 0)
            break;

        if (num < 0 || num > table.cnt)
        {
            printf("The file contains only %d line(s).\n", table.cnt);
            continue;
        }

        Line line = table.array[num - 1];
        printLine_from_map(map, line);
    }

    if (map && fsize)
        munmap(map, fsize);
    freeArray(&table);
    return 0;
}

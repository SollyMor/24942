#define _XOPEN_SOURCE 700
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

typedef struct {
    off_t offset;   // смещение начала строки
    off_t length;   // длина строки без '\n'
} Line;

typedef struct {
    Line  *array;
    size_t cnt;
    size_t cap;
} Array;

static void initArray(Array *a) {
    a->array = NULL;
    a->cnt = 0;
    a->cap = 0;
}

static int push(Array *a, Line x) {
    if (a->cnt == a->cap) {
        size_t newcap = a->cap ? a->cap * 2 : 64;
        void *p = realloc(a->array, newcap * sizeof(Line));
        if (!p) return -1;
        a->array = (Line*)p;
        a->cap = newcap;
    }
    a->array[a->cnt++] = x;
    return 0;
}

static void freeArray(Array *a) {
    free(a->array);
    a->array = NULL;
    a->cnt = a->cap = 0;
}

static void print_table(const Array *t) {
    printf("TABLE (line  offset  length)\n");
    for (size_t i = 0; i < t->cnt; ++i) {
        printf("%5zu  %6lld  %6lld\n",
               i + 1,
               (long long)t->array[i].offset,
               (long long)t->array[i].length);
    }
    printf("Total lines: %zu\n\n", t->cnt);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <file>\n", argv[0]);
        return 1;
    }

    int fd = open(argv[1], O_RDONLY);
    if (fd < 0) { perror("open"); return 1; }

    Array table; initArray(&table);

    // --- ПРОХОД 1: строим таблицу offset/length ---
    off_t pos = 0;   // текущая позиция (байт от начала)
    off_t len = 0;   // текущая длина строки (без '\n')
    char ch;
    ssize_t r;

    while ((r = read(fd, &ch, 1)) == 1) {
        pos++;
        if (ch == '\n') {
            Line L = (Line){ .offset = pos - 1 - len, .length = len };
            if (push(&table, L) < 0) { perror("realloc"); close(fd); freeArray(&table); return 1; }
            len = 0;
        } else {
            len++;
        }
    }
    if (r < 0) { perror("read"); close(fd); freeArray(&table); return 1; }

    // Последняя строка без завершающего '\n'
    if (len > 0) {
        Line L = (Line){ .offset = pos - len, .length = len };
        if (push(&table, L) < 0) { perror("realloc"); close(fd); freeArray(&table); return 1; }
    }

    // --- ПЕЧАТЬ ТАБЛИЦЫ ---
    print_table(&table);

    // --- ПРОХОД 2: чтение выбранных строк по таблице ---
    for (;;) {
        printf("Enter the line number (0 to quit): ");
        int num;
        if (scanf("%d", &num) != 1) {
            fprintf(stderr, "Invalid input\n");
            break;
        }
        if (num == 0) break;

        if (num < 0 || (size_t)num > table.cnt) {
            printf("The file contains only %zu line(s).\n", table.cnt);
            continue;
        }

        Line L = table.array[num - 1];
        if (lseek(fd, L.offset, SEEK_SET) == (off_t)-1) { perror("lseek"); break; }

        char *buf = (char*)malloc((size_t)L.length + 1);
        if (!buf) { perror("malloc"); break; }

        ssize_t need = (ssize_t)L.length, got = 0;
        while (got < need) {
            r = read(fd, buf + got, (size_t)(need - got));
            if (r <= 0) { perror("read line"); free(buf); close(fd); freeArray(&table); return 1; }
            got += r;
        }
        buf[L.length] = '\0';
        printf("%s\n", buf);
        free(buf);
    }

    close(fd);
    freeArray(&table);
    return 0;
}


#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/select.h>

typedef struct {
    off_t offset;
    off_t length;
} Row;

typedef struct {
    Row *table;
    int cnt;
    int cap;
} Table;

void init_table(Table *a) {
    a->table = malloc(sizeof(Row));
    a->cnt = 0;
    a->cap = 1;
}

void insert_row(Table *a, Row row) {
    if (a->cnt == a->cap) {
        a->cap *= 2;
        a->table = realloc(a->table, a->cap * sizeof(Row));
    }

    a->table[a->cnt++] = row;
}

void free_table(Table *a) {
    free(a->table);
    a->table = NULL;
    a->cnt = a->cap = 0;
}

void print_row(Row row, int fd) {
    char *buf = calloc(row.length + 1, sizeof(char));

    lseek(fd, row.offset, SEEK_SET);
    read(fd, buf, row.length * sizeof(char));

    printf("%s\n", buf);
    free(buf);
}

int main(int argc, char *argv[]) {
    if (argc != 2) { return 1; }
    char *path = argv[1];

    Table table;
    init_table(&table);

    int fd = open(path, O_RDONLY);
    if (fd == -1) { return 1; }
    lseek(fd, 0L, SEEK_CUR);

    char c;
    off_t line_offset = 0;
    off_t line_length = 0;
    while (read(fd, &c, 1) == 1) {
        if (c == '\n') {
            Row current = {line_offset, line_length};
            insert_row(&table, current);

            line_offset += line_length + 1;
            line_length = 0;
        } else {
            line_length++;
        }
    }

    if (line_length > 0) {
        Row current = {line_offset, line_length};
        insert_row(&table, current);
    }

    fd_set fdset;
    struct timeval timeout;

    while (1) {
        printf("Enter the line number: ");

        fflush(stdout);

        FD_ZERO(&fdset);
        FD_SET(STDIN_FILENO, &fdset);
        timeout.tv_sec = 5;
        timeout.tv_usec = 0;

        if (!select(1, &fdset, NULL, NULL, &timeout)) {
            printf("\n\n");
            for(int i = 0; i < table.cnt; i++)
                print_row(table.table[i], fd);
            return 0;
        }

        int num;
        scanf("%d", &num);

        if (num == 0) { break; }
        if (table.cnt < num) {
            printf("The file contains only %d line(s).\n", table.cnt);
            continue;
        }

        Row row = table.table[num - 1];
        char *buf = calloc(row.length + 1, sizeof(char));

        lseek(fd, row.offset, SEEK_SET);
        read(fd, buf, row.length * sizeof(char));

        printf("%s\n", buf);
        free(buf);
    }

    close(fd);
    free_table(&table);

    return 0;
}

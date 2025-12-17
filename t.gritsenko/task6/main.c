#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <inttypes.h>

typedef struct {
    int line_number;
    off_t offset;
    off_t length;
} Row;

typedef struct {
    Row *table;
    int cnt;
    int cap;
} Table;

Table table;
int fd;

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

void print_table(int fd, Table a) {
    if (a.cnt == 0) {
        printf("(Table is empty.)\n");
        return;
    }

    printf("\n");
    printf("+------------+------------+------------+----------------------------------------------+\n");
    printf("|  Line Num  |   Offset   |   Length   | Text                                         |\n");
    printf("+------------+------------+------------+----------------------------------------------+\n");

    for (int i = 0; i < a.cnt; i++) {
        Row row = a.table[i];
        char *buf = calloc(row.length + 1, sizeof(char));

        lseek(fd, row.offset, SEEK_SET);
        read(fd, buf, row.length);
        buf[row.length] = '\0';

        for (size_t j = 0; j < row.length; j++) {
            if (buf[j] == '\n' || buf[j] == '\r')
                buf[j] = ' ';
        }

        printf("| %10d | %10jd | %10jd | %s\n",
               row.line_number,
               (intmax_t)row.offset,
               (intmax_t)row.length,
               buf);

        free(buf);
    }

    printf("+------------+------------+------------+----------------------------------------------+\n");
}

void timeout_handler(int sig) {
    printf("\nTime is up!\n");
    print_table(fd, table);
    free_table(&table);
    close(fd);
    exit(0);
}

int main(int argc, char *argv[]) {
    if (argc != 2) { return 1; }
    char *path = argv[1];

    init_table(&table);
    
    fd = open(path, O_RDONLY);
    if (fd == -1) { return 1; }
    lseek(fd, 0L, SEEK_CUR);

    char c;
    int line_number = 0;
    off_t line_offset = 0;
    off_t line_length = 0;
    while (read(fd, &c, 1) == 1) {
        if (c == '\n') {
            line_number++;
            Row current = {line_number, line_offset, line_length};
            insert_row(&table, current);

            line_offset += line_length + 1;
            line_length = 0;
        } else {
            line_length++;
        }
    }

    if (line_length > 0) {
        line_number++;
        Row current = {line_number, line_offset, line_length};
        insert_row(&table, current);
    }

    signal(SIGALRM, timeout_handler);

    printf("Enter the line number: ");
    fflush(stdout);
    alarm(5);

    int num;
    if (scanf("%d", &num) != 1) { return 1; }
    alarm(0);
    if (num == 0) { return 0; }
    if (table.cnt < num) {
        printf("The file contains only %d line(s).\n", table.cnt);
    } else {
        Row row = table.table[num - 1];
        print_row(row, fd);
    }

    while (1) {

        printf("Enter the line number: ");
        fflush(stdout);
        if (scanf("%d", &num) != 1) { break; }
        if (num == 0) { break; }
        if (table.cnt < num) {
            printf("The file contains only %d line(s).\n", table.cnt);
            continue;
        }

        Row row = table.table[num - 1];
        print_row(row, fd);
    }

    close(fd);
    free_table(&table);

    return 0;
}

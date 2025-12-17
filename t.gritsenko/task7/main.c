#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/stat.h>
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
char *mapped;

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

void print_row(Row row, char *mapped) {
    for (int i = 0; i < row.length; i++) {
        printf("%c", mapped[row.offset + i]);
    }
    printf("\n");
}

void print_table(Table a) {
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

        printf("| %10d | %10jd | %10jd | ",
               row.line_number,
               (intmax_t)row.offset,
               (intmax_t)row.length);
        print_row(row, mapped);
    }

    printf("+------------+------------+------------+----------------------------------------------+\n");
}


void timeout_handler(int sig) {
    printf("\nTime is up!\n");
    print_table(table);
    free_table(&table);
    exit(0);
}

int main(int argc, char *argv[]) {
    if (argc != 2) { return 1; }
    char *path = argv[1];

    init_table(&table);

    int fd = open(path, O_RDONLY);
    if (fd == -1) { return 1; }
    
    struct stat file_info;
    if (fstat(fd, &file_info) == -1) { return 1; }
    size_t size = file_info.st_size;

    mapped = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (mapped == MAP_FAILED) { return 1; }
    close(fd);

    int line_number = 0;
    off_t line_offset = 0;
    off_t line_length = 0;

    for (int i = 0; i < size; i++) {
        char c = mapped[i];
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
        print_row(row, mapped);
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
        print_row(row, mapped);
    }

    munmap((void *) mapped, size);
    free_table(&table);

    return 0;
}

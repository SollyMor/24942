#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/select.h>
#include <signal.h>
#include <stdlib.h>

typedef struct {
    off_t offset;
    off_t length;
} Line;

typedef struct {
    Line* array;
    int cnt;
    int cap;
} Array;
int fd;

void initArray(Array* a) {
    a->array = malloc(sizeof(Line));
    a->cnt = 0;
    a->cap = 1;
}

void insertArray(Array* a, Line element) {
    if (a->cnt == a->cap) {
        a->cap *= 2;
        a->array = realloc(a->array, a->cap * sizeof(Line));
    }

    a->array[a->cnt++] = element;
}

void freeArray(Array* a) {
    free(a->array);
    a->array = NULL;
    a->cnt = a->cap = 0;
}

void signal_handler(int signo){
    if (signo != SIGALRM) return;
    printf("\n\nAlarm triggered");
    lseek(fd, 0, SEEK_SET); // Go to start of file
    char buf[1024];
    ssize_t n;
    printf("\nAll lines from file:\n");
    while ((n = read(fd, buf, sizeof(buf)-1)) > 0) {
        buf[n] = '\0';
        printf("%s", buf);
    }
    printf("\nExiting...");
    exit(0);
}

int main(int argc, char* argv[]) {
    if (argc != 2) { 
        return 1; 
    }
    char* path = argv[1];
    Array table;

    initArray(&table);

    fd = open(path, O_RDONLY);
    if (fd == -1) {
        return 1; 
    }
    if (signal(SIGALRM, signal_handler) == SIG_ERR) {
        fprintf(stderr, "An error occurred while setting a signal handler.\n");
        return 1;
    }
    // Get current position as mentioned in hint
    off_t current_pos = lseek(fd, 0L, SEEK_CUR);
    printf("Starting file analysis at position: %ld\n", current_pos);

    char c;
    off_t lineOffset = 0; //Offset of the line in the file
    off_t lineLength = 0; 
    while (read(fd, &c, 1) == 1) {
        if (c == '\n') {
            Line current = { lineOffset, lineLength };
            insertArray(&table, current);

            lineOffset += lineLength + 1;
            lineLength = 0;
        }
        else {
            lineLength++;
        }
    }

    if (lineLength > 0) {
        Line current = { lineOffset, lineLength };
        insertArray(&table, current);
    }

    // Print debugging table as mentioned in comments
    printf("\nLine Table (for debugging):\n");
    printf(" Line | Offset | Length\n");
    printf("------|--------|-------\n");
    for (int i = 0; i < table.cnt; i++) {
        printf("%5d | %6ld | %6ld\n", i + 1, table.array[i].offset, table.array[i].length);
    }
    printf("\nTotal lines: %d\n\n", table.cnt);
    alarm(5);

    while (1) {
        int num;
        printf("Enter the line number: ");

        scanf("%d", &num);


        if (num == 0) { break; }
        if (table.cnt < num || num < 1) {
            printf("The file contains only %d line(s).\n", table.cnt);
            continue;
        }

        Line line = table.array[num - 1]; //Line
        char* buf = calloc(line.length + 1, sizeof(char)); //Buffer

        if (lseek(fd, line.offset, SEEK_SET) == -1) {
            perror("Error seeking in file");
            free(buf);
            continue;
        }
        
        if (read(fd, buf, line.length) == -1) {
            perror("Error reading line");
            free(buf);
            continue;
        }

        printf("Line %d: %s\n", num, buf);
        free(buf);
    }

    close(fd);
    freeArray(&table);

    return 0;
}
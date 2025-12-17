#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/select.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/stat.h>

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
char* mapped_file;
size_t file_size;

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
    printf("\nAll lines from file:\n");
    // Print entire mapped file content
    for (size_t i = 0; i < file_size; i++) {
        printf("%c", mapped_file[i]);
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
    
    // Get file size
    struct stat file_stat;
    if (fstat(fd, &file_stat) == -1) {
        close(fd);
        return 1;
    }
    file_size = file_stat.st_size;
    
    // Map file to memory
    mapped_file = mmap(NULL, file_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (mapped_file == MAP_FAILED) {
        close(fd);
        return 1;
    }
    if (signal(SIGALRM, signal_handler) == SIG_ERR) {
        fprintf(stderr, "An error occurred while setting a signal handler.\n");
        return 1;
    }
    printf("Starting file analysis with memory mapping\n");

    off_t lineOffset = 0; //Offset of the line in the file
    off_t lineLength = 0; 
    for (size_t i = 0; i < file_size; i++) {
        char c = mapped_file[i];
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

    while (1) {
        int num;
        printf("Enter the line number: ");
        alarm(5);

        scanf("%d", &num);

        if (num == 0) { break; }
        if (table.cnt < num || num < 1) {
            printf("The file contains only %d line(s).\n", table.cnt);
            continue;
        }

        Line line = table.array[num - 1]; //Line
        char* buf = calloc(line.length + 1, sizeof(char)); //Buffer

        // Copy line from mapped memory
        for (off_t i = 0; i < line.length; i++) {
            buf[i] = mapped_file[line.offset + i];
        }
        buf[line.length] = '\0';

        printf("Line %d: %s\n", num, buf);
        free(buf);
    }

    // Unmap memory and close file
    munmap(mapped_file, file_size);
    close(fd);
    freeArray(&table);

    return 0;
}
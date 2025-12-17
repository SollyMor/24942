#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
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

// Global variables for timeout handling and memory mapping
static int timeout_occurred = 0;
static int fd_global = -1;
static Array* table_global = NULL;
static char* mapped_file = NULL;
static size_t file_size = 0;

// Signal handler for alarm
void timeout_handler(int sig) {
    if (sig == SIGALRM) {
        timeout_occurred = 1;
        printf("\nTimeout! 5 seconds elapsed. Printing entire file:\n");
        printf("==========================================\n");
        
        // Print entire file content using memory mapping
        if (mapped_file != NULL) {
            fwrite(mapped_file, 1, file_size, stdout);
        }
        
        printf("\n==========================================\n");
        printf("Program finished due to timeout.\n");
        
        // Clean up and exit
        if (mapped_file != NULL) {
            munmap(mapped_file, file_size);
        }
        close(fd_global);
        freeArray(table_global);
        exit(0);
    }
}

int main(int argc, char* argv[]) {
    if (argc != 2) { 
        return 1; 
    }
    char* path = argv[1];

    Array table;
    initArray(&table);

    int fd = open(path, O_RDONLY);
    if (fd == -1) {
        return 1; 
    }
    
    // Get file size for memory mapping
    struct stat file_stat;
    if (fstat(fd, &file_stat) == -1) {
        close(fd);
        return 1;
    }
    file_size = file_stat.st_size;
    
    // Map the file into memory
    mapped_file = mmap(NULL, file_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (mapped_file == MAP_FAILED) {
        close(fd);
        return 1;
    }
    
    // Set up global variables for signal handler
    fd_global = fd;
    table_global = &table;
    
    // Set up signal handler for alarm
    signal(SIGALRM, timeout_handler);
    printf("Starting file analysis with memory mapping. File size: %zu bytes\n", file_size);

    // Analyze file using memory mapping instead of read()
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

    // Set alarm for 5 seconds (only for the first prompt)
    printf("You have 5 seconds to enter a line number. If no input, entire file will be printed.\n");
    alarm(5);
    int first_prompt = 1;
    
    while (1) {
        int num;
        char input[100];
        printf("Enter the line number: ");
        fflush(stdout);
        
        // Check if timeout occurred (only for the first prompt)
        if (first_prompt && timeout_occurred) {
            break;
        }
        
        // Read input as string to validate
        if (fgets(input, sizeof(input), stdin) == NULL) {
            printf("Input error. Please try again.\n");
            // Reset alarm only while first prompt is active
            if (first_prompt) alarm(5);
            continue;
        }
        
        // Remove newline character
        input[strcspn(input, "\n")] = '\0';
        
        // Validate input - only accept numbers
        int valid = 1;
        for (int i = 0; input[i] != '\0'; i++) {
            if (input[i] < '0' || input[i] > '9') {
                valid = 0;
                break;
            }
        }
        
        if (!valid) {
            printf("Invalid input. Please enter only numbers (0-9).\n");
            // Reset alarm only while first prompt is active
            if (first_prompt) alarm(5);
            continue;
        }
        
        // Convert string to number
        num = atoi(input);
        
        // Cancel alarm since user provided first valid input
        if (first_prompt) {
            alarm(0);
            first_prompt = 0;
        }

        if (num == 0) { break; }
        if (table.cnt < num) {
            printf("The file contains only %d line(s).\n", table.cnt);
            // No more alarm after the first valid input
            continue;
        }

        Line line = table.array[num - 1]; //Line
        
        // Use memory mapping to access the line directly
        if (line.offset < 0 || line.length < 0 || 
            (size_t)(line.offset + line.length) > file_size) {
            printf("Error: Line extends beyond file size or has invalid offset/length\n");
            // No more alarm after the first valid input
            continue;
        }
        
        // Print line directly from memory mapping
        printf("Line %d: ", num);
        fwrite(mapped_file + line.offset, 1, line.length, stdout);
        printf("\n");
        
        // No more alarm after the first valid input
    }

    // Clean up memory mapping
    if (mapped_file != NULL) {
        munmap(mapped_file, file_size);
    }
    close(fd);
    freeArray(&table);

    return 0;
}
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

typedef struct {
    long offset;
    int length;
} LineInfo;

// Global variables for storing line information
LineInfo *line_table = NULL;
int total_lines = 0;
int table_capacity = 0;

int expand_table() {
    int new_capacity = table_capacity == 0 ? 10 : table_capacity * 2;
    LineInfo *new_table = realloc(line_table, new_capacity * sizeof(LineInfo));
    if (new_table == NULL) {
        perror("realloc");
        return -1;
    }
    line_table = new_table;
    table_capacity = new_capacity;
    return 0;
}

int add_line(long offset, int length) {
    if (total_lines >= table_capacity) {
        if (expand_table() == -1) {
            return -1;
        }
    }
    
    line_table[total_lines].offset = offset;
    line_table[total_lines].length = length;
    total_lines++;
    return 0;
}

int build_line_table(int fd) {
    char buffer[1];
    long current_offset = 0;
    int line_start = 0;
    int line_length = 0;
    
    printf("Building line table...\n");
    
    // Move to the beginning of the file
    if (lseek(fd, 0L, SEEK_SET) == -1) {
        perror("lseek");
        return -1;
    }
    
    while (read(fd, buffer, 1) > 0) {
        if (buffer[0] == '\n') {
            // Found newline character
            if (add_line(line_start, line_length) == -1) {
                return -1;
            }
            line_start = current_offset + 1;
            line_length = 0;
        } else {
            line_length++;
        }
        current_offset++;
    }
    
    // Handle the last line if the file doesn't end with \n
    if (line_length > 0) {
        if (add_line(line_start, line_length) == -1) {
            return -1;
        }
    }
    
    printf("Total lines in file: %d\n", total_lines);
    return 0;
}

int read_line(int fd, int line_number, char **buffer, int *buffer_size) {
    if (line_number < 0 || line_number >= total_lines) {
        return -1;
    }
    
    // Move to the beginning of the line
    if (lseek(fd, line_table[line_number].offset, SEEK_SET) == -1) {
        perror("lseek");
        return -1;
    }
    
    // Allocate memory for the line
    int line_length = line_table[line_number].length;
    *buffer_size = line_length + 1;
    *buffer = malloc(*buffer_size);
    if (*buffer == NULL) {
        perror("malloc");
        return -1;
    }
    
    // Read the line
    int bytes_read = read(fd, *buffer, line_length);
    if (bytes_read == -1) {
        perror("read");
        free(*buffer);
        return -1;
    }
    
    (*buffer)[bytes_read] = '\0';
    return bytes_read;
}

void print_debug_table() {
    printf("\n=== DEBUG TABLE ===\n");
    printf("Line Number | Offset | Length\n");
    printf("------------|--------|-------\n");
    
    for (int i = 0; i < total_lines; i++) {
        printf("%11d | %6ld | %4d\n", 
               i + 1, line_table[i].offset, line_table[i].length);
    }
    printf("========================\n\n");
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <filename>\n", argv[0]);
        exit(1);
    }
    
    char *filename = argv[1];
    int fd;
    
    // Open the file
    fd = open(filename, O_RDONLY);
    if (fd == -1) {
        perror("open");
        exit(1);
    }
    
    printf("File '%s' opened successfully\n", filename);
    
    // Build the line table
    if (build_line_table(fd) == -1) {
        close(fd);
        exit(1);
    }
    
    // Print debug table
    print_debug_table();
    
    // Main program loop
    int line_number;
    char *line_buffer = NULL;
    int buffer_size = 0;
    
    printf("Program ready. Enter line number (0 to exit):\n");
    
    while (1) {
        printf("> ");
        if (scanf("%d", &line_number) != 1) {
            printf("Input error. Enter a number.\n");
            // Clear input buffer
            int c;
            while ((c = getchar()) != '\n' && c != EOF);
            continue;
        }
        
        if (line_number == 0) {
            printf("Program termination.\n");
            break;
        }
        
        if (line_number > total_lines) {
            printf("Error: line number must be from 1 to %d\n", total_lines);
            continue;
        }
        
        // Read line (numbering starts from 1, but array from 0)
        int bytes_read = read_line(fd, line_number - 1, &line_buffer, &buffer_size);
        if (bytes_read == -1) {
            printf("Error reading line %d\n", line_number);
            continue;
        }
        
        printf("Line %d: \"%s\"\n", line_number, line_buffer);
        
        // Free memory
        free(line_buffer);
        line_buffer = NULL;
    }
    
    // Free table memory
    free(line_table);
    close(fd);
    return 0;
}
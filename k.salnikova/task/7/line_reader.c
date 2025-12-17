#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <signal.h>
#include <time.h>
#include <sys/select.h>

#define MAX_LINES 1000
#define BUFFER_SIZE 1024

struct LineInfo {
    off_t offset;
    size_t length;
};

char *file_data = NULL;
size_t file_size = 0;
struct LineInfo *lines_global;
int line_count_global;
volatile sig_atomic_t timeout_occurred = 0;

void alarm_handler(int sig) {
    timeout_occurred = 1;
}

void print_entire_file(char *data, struct LineInfo *lines, int line_count) {
    printf("\n ВРЕМЯ ВЫШЛО! Вывод всего файла:\n");
    printf("========================================\n");
    
    for (int i = 0; i < line_count; i++) {
        char *line_start = data + lines[i].offset;
        printf("%d: ", i + 1);
        for (size_t j = 0; j < lines[i].length; j++) {
            putchar(line_start[j]);
        }
        printf("\n");
    }
    printf("========================================\n");
}

int main(int argc, char *argv[]) {
    int fd;
    struct LineInfo lines[MAX_LINES];
    int line_count = 0;
    struct stat file_stat;
    
    if (argc != 2) {
        fprintf(stderr, "Использование: %s <файл>\n", argv[0]);
        return 1;
    }
    
    fd = open(argv[1], O_RDONLY);
    if (fd == -1) {
        perror("Ошибка открытия файла");
        return 1;
    }
    
    if (fstat(fd, &file_stat) == -1) {
        perror("Ошибка получения информации о файле");
        close(fd);
        return 1;
    }
    
    file_size = file_stat.st_size;
    
    file_data = mmap(NULL, file_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (file_data == MAP_FAILED) {
        perror("Ошибка отображения файла в память");
        close(fd);
        return 1;
    }
    
    printf("=== АНАЛИЗ ФАЙЛА: %s (размер: %ld байт) ===\n", argv[1], file_size);
    
    lines[0].offset = 0;
    size_t line_start = 0;
    
    for (size_t i = 0; i < file_size; i++) {
        if (file_data[i] == '\n') {
            lines[line_count].length = i - line_start;
            line_count++;
            
            if (line_count >= MAX_LINES) break;
            
            line_start = i + 1;
            lines[line_count].offset = line_start;
        }
    }
    
    if (line_count < MAX_LINES && line_start < file_size) {
        lines[line_count].length = file_size - line_start;
        line_count++;
    }
    
    lines_global = lines;
    line_count_global = line_count;
    
    printf("Найдено строк: %d\n", line_count);
    
    printf("\nТАБЛИЦА СТРОК:\n");
    printf("№\tСмещение\tДлина\n");
    printf("--\t--------\t-----\n");
    for (int i = 0; i < line_count; i++) {
        printf("%d\t%ld\t\t%zu\n", i + 1, (long)lines[i].offset, lines[i].length);
    }
    
    signal(SIGALRM, alarm_handler);
    
    printf("\n У вас 5 секунд чтобы ввести номер строки!\n");
    printf("Введите номер строки (1-%d), 0 для выхода:\n", line_count);
    
    int line_number;
    char input_buffer[BUFFER_SIZE];
    int timeout_active = 1;
    
    alarm(5);
    
    while (1) {
        printf("Введите номер строки: ");
        fflush(stdout);
        
        if (timeout_occurred) {
            printf("\n");
            print_entire_file(file_data, lines, line_count);
            break;
        }

        fd_set readfds;
        struct timeval tv;
        
        FD_ZERO(&readfds);
        FD_SET(STDIN_FILENO, &readfds);
        
        if (timeout_active) {
            tv.tv_sec = 5;
            tv.tv_usec = 0;
        } else {

            tv.tv_sec = 0;
            tv.tv_usec = 0;
        }
        
        int ready = select(STDIN_FILENO + 1, &readfds, NULL, NULL, 
                          timeout_active ? &tv : NULL);
        
        if (timeout_occurred) {
            printf("\n");
            print_entire_file(file_data, lines, line_count);
            break;
        }
        
        if (ready == 0 && timeout_active) {
            printf("\n");
            print_entire_file(file_data, lines, line_count);
            break;
        }
        else if (ready > 0) {
            if (fgets(input_buffer, sizeof(input_buffer), stdin) != NULL) {
                if (timeout_active) {
                    alarm(0);
                    timeout_active = 0;
                    timeout_occurred = 0;
                }

                input_buffer[strcspn(input_buffer, "\n")] = 0;
                
                if (sscanf(input_buffer, "%d", &line_number) != 1) {
                    printf("Ошибка ввода. Введите число.\n");
                    continue;
                }
                
                if (line_number == 0) {
                    printf("Завершение работы.\n");
                    break;
                }
                
                if (line_number < 1 || line_number > line_count) {
                    printf("Ошибка: номер строки должен быть от 1 до %d\n", line_count);
                    continue;
                }
                
                int index = line_number - 1;
                char *line_start_ptr = file_data + lines[index].offset;
                size_t line_length = lines[index].length;
                
                printf("Строка %d: ", line_number);
                for (size_t i = 0; i < line_length; i++) {
                    putchar(line_start_ptr[i]);
                }
                printf("\n");
            }
        } else if (ready == -1) {
            perror("Ошибка select");
            break;
        }
    }
    
    if (file_data != NULL && file_data != MAP_FAILED) {
        munmap(file_data, file_size);
    }
    close(fd);
    
    return 0;
}
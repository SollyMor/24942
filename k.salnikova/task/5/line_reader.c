#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#define MAX_LINES 1000
#define BUFFER_SIZE 1024

struct LineInfo {
    off_t offset;
    size_t length;
};

int main(int argc, char *argv[]) {
    int fd;
    char ch;
    struct LineInfo lines[MAX_LINES];
    int line_count = 0;
    ssize_t bytes_read;
    
    if (argc != 2) {
        fprintf(stderr, "Использование: %s <файл>\n", argv[0]);
        return 1;
    }
    

    fd = open(argv[1], O_RDONLY);
    if (fd == -1) {
        perror("Ошибка открытия файла");
        return 1;
    }
    
    printf("=== АНАЛИЗ ФАЙЛА: %s ===\n", argv[1]);

    lines[0].offset = 0;
    off_t line_start = 0;
    

    off_t current_pos = lseek(fd, 0L, SEEK_CUR);
    
    while ((bytes_read = read(fd, &ch, 1)) > 0) {
        if (ch == '\n') {
            current_pos = lseek(fd, 0L, SEEK_CUR);
            lines[line_count].length = current_pos - 1 - line_start; 
            
            line_count++;
            
            if (line_count >= MAX_LINES) break;
            
            line_start = current_pos;
            lines[line_count].offset = line_start;
        }
    }
    

    current_pos = lseek(fd, 0L, SEEK_CUR);
    if (current_pos > line_start && line_count < MAX_LINES) {
        lines[line_count].length = current_pos - line_start;
        line_count++;
    }
    
    printf("Найдено строк: %d\n", line_count);
    
    printf("\nТАБЛИЦА СТРОК:\n");
    printf("№\tСмещение\tДлина\n");
    printf("--\t--------\t-----\n");
    for (int i = 0; i < line_count; i++) {
        printf("%d\t%ld\t\t%zu\n", i + 1, (long)lines[i].offset, lines[i].length);
    }
    
    printf("\nВведите номер строки (1-%d), 0 для выхода:\n", line_count);
    
    int line_number;
    char line_buffer[BUFFER_SIZE];
    
    while (1) {
        printf("Введите номер строки: ");
        
        if (scanf("%d", &line_number) != 1) {
            printf("Ошибка ввода.\n");
            while (getchar() != '\n');
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
        
        // lseek(2) - позиционирование к началу строки
        if (lseek(fd, lines[index].offset, SEEK_SET) == (off_t)-1) {
            perror("Ошибка позиционирования");
            continue;
        }
        
        // read(2) - чтение строки точной длины
        ssize_t read_bytes = read(fd, line_buffer, lines[index].length);
        if (read_bytes == -1) {
            perror("Ошибка чтения строки");
            continue;
        }
        
        line_buffer[read_bytes] = '\0';
        
        printf("Строка %d: ", line_number);
    
        for (int i = 0; i < read_bytes; i++) {
            putchar(line_buffer[i]);
        }
        printf("\n");
    }
    
    close(fd);
    return 0;
}

// gcc -o line_reader_fixed line_reader.c
// ./line_reader_fixed testfile.txt


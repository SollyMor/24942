#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>

int main() {
    char filename[256];
    int fd;
    struct stat file_stat;
    char *file_data;
    
    printf("Введите имя файла: ");
    fgets(filename, sizeof(filename), stdin);
    filename[strcspn(filename, "\n")] = '\0';
    
    fd = open(filename, O_RDONLY);
    if (fd == -1) {
        perror("Ошибка открытия файла");
        exit(1);
    }
    
    if (fstat(fd, &file_stat) == -1) {
        perror("Ошибка получения информации о файле");
        close(fd);
        exit(1);
    }
    
    // Отображаем файл в память
    file_data = mmap(NULL, file_stat.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (file_data == MAP_FAILED) {
        perror("Ошибка отображения файла в память");
        close(fd);
        exit(1);
    }
    
    // Строим таблицу строк (используя mmap)
    off_t *offsets = malloc(file_stat.st_size * sizeof(off_t));
    int *lengths = malloc(file_stat.st_size * sizeof(int));
    int line_count = 0;
    
    offsets[0] = 0;
    int current_length = 0;
    
    for (off_t i = 0; i < file_stat.st_size; i++) {
        if (file_data[i] == '\n') {
            lengths[line_count] = current_length;
            line_count++;
            offsets[line_count] = i + 1;
            current_length = 0;
        } else {
            current_length++;
        }
    }
    
    if (current_length > 0) {
        lengths[line_count] = current_length;
        line_count++;
    }
    
    printf("Файл содержит %d строк\n", line_count);
    
    // Основной цикл запросов
    int line_number;
    char input[16];
    
    while (1) {
        printf("Введите номер строки (0 для выхода): ");
        fgets(input, sizeof(input), stdin);
        line_number = atoi(input);
        
        if (line_number == 0) break;
        if (line_number < 1 || line_number > line_count) {
            printf("Неверный номер строки. Допустимый диапазон: 1-%d\n", line_count);
            continue;
        }
        
        // Выводим строку непосредственно из памяти
        printf("Строка %d: ", line_number);
        for (int i = 0; i < lengths[line_number - 1]; i++) {
            putchar(file_data[offsets[line_number - 1] + i]);
        }
        printf("\n");
    }
    
    // Освобождаем ресурсы mmap
    munmap(file_data, file_stat.st_size);
    close(fd);
    free(offsets);
    free(lengths);
    
    return 0;
}
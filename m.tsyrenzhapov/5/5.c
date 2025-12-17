#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

int main() {
    char filename[256];
    int fd;
    struct stat file_stat;
    
    printf("Введите имя файла: ");
    fgets(filename, sizeof(filename), stdin);
    filename[strcspn(filename, "\n")] = '\0';
    
    // Открываем файл
    fd = open(filename, O_RDONLY);
    if (fd == -1) {
        perror("Ошибка открытия файла");
        exit(1);
    }
    
    // Получаем информацию о файле
    if (fstat(fd, &file_stat) == -1) {
        perror("Ошибка получения информации о файле");
        close(fd);
        exit(1);
    }
    
    // Строим таблицу смещений и длин строк
    off_t *offsets = malloc(file_stat.st_size * sizeof(off_t));
    int *lengths = malloc(file_stat.st_size * sizeof(int));
    int line_count = 0;
    char buffer;
    off_t current_offset = 0;
    int current_length = 0;
    
    offsets[0] = 0;
    
    // Читаем файл и заполняем таблицу
    while (read(fd, &buffer, 1) > 0) {
        if (buffer == '\n') {
            lengths[line_count] = current_length;
            line_count++;
            offsets[line_count] = current_offset + 1;
            current_length = 0;
        } else {
            current_length++;
        }
        current_offset++;
    }
    
    // Обрабатываем последнюю строку
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
        
        // Позиционируемся и читаем строку
        lseek(fd, offsets[line_number - 1], SEEK_SET);
        printf("Строка %d: ", line_number);
        for (int i = 0; i < lengths[line_number - 1]; i++) {
            read(fd, &buffer, 1);
            putchar(buffer);
        }
        printf("\n");
    }
    
    close(fd);
    free(offsets);
    free(lengths);
    return 0;
}
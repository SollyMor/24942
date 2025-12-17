#include <stdio.h> // СДАЛ
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#define MAX_LINES 1000
#define MAX_LINE_LENGTH 1024

// Структура для хранения позиций и длин строк
typedef struct {
    off_t offset;    // Позиция начала строки в файле
    ssize_t length;   // Длина строки (без \n)
} LineInfo;


int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Использование: %s <имя_файла>\n", argv[0]);
        return 1;
    }

    // Открываем файл
    int fd = open(argv[1], O_RDONLY);
    if (fd == -1) {
        perror("Ошибка открытия файла");
        return 1;
    }

    LineInfo lines[MAX_LINES];
    int line_count = 0;
    off_t current_offset = 0;
    char buffer[1024];
    size_t bytes_read;

    // Построение таблицы отступов и длин строк
    printf("=== Построение таблицы отступов ===\n");
    
    while ((bytes_read = read(fd, buffer, sizeof(buffer))) > 0) {
        for (int i = 0; i < bytes_read; i++) {
            if (buffer[i] == '\n') {
                // Записываем информацию о строке
                if (line_count < MAX_LINES) {
                    lines[line_count].offset = current_offset;
                    lines[line_count].length = lseek(fd, 0L, SEEK_CUR) - current_offset - bytes_read + i;
                    
                    printf("Строка %d: offset=%ld, length=%zu\n", 
                           line_count + 1, 
                           (long)lines[line_count].offset, 
                           lines[line_count].length);
                    
                    line_count++;
                }
                current_offset = lseek(fd, 0L, SEEK_CUR) - bytes_read + i + 1;
            }
        }
    }

    // Обработка последней строки (если файл не заканчивается на \n)
    if (current_offset < lseek(fd, 0L, SEEK_END)) {
        lines[line_count].offset = current_offset;
        lines[line_count].length = lseek(fd, 0L, SEEK_END) - current_offset;
        
        printf("Строка %d: offset=%ld, length=%zu\n", 
                           line_count + 1, 
                           (long)lines[line_count].offset, 
                           lines[line_count].length);
        line_count++;
    }

    // Основной цикл запросов
    while (1) {
        int line_num;
        printf("\nВведите номер строки (0 для выхода):");

        if (scanf("%d", &line_num) != 1) {
            printf("Ошибка ввода\n");
            while (getchar() != '\n'); // Очистка буфера
            continue;
        }

        if (line_num == 0) {
            printf("Завершение работы\n");
            break;
        }

        if (line_num < 1 || line_num > line_count) {
            printf("Некорректный номер строки");
            continue;
        }

        // Позиционируемся на начало строки
        LineInfo *line = &lines[line_num - 1];
        if (lseek(fd, line->offset, SEEK_SET) == -1) {
            perror("Ошибка позиционирования");
            continue;
        }

        // Читаем строку
        char line_buffer[MAX_LINE_LENGTH];
        size_t to_read = (line->length < sizeof(line_buffer) - 1) ? 
                         line->length : sizeof(line_buffer) - 1;
        ssize_t read_bytes = read(fd, line_buffer, to_read);


        if (read_bytes > 0) {
            line_buffer[read_bytes] = '\0';
            printf("Строка %d: %s\n", line_num, line_buffer);
        } else if (read_bytes == 0){
            printf("Cтрока пустая\n");
        } else{
            printf("Ошибка чтения строки");
        }
    }

    close(fd);
    return 0;
}
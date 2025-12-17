#include <stdio.h> // СДАЛ
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>

#define MAX_LINES 1000
#define MAX_LINE_LENGTH 1024

int line_count = 0;
int fd = -1;

// Структура для хранения позиций и длин строк
typedef struct {
    off_t offset;    // Позиция начала строки в файле
    size_t length;   // Длина строки (без \n)
    char buffer_data[MAX_LINE_LENGTH];
} LineInfo;

LineInfo lines[MAX_LINES];

static void alarmHandler(int signo){
    printf("\nВремя кончилось!\n");

    printf("Содержимое файла: \n");
    for (int i = 0; i < line_count; i++){
        printf("%s\n", lines[i].buffer_data);
    }
    
    if (fd != -1) {
        close(fd);
    }

    exit(1);
}


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

    off_t current_offset = 0;
    char buffer[MAX_LINE_LENGTH];
    size_t bytes_read;

    // Построение таблицы отступов и длин строк
    printf("=== Построение таблицы отступов ===\n");
    
    while ((bytes_read = read(fd, buffer, sizeof(buffer))) > 0) {
        for (int i = 0; i < bytes_read; i++) {
            if (buffer[i] == '\n') {
                // Сохраняем текущую позицию в файле
                off_t file_pos = lseek(fd, 0L, SEEK_CUR);
                
                // Записываем информацию о строке
                if (line_count < MAX_LINES) {
                    lines[line_count].offset = current_offset;
                    lines[line_count].length = file_pos - bytes_read + i - current_offset;
                    
                    // Вычисляем позицию в буфере
                    size_t buffer_start_pos = current_offset - (file_pos - bytes_read);
                    if (buffer_start_pos < bytes_read && lines[line_count].length < MAX_LINE_LENGTH) {
                        memcpy(lines[line_count].buffer_data, &buffer[buffer_start_pos], lines[line_count].length);
                        lines[line_count].buffer_data[lines[line_count].length] = '\0';
                    }
                    
                    printf("Строка %d: offset=%ld, length=%zu\n", 
                           line_count + 1, 
                           (long)lines[line_count].offset, 
                           lines[line_count].length);
                    
                    line_count++;
                }
                
                current_offset = file_pos - bytes_read + i + 1;
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



    signal(SIGALRM, alarmHandler); // устанавливаем обработчик сигнала
    alarm(5);
    // Основной цикл запросов
    while (1) {
        int line_num;
        printf("\nВведите номер строки (0 для выхода). У вас есть 5 секунд чтобы ввести данные:");
        

        if (scanf("%d", &line_num) != 1) {
            printf("Ошибка ввода\n");
            while (getchar() != '\n'); // Очистка буфера
            alarm(0);
            continue;
        }

        alarm(0); // отключение таймера

        if (line_num == 0) {
            printf("Завершение работы\n");
            break;
        }

        if (line_num < 1 || line_num > line_count) {
            printf("Некорректный номер строки");
            continue;
        }

        
        int index = line_num - 1;
        
        if (lines[index].length > 0) {
            printf("Строка %d: %s\n", line_num, lines[index].buffer_data);
        } else if (lines[index].length == 0) {
            printf("Строка пустая\n");
        } else {
            printf("Ошибка чтения строки\n");
        }
    }

    close(fd);
    return 0;
}
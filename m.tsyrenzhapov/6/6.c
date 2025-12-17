#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>

volatile sig_atomic_t timeout_occurred = 0;

void alarm_handler(int sig) {
    timeout_occurred = 1;
}

int main() {
    char filename[256];
    int fd;
    struct stat file_stat;
    
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
    
    // Построение таблицы строк (как в задании 5)
    off_t *offsets = malloc(file_stat.st_size * sizeof(off_t));
    int *lengths = malloc(file_stat.st_size * sizeof(int));
    int line_count = 0;
    char buffer;
    off_t current_offset = 0;
    int current_length = 0;
    
    offsets[0] = 0;
    
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
    
    if (current_length > 0) {
        lengths[line_count] = current_length;
        line_count++;
    }
    
    printf("Файл содержит %d строк\n", line_count);
    
    // Настройка обработчика сигнала ALARM
    signal(SIGALRM, alarm_handler);
    
    int line_number;
    char input[16];
    
    printf("У вас 5 секунд чтобы ввести номер строки...\n");
    alarm(5);
    
    if (fgets(input, sizeof(input), stdin) != NULL) {
        alarm(0); // Отключаем таймер
        line_number = atoi(input);
        
        if (line_number != 0 && line_number >= 1 && line_number <= line_count) {
            lseek(fd, offsets[line_number - 1], SEEK_SET);
            printf("Строка %d: ", line_number);
            for (int i = 0; i < lengths[line_number - 1]; i++) {
                read(fd, &buffer, 1);
                putchar(buffer);
            }
            printf("\n");
        }
    }
    
    // Если сработал таймер
    if (timeout_occurred) {
        printf("\nВремя вышло! Содержимое файла:\n");
        lseek(fd, 0, SEEK_SET);
        while (read(fd, &buffer, 1) > 0) {
            putchar(buffer);
        }
    }
    
    close(fd);
    free(offsets);
    free(lengths);
    return 0;
}
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/stat.h>

typedef struct 
{
    off_t offset;
    off_t length;
} Line;

typedef struct 
{
    Line *array;
    int cnt;
    int cap;
} Array;

// Глобальные переменные для обработки сигнала
volatile sig_atomic_t timeout_occurred = 0;
volatile sig_atomic_t user_entered_something = 0;
char *file_data = NULL;    // Указатель на отображенный файл
size_t file_size = 0;      // Размер файла

// Обработчик сигнала ALARM - завершает программу
void alarm_handler(int sig) 
{
    if (!user_entered_something) {
        printf("\nTIME'S UP! 5 seconds have passed without input. Printing entire file...\n");
        
        // Выводим весь файл из отображенной памяти
        if (file_data != NULL) {
            fwrite(file_data, 1, file_size, stdout);
        }
        printf("\n");
        
        // Немедленно завершаем программу
        _exit(0);
    }
}

void initArray(Array *a) 
{
    a->array = malloc(sizeof(Line));
    a->cnt = 0;
    a->cap = 1;
}

void insertArray(Array *a, Line element) 
{
    if (a->cnt == a->cap) 
    {
        a->cap *= 2;
        a->array = realloc(a->array, a->cap * sizeof(Line));
    }
    a->array[a->cnt++] = element;
}

void freeArray(Array *a) 
{
    free(a->array);
    a->array = NULL;
    a->cnt = a->cap = 0;
}

// Функция для вывода таблицы строк
void printLineTable(Array *table) 
{
    printf("\n=== LINE TABLE ===\n");
    printf("Line # | Offset | Length | Preview\n");
    printf("-------+--------+--------+-------------------\n");
    
    for (int i = 0; i < table->cnt; i++) 
    {
        Line line = table->array[i];
        
        // Предпросмотр строки напрямую из отображенной памяти
        char preview[31];
        int preview_len = (line.length > 30) ? 30 : line.length;
        for (int j = 0; j < preview_len; j++) {
            char c = file_data[line.offset + j];
            if (c == '\n' || c == '\t' || c == '\r') {
                preview[j] = ' ';
            } else {
                preview[j] = c;
            }
        }
        preview[preview_len] = '\0';
        
        printf("%6d | %6ld | %6ld | %s", 
               i + 1, 
               (long)line.offset, 
               (long)line.length,
               preview);
        
        if (line.length > 30) {
            printf("...");
        }
        printf("\n");
    }
    printf("-------+--------+--------+-------------------\n");
}

// Построение таблицы строк с использованием mmap
Array buildLineTable(char *data, size_t size) 
{
    Array table;
    initArray(&table);
    
    off_t lineOffset = 0;
    off_t lineLength = 0;
    
    // Обрабатываем данные напрямую из памяти
    for (size_t i = 0; i < size; i++) 
    {
        if (data[i] == '\n') 
        {
            Line current = {lineOffset, lineLength};
            insertArray(&table, current);
            
            lineOffset = i + 1;
            lineLength = 0;
        } 
        else 
        {
            lineLength++;
        }
    }
    
    if (lineLength > 0) 
    {
        Line current = {lineOffset, lineLength};
        insertArray(&table, current);
    }
    
    return table;
}

// Вывод строки по номеру с использованием mmap
void printLine(Array *table, int lineNumber) 
{
    if (table->cnt < lineNumber) 
    {
        printf("The file contains only %d line(s).\n", table->cnt);
        return;
    }
    
    Line line = table->array[lineNumber - 1];
    
    // Выводим строку напрямую из отображенной памяти
    printf("Line %d: %.*s\n", lineNumber, (int)line.length, file_data + line.offset);
}

int main(int argc, char *argv[]) 
{
    if (argc != 2) 
    { 
        printf("Usage: %s <filename>\n", argv[0]);
        return 1; 
    }
    
    // Открываем файл
    int fd = open(argv[1], O_RDONLY);
    if (fd == -1) 
    { 
        perror("Failed to open file");
        return 1; 
    }
    
    // Получаем размер файла
    struct stat sb;
    if (fstat(fd, &sb) == -1) {
        perror("fstat failed");
        close(fd);
        return 1;
    }
    file_size = sb.st_size;
    
    // Отображаем файл в память
    file_data = mmap(NULL, file_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (file_data == MAP_FAILED) {
        perror("mmap failed");
        close(fd);
        return 1;
    }
    
    // Файл можно закрыть
    close(fd);
    
    // Настройка обработчика сигнала ALARM
    signal(SIGALRM, alarm_handler);
    
    // Построение таблицы строк из отображенной памяти
    Array table = buildLineTable(file_data, file_size);
    printf("Loaded %d lines from file.\n", table.cnt);
    
    // Выводим таблицу строк
    printLineTable(&table);
    
    printf("\nYou have 5 seconds to enter first line number!\n");
    printf("If you enter anything, timer stops completely.\n");
    printf("Otherwise program will print entire file and exit.\n");
    
    // Устанавливаем таймер на 5 секунд
    alarm(5);
    
    // Основной цикл
    while (1) 
    {
        int num;
        char input[100];
        printf("\nEnter the line number (0 to exit): ");
        
        if (fgets(input, sizeof(input), stdin) != NULL) 
        {
            // ЛЮБОЙ ввод останавливает таймер
            if (!user_entered_something) {
                user_entered_something = 1;
                alarm(0);  // Полностью останавливаем таймер
                printf("Timer stopped! You can take your time now.\n");
            }
            
            // Пробуем преобразовать в число
            if (sscanf(input, "%d", &num) == 1) 
            {
                if (num == 0) {
                    printf("Normal exit.\n");
                    break;
                }
                
                printLine(&table, num);
            } 
            else 
            {
                printf("Invalid input. Please enter a number.");
            }
        }
    }
    
    // Освобождаем ресурсы
    if (file_data != NULL && file_data != MAP_FAILED) {
        munmap(file_data, file_size);
    }
    freeArray(&table);
    
    return 0;
}

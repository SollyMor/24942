#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>

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
int fd_global;

// Обработчик сигнала ALARM - завершает программу
void alarm_handler(int sig) 
{
    if (!user_entered_something) {
        printf("\nTIME'S UP! 5 seconds have passed without input. Printing entire file...\n");
        
        // Выводим весь файл
        lseek(fd_global, 0, SEEK_SET);
        char buffer[1024];
        ssize_t bytes_read;
        
        while ((bytes_read = read(fd_global, buffer, sizeof(buffer))) > 0) 
        {
            write(STDOUT_FILENO, buffer, bytes_read);
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
void printLineTable(int fd, Array *table) 
{
    printf("\n=== LINE TABLE ===\n");
    printf("Line # | Offset | Length | Preview\n");
    printf("-------+--------+--------+-------------------\n");
    
    for (int i = 0; i < table->cnt; i++) 
    {
        Line line = table->array[i];
        
        off_t current_pos = lseek(fd, 0, SEEK_CUR);
        char *preview_buf = calloc(line.length + 1, sizeof(char));
        lseek(fd, line.offset, SEEK_SET);
        read(fd, preview_buf, line.length);
        
        char preview[31];
        int preview_len = (line.length > 30) ? 30 : line.length;
        for (int j = 0; j < preview_len; j++) {
            if (preview_buf[j] == '\n' || preview_buf[j] == '\t' || preview_buf[j] == '\r') {
                preview[j] = ' ';
            } else {
                preview[j] = preview_buf[j];
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
        
        free(preview_buf);
        lseek(fd, current_pos, SEEK_SET);
    }
    printf("-------+--------+--------+-------------------\n");
}

Array buildLineTable(int fd) 
{
    Array table;
    initArray(&table);
    
    char c;
    off_t lineOffset = 0;
    off_t lineLength = 0;
    
    while (read(fd, &c, 1) == 1)
    {
        if (c == '\n') 
        {
            Line current = {lineOffset, lineLength};
            insertArray(&table, current);
            
            lineOffset += lineLength + 1;
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

void printLine(int fd, Array *table, int lineNumber) 
{
    if (table->cnt < lineNumber) 
    {
        printf("The file contains only %d line(s).\n", table->cnt);
        return;
    }
    
    Line line = table->array[lineNumber - 1];
    char *buf = calloc(line.length + 1, sizeof(char));
    
    lseek(fd, line.offset, SEEK_SET);
    read(fd, buf, line.length);
    
    printf("Line %d: %s\n", lineNumber, buf);
    free(buf);
}

int main(int argc, char *argv[]) 
{
    if (argc != 2) 
    { 
        printf("Usage: %s <filename>\n", argv[0]);
        return 1; 
    }
    
    int fd = open(argv[1], O_RDONLY);
    if (fd == -1) 
    { 
        perror("Failed to open file");
        return 1; 
    }
    fd_global = fd;
    
    // Настройка обработчика сигнала ALARM
    signal(SIGALRM, alarm_handler);
    
    // Построение таблицы строк
    Array table = buildLineTable(fd);
    printf("Loaded %d lines from file.\n", table.cnt);
    
    // Выводим таблицу строк
    printLineTable(fd, &table);
    
    printf("\nYou have 5 seconds to enter first line number!\n");
    printf("If you enter at least one number, timer stops completely.\n");
    printf("Otherwise program will print entire file and exit.\n");
    
    // Устанавливаем таймер на 5 секунд
    alarm(5);
    
    while (1) 
{
    int num;
    char input[100];  // ← ДОБАВЬ ЭТУ СТРОЧКУ
    printf("\nEnter the line number (0 to exit): ");
    
    // ← ЗАМЕНИ ЭТУ ЧАСТЬ:
    if (fgets(input, sizeof(input), stdin) != NULL)  // ← Читаем всю строку
    {
        // ← ДОБАВЬ ЭТИ ДВЕ СТРОЧКИ ВНУТРИ if:
        user_entered_something = 1;  // ЛЮБОЙ ввод останавливает таймер
        alarm(0);                    // ОСТАНАВЛИВАЕМ ТАЙМЕР ПОЛНОСТЬЮ
        
        // Пробуем преобразовать в число
        if (sscanf(input, "%d", &num) == 1) 
        {
            if (num == 0) {
                printf("Normal exit.\n");
                break;
            }
            
            printLine(fd, &table, num);
        } 
        else 
        {
            printf("Invalid input. Please enter a number.");
        }
    }
}


    
    close(fd);
    freeArray(&table);
    return 0;
}

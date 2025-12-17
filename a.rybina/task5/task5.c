// Написать программу, которая анализирует текстовый файл, созданный текстовым редактором, таким как ed(1) или vi(1). После запроса, который предлагает ввести номер строки, с использованием printf(3) программа печатает соответствующую строку текста. Ввод нулевого номера завершает работу программы. Используйте open(2), read(2), lseek(2) и close(2) для ввода/вывода. Постройте таблицу отступов в файле и длин строк для каждой строки файла. Как только эта таблица построена, позиционируйтесь на начало заданной строки и прочтите точную длину строки. 
// Подсказка: Выберите или создайте текстовый файл с короткими строками. Помните, что первая строка начинается с нулевого отступа в файле. Найдите каждый символ перевода строки, запишите его позицию; в программе следует использовать вызов lseek(fd, 0L, 1). Для отладки распечатайте эту таблицу и сравните с таблицей, полученной вручную. Как только таблицы начнут совпадать, можно приступать к запросу номера строки.


#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    off_t offset;
    off_t length;
} Line;

typedef struct {
    Line* array;
    int cnt;
    int cap;
} Array;

void initArray(Array* a) {
    a->array = malloc(sizeof(Line));
    a->cnt = 0;
    a->cap = 1;
}

void insertArray(Array* a, Line element) {
    if (a->cnt == a->cap) {
        a->cap *= 2;
        a->array = realloc(a->array, a->cap * sizeof(Line));
    }

    a->array[a->cnt++] = element;
}

void freeArray(Array* a) {
    free(a->array);
    a->array = NULL;
    a->cnt = a->cap = 0;
}

int main(int argc, char* argv[]) {
    if (argc != 2) { 
        return 1; 
    }
    char* path = argv[1];

    Array table;
    initArray(&table);

    int fd = open(path, O_RDONLY);
    if (fd == -1) {
        return 1; 
    }
    // Get current position as mentioned in hint
    off_t current_pos = lseek(fd, 0L, SEEK_CUR);
    printf("Starting file analysis at position: %ld\n", current_pos);

    char c;
    off_t lineOffset = 0; //Offset of the line in the file
    off_t lineLength = 0; 
    while (read(fd, &c, 1) == 1) {
        if (c == '\n') {
            Line current = { lineOffset, lineLength };
            insertArray(&table, current);

            lineOffset += lineLength + 1;
            lineLength = 0;
        }
        else {
            lineLength++;
        }
    }

    if (lineLength > 0) {
        Line current = { lineOffset, lineLength };
        insertArray(&table, current);
    }

    // Print debugging table as mentioned in comments
    printf("\nLine Table (for debugging):\n");
    printf(" Line | Offset | Length\n");
    printf("------|--------|-------\n");
    for (int i = 0; i < table.cnt; i++) {
        printf("%5d | %6ld | %6ld\n", i + 1, table.array[i].offset, table.array[i].length);
    }
    printf("\nTotal lines: %d\n\n", table.cnt);

    while (1) {
        int num;
        char input[100];
        printf("Enter the line number: ");
        
        // Read input as string to validate
        if (fgets(input, sizeof(input), stdin) == NULL) {
            printf("Input error. Please try again.\n");
            continue;
        }
        
        // Remove newline character
        input[strcspn(input, "\n")] = '\0';
        
        // Validate input - only accept numbers
        int valid = 1;
        for (int i = 0; input[i] != '\0'; i++) {
            if (input[i] < '0' || input[i] > '9') {
                valid = 0;
                break;
            }
        }
        
        if (!valid) {
            printf("Invalid input. Please enter only numbers (0-9).\n");
            continue;
        }
        
        // Convert string to number
        num = atoi(input);

        if (num == 0) { break; }
        if (table.cnt < num) {
            printf("The file contains only %d line(s).\n", table.cnt);
            continue;
        }

        Line line = table.array[num - 1]; //Line
        char* buf = calloc(line.length + 1, sizeof(char)); //Buffer

        if (lseek(fd, line.offset, SEEK_SET) == -1) {
            perror("Error seeking in file");
            free(buf);
            continue;
        }
        
        if (read(fd, buf, line.length) == -1) {
            perror("Error reading line");
            free(buf);
            continue;
        }

        printf("Line %d: %s\n", num, buf);
        free(buf);
    }

    close(fd);
    freeArray(&table);

    return 0;
}
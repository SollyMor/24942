#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

typedef struct 
{
    off_t offset; // Позиция в файле где НАЧИНАЕТСЯ строка (в байтах от начала)
    off_t length; // Сколько байт ЗАНИМАЕТ строка (без символа \n)
} Line;

typedef struct 
{
    Line *array;
    int cnt;
    int cap;
} Array;

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
        a->cap *= 2 + 1;
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
        
        // Сохраняем текущую позицию в файле
        off_t current_pos = lseek(fd, 0, SEEK_CUR);
        
        // Читаем строку для предпросмотра
        char *preview_buf = calloc(line.length + 1, sizeof(char));
        lseek(fd, line.offset, SEEK_SET);
        read(fd, preview_buf, line.length);
        
        // Обрезаем предпросмотр до 30 символов
        char preview[31];
        int preview_len = (line.length > 30) ? 30 : line.length;
        for (int j = 0; j < preview_len; j++) {
            if (preview_buf[j] == '\n' || preview_buf[j] == '\t' || preview_buf[j] == '\r') {
                preview[j] = ' ';  // Заменяем служебные символы на пробелы
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
            printf("...");  // Показываем многоточие если строка обрезана
        }
        printf("\n");
        
        free(preview_buf);
        
        // Восстанавливаем позицию в файле
        lseek(fd, current_pos, SEEK_SET);
    }
    printf("-------+--------+--------+-------------------\n");
}

// Функция для построения таблицы строк
Array buildLineTable(int fd) 
{
    Array table;
    initArray(&table);
    
    char c; // Буфер для одного символа
    off_t lineOffset = 0; // Текущая позиция начала строки в файле
    off_t lineLength = 0; // Длина текущей строки
    
    while (read(fd, &c, 1) == 1) // read возвращает 1 если прочитал символ
    {
        if (c == '\n') // Найден символ конца строки
        {
            // Нашли конец строки - сохраняем информацию 
            Line current = {lineOffset, lineLength};
            insertArray(&table, current);
            
            // Следующая строка начинается после \n
            lineOffset += lineLength + 1; // +1 потому что пропускаем \n
            lineLength = 0; // Начинаем считать длину новой строки
        } 
        else 
        {
            lineLength++; // Увеличиваем длину текущей строки
        }
    }
    
    // Если последняя строка не заканчивается \n - сохраняем ее
    if (lineLength > 0) 
    {
        Line current = {lineOffset, lineLength};
        insertArray(&table, current);
    }
    
    return table;
}

// Функция для вывода строки по номеру
void printLine(int fd, Array *table, int lineNumber) 
{
    // Проверяем что запрошенная строка существует
    if (table->cnt < lineNumber) 
    {
        printf("The file contains only %d line(s).\n", table->cnt);
        return;
    }
    
    // Получаем информацию о строке (индексация с 0)
    Line line = table->array[lineNumber - 1];

     // Выделяем память под строку +1 для нулевого байта
    char *buf = calloc(line.length + 1, sizeof(char));
    // calloc заполняет память нулями, поэтому строка автоматически завершается \0

    lseek(fd, line.offset, SEEK_SET); // Перемещаем указатель чтения в нужную позицию
    read(fd, buf, line.length); // Читаем строку из файла в буфер
    
    printf("%s\n", buf); // Выводим строку
    free(buf); // Освобождаем память буфера
}

int main(int argc, char *argv[]) 
{
    if (argc != 2) 
    { 
        printf("Usage: %s <filename>\n", argv[0]);
        return 1; 
    }
    
    // Открываем файл ТОЛЬКО для чтения
    int fd = open(argv[1], O_RDONLY);
    if (fd == -1) 
    { 
        perror("Failed to open file");
        return 1; 
    }
    
    // Построение таблицы строк
    Array table = buildLineTable(fd);
    printf("Loaded %d lines from file.\n", table.cnt);
    
    // Выводим таблицу строк
    printLineTable(fd, &table);
    
    // Основной цикл
    while(1)  // Исправлено: было "while(true)"
    {
        int num;
        printf("\nEnter the line number (0 to exit): ");
        scanf("%d", &num); // Читаем номер строки
        
        if (num == 0) break; // Выход если ввели 0

        printLine(fd, &table, num); // Выводим запрошенную строку
    }
    
    close(fd);
    freeArray(&table);
    return 0;
}

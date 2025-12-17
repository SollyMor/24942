/*
    Программма для индексации и произвольного доступа к строкам файла
*/
#include <stdio.h>
#include <stdlib.h> //для exit
#include <unistd.h> //read, lseek, close
#include <fcntl.h>  //open, O_RDONLY
#include <sys/types.h>  
#include <sys/stat.h>
#include <errno.h>
#include <string.h> 
#include <ctype.h>


#define MAX_LINES 10000 //макс кол-во строк в файле
#define MAX_LINE_LENGTH 256 //макс длина одной строки

//структура для хранения информации о строках
typedef struct{
    long offset; //смещение(в байтах) начала строки в файле до начала новой строки 
    int length;  //длина строки(без символа новой строки)
} LineInfo;

static int read_int_from_stdin(long min, long max, long *out){
    char buf[64];

    if (!fgets(buf, sizeof(buf), stdin)){   //EOF (Ctrl+D) или ошибка
        return 0;                           // сообщим вызываещему - надо выйти 
    }

    //если строка длиннее нашего буфера - выбросим остаток до \n
    if (strchr(buf, '\n') == NULL){
        int c;
        while ((c = getchar()) != '\n' && c != EOF){}
    }

    //уберем пробелы по краям
    char *p = buf;
    while (isspace((unsigned char)*p)) p++;
    char *end = p + strlen(p);
    while (end > p && isspace((unsigned char)end[-1])) end--;
    *end = '\0';

    if (*p == '\0'){
        fprintf(stderr, "Пустой ввод. Повторите.\n");
        return -1;                                       //неверный ввод но stdin жив
    }

    errno = 0;
    char *q = NULL;
    long v = strtol(p, &q, 10);

    if (errno == ERANGE){
        fprintf(stderr, "Число вне диапозона long. Повторите.\n");
        return -1;
    }
    if (q == p || *q != '\0'){
        fprintf (stderr, "Ожидалось целое число. Повторите.\n");
        return -1;
    }

    if (v < min || v > max) {
        fprintf(stderr, "Число должно быть от %ld до %ld.\n", min, max);
        return -1;
    }

    *out = v;
    return 1;   //успех
}

int main(int argc, char *argv[]){
    int fd;
    char ch;
    LineInfo lines[MAX_LINES];
    int line_count = 0;
    long current_offset = 0;
    int line_length = 0;

    if (argc != 2){
        printf("Использование: %s <filename>\n", argv[0]);
        return 1;
    }
    fd = open(argv[1], O_RDONLY);
    if (fd == -1){
        perror("Ошибка открытия файла");
        return 1;
    }
    printf("Файл '%s' успешно открыт\n", argv[1]);

    // Построение таблицы строк
    printf("\n=== ПОСТРОЕНИЕ ТАБЛИЦЫ СТРОК ===\n");
    lines[0].offset = 0;

    ssize_t r;
    while ((r = read(fd, &ch, 1)) > 0){
        line_length++;
        if (ch == '\n'){
            lines[line_count].length = line_length - 1;
            printf("Строка %d: смещение = %ld, длина = %d\n",
                   line_count + 1, lines[line_count].offset, lines[line_count].length);
            line_count++;
            if (line_count >= MAX_LINES) {
                fprintf(stderr, "Достигнут лимит %d строк. Остальные игнорируются.\n", MAX_LINES);
                break;
            }
            current_offset = lseek(fd, 0L, SEEK_CUR);
            lines[line_count].offset = current_offset;
            line_length = 0;
        }
    }
    if (r < 0) {
        perror("Ошибка чтения файла");
        close(fd);
        return 1;
    }

    if (line_length > 0 && line_count < MAX_LINES){
        lines[line_count].length = line_length;
        printf("Строка %d: смещение = %ld, длина = %d\n",
               line_count + 1, lines[line_count].offset, lines[line_count].length);
        line_count++;
    }
    printf("\nВсего строк в файле: %d\n", line_count);

    // Интерактивный режим
    printf("\n=== Интерактивный режим ===\n");
    printf("Введите номер строки (1-%d) или 0 для выхода:\n", line_count);

    for (;;) {
        printf("> ");
        long ln;
        int rc = read_int_from_stdin(0, line_count, &ln);
        if (rc == 0) {                          // EOF
            printf("\nВыход (EOF)\n");
            break;
        }
        if (rc < 0) {                           // некорректный ввод
            continue;
        }
        if (ln == 0) {
            printf("Выход из программы\n");
            break;
        }

        int index = (int)ln - 1;
        long offset = lines[index].offset;
        int  length = lines[index].length;

        printf("Строка %ld: смещение = %ld, длина = %d\n", ln, offset, length);
        printf("Содержимое: ");

        if (lseek(fd, offset, SEEK_SET) == -1) {
            perror("Ошибка позиционирования");
            continue;
        }

        // печатаем безопасно: если строка длиннее нашего буфера — обрежем вывод
        char buffer[MAX_LINE_LENGTH];
        int to_read = length < (MAX_LINE_LENGTH - 1) ? length : (MAX_LINE_LENGTH - 1);
        ssize_t bytes_read = read(fd, buffer, to_read);
        if (bytes_read < 0) {
            perror("Ошибка чтения строки");
            continue;
        }
        buffer[bytes_read] = '\0';
        printf("'%s'%s\n", buffer, (length > to_read) ? " ...[обрезано]" : "");
    }

    close(fd);
    return 0;
}
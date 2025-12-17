#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <termios.h>
#include <stdlib.h>
#include <ctype.h>


#define MAX_LINE_LEN 40 // максимальная длина строки
#define MAX_MAX_LEN 10 * MAX_LINE_LEN
#define ERASE  0x7F // удалить символ
#define CTRL_U 0x15 // очистка строки
#define CTRL_W 0x17 // очистка слова
#define CTRL_D 0x04 // завершение
#define CTRL_G 0x07 // звук ??
#define CTRL_A 0x01 // + полная отчистка

#define UP     "\033[A"
#define DOWN   "\033[B"
#define RIGHT  "\033[C"
#define LEFT   "\033[D"
#define SOUND  "\a"
#define DELETE "\b \b"


char buf[MAX_MAX_LEN + 1];
// строка на которой пользователь
int curr_row = 0;
// виртуальный курсор
size_t cursor = 0;

struct termios original_termios;

void restore_terminal() {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &original_termios);
}

void setup_terminal() {

    struct termios new_termios;
    
    if (tcgetattr(STDIN_FILENO, &original_termios) == -1) {
        perror("tcgetattr");
        exit(1);
    }
    
    // восстанавливаем настройки терминала после заверешения работы программы
    atexit(restore_terminal);
    
    new_termios = original_termios;
    // отключение канонического режима (символы передаются программе немедленно - без встроенной буферизации)
    // отключение эхо (символы не отображаются (программа сама управляет выводом))
    new_termios.c_lflag &= ~(ICANON | ECHO);
    // минимальное количество символов для чтения (в нашем случае 1)
    new_termios.c_cc[VMIN] = 1;
    // таймаут чтения в десятых долях секунды (в нашем случае 0 - бесконечное ожидание)
    new_termios.c_cc[VTIME] = 0;
    
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &new_termios) == -1) {
        perror("tcsetattr");
        exit(1);
    }
}



int main() {

    setup_terminal();

    printf("┌────────────────────────────────────────┐\n");
    printf("│           Строчный редактор            │\n");
    printf("├──────────┬─────────────────────────────┤\n");
    printf("│ ERASE    │ Стирает последний символ    │\n");
    printf("│ CTRL+U   │ Стирает текущую строку      │\n");
    printf("│ CTRL+W   │ Стирает последнее слово     │\n");
    printf("│ CTRL+A   │ Стирает все строки          │\n");
    printf("│ CTRL+D   │ Завершение (начало строки)  │\n");
    printf("├──────────┴─────────────────────────────┤\n");
    printf("│ Макс. длина строки: 40 символов        │\n");
    printf("└────────────────────────────────────────┘\n");
    printf("Ввод:\n");
    
    char c;
    ssize_t n;

    while (1) {

        // посимвольно будем считывать
        n = read(STDIN_FILENO, &c, 1);

        // завершение работы CTRL^D
        if (c == CTRL_D) {
            // если курсор в начале буфера
            if (cursor == 0) {
                printf("\n┌────────────────────────────────────────┐\n");
                  printf("│   Строчный редактор завершил работу    │\n");
                  printf("└────────────────────────────────────────┘\n");
                break;
            }
        }
        // удаление символа с поддержкой межстрочного удаления
        else if (c == ERASE) {

            // курсор в начале буфера — нечего удалять
            if (cursor == 0) continue;


            int was_at_line_start = (cursor % MAX_LINE_LEN == 0);
            
            // Всегда уменьшаем курсор и удаляем символ из буфера
            cursor--;
            
            if (was_at_line_start && curr_row > 0) {
                // Если были в начале строки (кроме первой) - переходим на строку выше
                curr_row--;
                
                // Перемещаем курсор в конец предыдущей строки
                write(1, UP, 3); // переходим на строку вверх
                for (int i = 0; i < MAX_LINE_LEN; i++) {
                    write(1, RIGHT, 3); // перемещаемся в конец строки
                }
                // Удаляем последний символ предыдущей строки
                write(1, DELETE, 3);
            } 
            else {
                // Обычное удаление символа
                write(1, DELETE, 3);
            }
        }
        // если символ ПОЛНОГО стирания CTRL^A
        else if (c == CTRL_A) {

            // если есть символы в строках
            if (cursor > 0) {

                // проходимся по всем строкам
                for (int i = 0; i <= curr_row; i++) {
                    // вернемся в начало текущей строки
                    write(1, "\r", 1);
                    for (int j = 0; j != MAX_LINE_LEN; j++) {
                        write(1, " ", 1);
                    }
                    write(1, "\r", 1);

                    if (i < curr_row) write(1, UP, 3);
                }
                write(1, "\r", 1);

                curr_row = 0;
                cursor = 0;
            }
        }

        // для удаления всех символов в текущей строки CTRL^U
        else if (c == CTRL_U) {

            if (cursor == 0) continue;
            
            int current_line_start = (cursor / MAX_LINE_LEN) * MAX_LINE_LEN;
            int chars_to_delete    = cursor - current_line_start;
            
            if (chars_to_delete > 0) {
                // удаляем символы на экране
                for (int i = 0; i < chars_to_delete; i++) {
                    write(1, DELETE, 3);
                }
                // Очищаем буфер
                for (int i = current_line_start; i < cursor; i++) {
                    buf[i] = '\0';
                }
                
                cursor = current_line_start;
                
                // Корректируем curr_row если нужно
                if (current_line_start > 0 && chars_to_delete == MAX_LINE_LEN) {
                    curr_row--;
                }
            }
        }

        // удаляет последнее слово с последующими пробелами
        else if (c == CTRL_W) {

            int fl = 0; // 0 - ищем слово, 1 - встретили пробел после слова
            
            while (cursor > 0) {
                cursor--;


                // если пробел
                if (isspace(buf[cursor])) {
                    // если до этого не встречали пробел
                    if (fl == 0) {
                        // встретили пробел после слова - это начало пробелов между словами
                        fl = 1;
                    }
                }
                // иначе это символ
                else {
                    // встретили не пробел (причем встречали пробел до этого)
                    if (fl == 1) {
                        // это начало нового слова - останавливаемся
                        cursor++; // возвращаемся к пробелу
                        break;
                    }
                    // продолжаем идти по текущему слову
                }
                
                // удаляем символ на экране
                if (cursor % MAX_LINE_LEN == MAX_LINE_LEN - 1 && curr_row > 0) {
                    // если удалили символ перевода строки
                    curr_row--;
                    write(1, UP, 3);
                    write(1, "\r", 1);
                    for (int i = 0; i < MAX_LINE_LEN; i++) {
                        write(1, RIGHT, 3);
                    }
                    write(1, DELETE, 3);
                }
                else {
                    write(1, DELETE, 3);
                }
            }
        }
        // иначе просто печатаем символы с переносом
        else if (c >= 32 && c <= 126) {

            if (cursor < MAX_MAX_LEN) {

                buf[cursor++] = c;

                write(1, &c, 1);
                
                if (cursor % MAX_LINE_LEN == 0) {
                    
                    write(1, "\n", 1);
                    curr_row++;
                }
            }
            else write(1, SOUND, 1);
        }
        else write(1, SOUND, 1);
    
    }
    return 0;
}

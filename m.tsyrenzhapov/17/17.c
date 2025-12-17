#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#define MAX_LINE 40
#define ERASE 0x7F
#define KILL 0x15
#define CTRL_W 0x17
#define CTRL_D 0x04

struct termios orig_attrs;

void restore_terminal(void) {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_attrs);
}

void setup_terminal(void) {
    struct termios attrs;
    tcgetattr(STDIN_FILENO, &orig_attrs);
    atexit(restore_terminal);
    
    attrs = orig_attrs;
    attrs.c_lflag &= ~(ICANON | ECHO); // неканонический вид
    attrs.c_cc[VMIN] = 1;
    attrs.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &attrs);
}

int main(void) {
    char line[1024] = {0};
    int len = 0;
    char c;
    
    setup_terminal();
    printf("Line editor (Ctrl+D to exit): ");
    fflush(stdout);

    while (read(STDIN_FILENO, &c, 1) == 1) {
        // Выход по Ctrl+D только в начале строки
        if (c == CTRL_D && len == 0) {
            printf("\n");
            break;
        }
        
        // Обработка управляющих символов
        if (iscntrl(c) || !isprint(c)) {
            switch (c) {
                case ERASE: // Backspace
                    if (len > 0) {
                        len--;
                        line[len] = '\0';
                        printf("\33[D\33[K");
                    } else {
                        printf("\a");
                    }
                    break;
                    
                case KILL: // Очистка строки
                    if (len > 0) {
                        len = 0;
                        line[0] = '\0';
                        printf("\33[2K\rLine editor (Ctrl+D to exit): ");
                    } else {
                        printf("\a");
                    }
                    break;
                    
                case CTRL_W: // Удаление слова
                    if (len > 0) {
                        int word_start = 0;
                        char prev = ' ';
                        for (int i = 0; i < len; i++) {
                            if (line[i] != ' ' && prev == ' ')
                                word_start = i;
                            prev = line[i];
                        }
                        len = word_start;
                        line[len] = '\0';
                        printf("\33[%dD\33[K", len - word_start);
                    } else {
                        printf("\a");
                    }
                    break;
                    
                default:
                    printf("\a");
                    break;
            }
        } 
        else {
            //обычный символ - проверяем перенос
            line[len++] = c;
            line[len] = '\0';
            
            //если достигли лимита и символ не пробел - делаем перенос
            if (len == MAX_LINE && c != ' ') {
                // Ищем начало текущего слова
                int word_start = len - 1;
                while (word_start >= 0 && line[word_start] != ' ') {
                    word_start--;
                }
                word_start++; //переходим к первому символу слова
                
                //если слово начинается до границы - переносим
                if (word_start < MAX_LINE) {
                    printf("\33[%dD\33[K", len - word_start);
                    printf("\n");
                    
                    //копируем слово в начало буфера
                    int word_len = len - word_start;
                    memmove(line, line + word_start, word_len);
                    len = word_len;
                    line[len] = '\0';
                    
                    //выводим перенесенное слово
                    for (int i = 0; i < len; i++) {
                        putchar(line[i]);
                    }
                }
            } else {
                //обычный вывод
                putchar(c);
            }
        }
        
        fflush(stdout);
    }
    
    return 0;
}
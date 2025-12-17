#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <string.h>
#include <ctype.h>

#define MAX_BUFFER 1024
#define MAX_LINE   40

#define ERASE   0x7F   // удалить символ
#define CTRL_U  0x15   // очистка строки
#define CTRL_W  0x17   // очистка слова
#define CTRL_D  0x04   // завершение
#define CTRL_G  0x07   // звуковой сигнал

#define UP     "\033[A"
#define DOWN   "\033[B"
#define RIGHT  "\033[C"
#define LEFT   "\033[D"
#define DELETE "\b \b"

const char bell = '\a';
static struct termios orig_termios;

void disable_raw_mode() {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

void enable_raw_mode() {
    struct termios raw;
    tcgetattr(STDIN_FILENO, &orig_termios);
    raw = orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON);
    raw.c_cc[VMIN]  = 1;
    raw.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
    atexit(disable_raw_mode);
}

void erase_last_word(char *buf, int *len, int *len_temp, char *word_buf, int *word_len) {
    if (*len <= 0 || *len_temp <= 0) {
        *word_len = 0;
        return;
    }

    // Пропускаем пробелы
    while (*len > 0 && *len_temp > 0 && buf[*len - 1] == ' ') {
        write(STDOUT_FILENO, DELETE, strlen(DELETE));
        (*len)--;
        (*len_temp)--;
    }

    *word_len = 0;

    // Сохраняем слово в обратном порядке
    int temp_len = 0;
    char temp_word[MAX_LINE + 1];

    while (*len > 0 && *len_temp > 0 && buf[*len - 1] != ' ') {
        temp_word[temp_len++] = buf[*len - 1]; // сохраняем символ
        write(STDOUT_FILENO, DELETE, strlen(DELETE));
        (*len)--;
        (*len_temp)--;
    }

    // Копируем в правильном порядке
    for (int i = 0; i < temp_len; i++) {
        word_buf[i] = temp_word[temp_len - 1 - i];
    }
    *word_len = temp_len;
}

int main(void) {
    enable_raw_mode();
    char buffer[MAX_BUFFER];
    int len = 0;
    char ch;
    int len_temp = 0;
    int flag_word = 0;
    while (read(STDIN_FILENO, &ch, 1) == 1) {

        if (ch == CTRL_D) {                     // CTRL-D
            if (len_temp == 0 && len == 0) {
                write(STDOUT_FILENO, "\n", 1);
                break;
            } else {
                write(STDOUT_FILENO, &bell, 1);
                continue;
            }
        }

        else if (ch == ERASE) {                 // Backspace
            if (len == 0) continue;
            len--;
            len_temp--;
            if (len_temp + 1 == 0) {
                write(STDOUT_FILENO, UP, strlen(UP));    
                for (int i = 0; i < MAX_LINE; i++)
                    write(STDOUT_FILENO, RIGHT, strlen(RIGHT));
                write(STDOUT_FILENO, DELETE, strlen(DELETE));
                len_temp = MAX_LINE;
            } else {
                write(STDOUT_FILENO, DELETE, strlen(DELETE));
            }
        }

        else if (ch == CTRL_U) {
            if (len_temp == 0 || len == 0) continue;
            
            for (int i = 0; i < len_temp+1; i++) {
                write(STDOUT_FILENO, DELETE, strlen(DELETE));
            }
            for (size_t i = len-len_temp; i < len; i++) buffer[i] = '\0';
            
            len -= len_temp;
            len_temp = 0;
        }

        else if (ch == CTRL_W) { // CTRL-W
            if (len == 1){
                continue;
            }
            
            if (len_temp == 0) {
                write(STDOUT_FILENO, UP, strlen(UP));    
                for (int i = 0; i < MAX_LINE; i++)
                    write(STDOUT_FILENO, RIGHT, strlen(RIGHT));
                len_temp = MAX_LINE;
            }
            char last_word[MAX_LINE + 1] = {0};
            int word_len = 0;
            erase_last_word(buffer, &len, &len_temp, last_word, &word_len);
        }

        else if (ch >= 32 && ch <= 126) {
            if (len < MAX_BUFFER) {
                buffer[len++] = ch;
                len_temp++;
                write(STDOUT_FILENO, &ch, 1);
                if (len_temp == MAX_LINE && !isspace(ch)) {
                    char word[MAX_LINE + 1];
                    int word_len = 0;
                    erase_last_word(buffer, &len, &len_temp, word, &word_len);
                    write(STDOUT_FILENO, "\n", 1);
                    len += word_len + 1; 

                    len_temp = 0;
                    // Выводим слово на новой строке
                    for (int i = 0; i < word_len; i++) {
                        write(STDOUT_FILENO, &word[i], 1);

                        buffer[len++] = word[i]; // сохраняем в buffer
                        len_temp++;
                    }
                }
                else if (len_temp == MAX_LINE){
                    write(STDOUT_FILENO, "\n", 1);
                    len++;
                    len_temp = 0;
                }
                
            } else {
                write(STDOUT_FILENO, &bell, 1);
            }
        }  

        else {
            write(STDOUT_FILENO, &bell, 1);
        }
    }

    return 0;
}

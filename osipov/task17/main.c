#include <unistd.h>
#include <sys/termios.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>

#define LINE_LENGTH 40
struct termios t;

void disableRawMode() {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &t);  //set old terminal settings
}

void enableRawMode() {
    tcgetattr(STDIN_FILENO, &t);     //remember default termios
    atexit(disableRawMode);                     //call on exit

    struct termios raw = t;
    raw.c_lflag &= ~(ECHO | ICANON);            //disable echo and canon mode
    raw.c_cc[VMIN]  = 1;
    raw.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);   //set new terminal settings
}

int main() {
    enableRawMode();

    char c;
    static char line[LINE_LENGTH + 1];

     while (read(STDIN_FILENO, &c, 1) == 1) {
        int len = strlen(line);
        if (iscntrl(c) || !isprint(c)) {

            if (c == t.c_cc[VERASE]) {
                // ERASE - стереть последний симовол в строке.

                line[len - 1] = 0;

                // [D - двигает курсор на знак влево
                // [K - чистит строку справа от курсора
                // printf("\33[D\33[K");
                
                printf("\b \b");

            } else if (c == t.c_cc[VKILL]) {
                // KILL - стереть все символы в строке.
                line[0] = 0;

                // [2K - чистит строку целиком
                printf("\33[2K\r");

            } else if (c == t.c_cc[VWERASE]) {
                // CTRL-W - стереть последнее слово в строке

                int word_start = 0;
                char prev = ' ';
                for (int i = 0; i < len; i++) {
                    if (line[i] != ' ' && prev == ' ') {
                        word_start = i;
                    }
                    prev = line[i];
                }
                line[word_start] = 0;

                // [<n>D - двигает курсор на n знаков влево
                // [K - чистит строку справа от курсора
                printf("\33[%dD\33[K", len - word_start);

            } else if (c == t.c_cc[VEOF]) {
                // CTRL-D - завершение программы, если
                // курсор находится в начале строки

                if (line[0] == 0) { exit(0); }

            } else {
                // Все остальные непечатаемые символы должны
                // издавать звуковой сигнал (CTRL-G)
                putchar('\a');
            }

        } else {

            if (len == LINE_LENGTH) {
                
                int i = len - 1;
                while (i >= 0 && line[i] != ' ') i--;
                int word_start = i + 1;

                if (word_start > 0 && word_start < len) {
                    int move = len - word_start;
                    printf("\33[%dD\33[K", move);

                    putchar('\n');
                    printf("%s", line + word_start);

                    memmove(line, line + word_start, move + 1);
                    len = move;

                } else {
                    putchar('\n');
                    line[0] = 0;
                    len = 0;
                }
            }

            line[len++] = c;
            line[len] = 0;

            putchar(c);
        }

        fflush(NULL);
    }

    return 0;
}
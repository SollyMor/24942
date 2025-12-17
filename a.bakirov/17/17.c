#include <unistd.h>
#include <sys/termios.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>

#define LINE_LENGTH 40

#define CERASE  0177
#define CKILL   CTRL('u')
#define CWERASE CTRL('w')

struct termios orig_termios;

void disableRawMode() {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

void enableRawMode() {
    tcgetattr(STDIN_FILENO, &orig_termios);
    atexit(disableRawMode);

    struct termios raw = orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON);

    raw.c_cc[VMIN]  = 1;
    raw.c_cc[VTIME] = 0;
    
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

int main() {
    enableRawMode();

    char c;
    static char line[LINE_LENGTH + 1];

    while (read(STDIN_FILENO, &c, 1) == 1) {
        int len = strlen(line);

        if (iscntrl((unsigned char)c) || !isprint((unsigned char)c)) {
            switch ((unsigned char)c) {
                case CERASE: {
                    if (len > 0) {
                        line[len - 1] = 0;
                        printf("\33[D\33[K");
                    }
                    break;
                }

                case CKILL: {
                    line[0] = 0;
                    printf("\33[2K\r");
                    break;
                }

                case CWERASE: {
                    int word_start = 0;
                    char prev = ' ';
                    for (int i = 0; i < len; i++) {
                        if (line[i] != ' ' && prev == ' ')
                            word_start = i;
                        prev = line[i];
                    }

                    line[word_start] = 0;

                    int n = len - word_start;
                    printf("\33[%dD\33[K", n);

                    break;
                }

                case CEOF: {
                    if (line[0] == 0) exit(0);
                    break;
                }

                default: {
                    putchar('\a');
                    break;
                }
            }
        } else {
            if (len == LINE_LENGTH) {
                int word_start = 0;
                char prev = ' ';

                for (int i = 0; i < len; i++) {
                    if (line[i] != ' ' && prev == ' ')
                        word_start = i;
                    prev = line[i];
                }

                int word_len = len - word_start;

                if (word_start == 0) {
                    putchar('\n');
                    line[0] = 0;
                    len = 0;
                } else {
                    char word_buf[LINE_LENGTH + 1];
                    memcpy(word_buf, &line[word_start], word_len);
                    word_buf[word_len] = '\0';

                    line[word_start] = '\0';

                    int n = word_len;
                    printf("\33[%dD\33[K", n);

                    putchar('\n');

                    memcpy(line, word_buf, word_len + 1);
                    len = word_len;

                    fputs(line, stdout);
                    fflush(stdout);
                }
            }

            line[len++] = c;
            line[len] = '\0';

            putchar(c);
            fflush(stdout);
        }
    }

    return 0;
}

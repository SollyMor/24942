#include <unistd.h>
#include <sys/termios.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>

#define LINE_LENGTH 40

struct termios orig_termios;

void disableRawMode(){
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

int main() {

    tcgetattr(STDIN_FILENO, &orig_termios);
    atexit(disableRawMode);

    struct termios raw = orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON);
    
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;
    
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);


    printf("Terminal line editor started.\n");
    printf("Controls: Backspace - erase char, Ctrl+U - kill line, Ctrl+W - kill word, Enter - new line, Ctrl+D - exit if line empty\n");
    printf("Line length limit: %d characters. Enter text:\n", LINE_LENGTH);
    fflush(stdout);

    char c;
    static char line[LINE_LENGTH + 1];
    int len = 0;
    
    while (read(STDIN_FILENO, &c, 1) == 1) {
        if (iscntrl(c) || !isprint(c)) {
            switch (c) {
            case '\b':
            case 127: {
                if (len > 0) {
                    line[len - 1] = '\0';
                    len--;
                    printf("\33[D\33[K");
                }
                break;
            }

            case '\025': {
                line[0] = '\0';
                len = 0;
                printf("\33[2K\r");
                break;
            }

            case '\027': {
                if (len > 0) {
                    int word_start = 0;
                    char prev = ' ';
                    for (int i = 0; i < len; i++) {
                        if (line[i] != ' ' && prev == ' ') {
                            word_start = i;
                        }
                        prev = line[i];
                    }

                    int chars_to_erase = len - word_start;
                    line[word_start] = '\0';
                    len = word_start;

                    printf("\33[%dD\33[K", chars_to_erase);
                }
                break;
            }

            case '\n': {
                putchar('\n');
                len = 0;
                line[0] = '\0';
                break;
            }

            case '\004': {
                if (len == 0) { 
                    exit(0); 
                }
                break;
            }

            default: {
                putchar('\a');
                break;
            }
            }
        }
        else {
            if (len == LINE_LENGTH) {
                int chars_to_erase = 0;
                static char buf[LINE_LENGTH + 1];
                if (len > 0) {
                    int word_start = 0;
                    char prev = ' ';
                    for (int i = 0; i < len; i++) {
                        if (line[i] != ' ' && prev == ' ') {
                            word_start = i;
                        }
                        prev = line[i];
                    }
                    chars_to_erase = len - word_start;
                    for (int i = 0 ; i < chars_to_erase; i++){
                        buf[i] = line[word_start+i];
                    }
                    line[word_start] = '\0';
                    len = word_start;

                    printf("\33[%dD\33[K", chars_to_erase);
                }
                putchar('\n');
                if (chars_to_erase > 0)
                    for (int i = 0; i < chars_to_erase; i++){
                        putchar(buf[i]);
                    }
                len = 0;
                line[0] = '\0';
            }

            if (len < LINE_LENGTH) {
                line[len++] = c;
                line[len] = '\0';
                putchar(c);
            }
        }

        fflush(stdout);
    }

    return 0;
}

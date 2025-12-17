// Многие программы, принимающие ввод с терминала, позволяют редактировать строку перед использованием. Напишите программу, которая выключает эхо и каноническую обработку, таким образом выключив и обработку символа забоя. Ваша программа должна получать ввод с клавиатуры и показывать его на терминале в соответствии со следующими правилами:
//      Каждый введенный символ должен немедленно появляться на дисплее.
//      Когда вводится символ ERASE, стирается последний символ в текущей строке.
//      Когда вводится символ KILL, стираются все символы в текущей строке.
//      Когда вводится CTRL-W, стирается последнее слово в текущей строке, вместе со всеми следующими за ним пробелами.
//      Программа завершается, когда введен CTRL-D и курсор находится в начале строки.
//      Все непечатаемые символы, кроме перечисленных выше, должны издавать звуковой сигнал, выводя на терминал символ CTRL-G.
//      Длина строки ограничена 40 символами. Если какое-то слово пересекает 40-й столбец, это слово должно быть помещено в начало следующей строки.

#include <unistd.h>
#include <sys/termios.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>

#define LINE_LENGTH 40

// Define control characters for Solaris compatibility
#ifndef CERASE
#define CERASE '\b'    // Backspace
#endif
#ifndef CKILL
#define CKILL '\025'   // Ctrl+U (kill line)
#endif
#ifndef CWERASE
#define CWERASE '\027'  // Ctrl+W (kill word)
#endif
#ifndef CEOF
#define CEOF '\004'    // Ctrl+D (EOF)
#endif
struct termios orig_termios;

void disableRawMode() {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios); // Terminal control set attribute
}

void enableRawMode() {
    tcgetattr(STDIN_FILENO, &orig_termios);
    atexit(disableRawMode); // At exit do this

    struct termios raw = orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON);
    
    // Set VMIN and VTIME for 1-byte buffering
    raw.c_cc[VMIN] = 1;   // Read at least 1 character
    raw.c_cc[VTIME] = 0;  // No timeout
    
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

int main() {
    enableRawMode();

    printf("Terminal line editor started.\n");
    printf("Controls: Backspace (erase char), Ctrl+U (kill line), Ctrl+W (kill word), Enter (new line), Ctrl+D (exit if line empty)\n");
    printf("Line length limit: %d characters. Enter text:\n", LINE_LENGTH);
    fflush(stdout);

    char c;
    static char line[LINE_LENGTH + 1];
    int len = 0;  // Track current line length
    
    while (read(STDIN_FILENO, &c, 1) == 1) {
        if (iscntrl(c) || !isprint(c)) { // Control key or is printable
            switch (c) {
            case CERASE: { // backspace
                if (len > 0) {
                    line[len - 1] = '\0';
                    len--;
                    printf("\33[D\33[K");  // Move left and clear to end of line
                }
                break;
            }

            case CKILL: { // ctrl+U
                line[0] = '\0';
                len = 0;
                printf("\33[2K\r");  // Clear entire line and move to beginning
                break;
            }

            case CWERASE: { // ctrl+W
                if (len > 0) {
                    // Find start of last word
                    int word_start = 0;
                    char prev = ' ';
                    for (int i = 0; i < len; i++) {
                        if (line[i] != ' ' && prev == ' ') {
                            word_start = i;
                        }
                        prev = line[i];
                    }

                    // Erase from word_start to end
                    int chars_to_erase = len - word_start;
                    line[word_start] = '\0';
                    len = word_start;

                    // Move cursor back and clear
                    printf("\33[%dD\33[K", chars_to_erase);
                }
                break;
            }

            case '\n':
            case '\r': { // Enter key
                putchar('\n');
                len = 0;
                line[0] = '\0';
                break;
            }

            case CEOF: { // ctrl+D
                if (len == 0) { 
                    exit(0); 
                }
                break;
            }

            default: {
                putchar('\a');  // Bell character for unhandled control chars
                break;
            }
            }
        }
        else {
            // Printable character
            if (len == LINE_LENGTH) {
                putchar('\n');
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


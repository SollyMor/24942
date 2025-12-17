#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>

#define MAX_LINE_LENGTH 40
#define MAX_TEXT_LENGTH 2000
#define BELL '\007'
#define ERASE 0x7F
#define KILL 0x15
#define CTRL_W 0x17
#define CTRL_D 0x04

struct termios original_termios;

void restore_terminal(void) {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &original_termios);
}

void setup_terminal(void) {
    struct termios new_termios;
    tcgetattr(STDIN_FILENO, &original_termios);
    atexit(restore_terminal);
    new_termios = original_termios;
    new_termios.c_lflag &= ~(ICANON | ECHO);
    new_termios.c_cc[VMIN] = 1;
    new_termios.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &new_termios);
}

struct editor {
    char text[MAX_TEXT_LENGTH];
    int pos;
    int len;
};

void clear_screen() {
    printf("\033[s\033[1;1H\033[2KInput text (CTRL-D at line start to exit):");
    printf("\033[2;1H\033[J");
}

void print_wrapped_text(const char *text, int len) {
    if (len <= 0) return;

    int col = 0;
    int i = 0;

    while (i < len) {
        if (text[i] == ' ' || text[i] == '\t') {
            if (col >= MAX_LINE_LENGTH) {
                printf("\n");
                col = 0;
            }
            putchar(text[i]);
            col++;
            i++;
        } else {
            int word_start = i;
            int word_end = i;
            while (word_end < len && text[word_end] != ' ' && text[word_end] != '\t') {
                word_end++;
            }
            int word_length = word_end - word_start;

            if (word_length > MAX_LINE_LENGTH) {
                int chars_to_print = MAX_LINE_LENGTH - col;
                if (chars_to_print <= 0) {
                    printf("\n");
                    col = 0;
                    chars_to_print = MAX_LINE_LENGTH;
                }

                for (int j = 0; j < chars_to_print && i < len && text[i] != ' ' && text[i] != '\t'; j++) {
                    putchar(text[i]);
                    i++;
                    col++;
                }

                if (col >= MAX_LINE_LENGTH) {
                    printf("\n");
                    col = 0;
                }
            } else if (col + word_length > MAX_LINE_LENGTH) {
                printf("\n");
                col = 0;

                for (int j = word_start; j < word_end; j++) {
                    putchar(text[j]);
                }
                col += word_length;
                i = word_end;
            } else {
                for (int j = word_start; j < word_end; j++) {
                    putchar(text[j]);
                }
                col += word_length;
                i = word_end;
            }
        }
    }
}

void calculate_cursor_position(const char *text, int length, int pos, int *out_line, int *out_col) {
    int line = 2;
    int col = 0;
    int i = 0;

    while (i < pos) {
        if (text[i] == ' ' || text[i] == '\t') {
            if (col >= MAX_LINE_LENGTH) {
                line++;
                col = 0;
            }
            col++;
            i++;
        } else {
            int word_start = i;
            int word_end = i;
            while (word_end < pos && text[word_end] != ' ' && text[word_end] != '\t') {
                word_end++;
            }
            int word_length = word_end - word_start;

            if (word_length > MAX_LINE_LENGTH) {
                int remaining_in_line = MAX_LINE_LENGTH - col;
                if (remaining_in_line <= 0) {
                    line++;
                    col = 0;
                    remaining_in_line = MAX_LINE_LENGTH;
                }

                int chars_to_process = (pos - i < remaining_in_line) ? pos - i : remaining_in_line;
                col += chars_to_process;
                i += chars_to_process;

                if (col >= MAX_LINE_LENGTH) {
                    line++;
                    col = 0;
                }
            } else if (col + word_length > MAX_LINE_LENGTH) {
                line++;
                col = word_length;
                i = word_end;
            } else {
                col += word_length;
                i = word_end;
            }
        }
    }

    *out_line = line;
    *out_col = col;
}

void redraw(struct editor *e) {
    clear_screen();

    if (e->len > 0) {
        print_wrapped_text(e->text, e->len);
    }

    int line, col;
    calculate_cursor_position(e->text, e->len, e->pos, &line, &col);

    printf("\033[%d;%dH", line, col + 1);
    fflush(stdout);
}


void erase_word(struct editor *e) {
    if (e->pos == 0) {
        putchar(BELL);
        fflush(stdout);
        return;
    }
    
    int end = e->pos;
    while (end > 0 && (e->text[end - 1] == ' ' || e->text[end - 1] == '\t')) end--;
    while (end > 0 && e->text[end - 1] != ' ' && e->text[end - 1] != '\t') end--;
    
    int n = e->pos - end;
    if (n > 0) {
        memmove(e->text + end, e->text + e->pos, e->len - e->pos + 1);
        e->len -= n;
        e->pos = end;
        redraw(e);
    }
}

int main(void) {
    struct editor e = { .pos = 0, .len = 0 };
    char c;
    
    setup_terminal();
    printf("\033[2J\033[HInput text (CTRL-D at line start to exit):\n");
    fflush(stdout);
    
    while (read(STDIN_FILENO, &c, 1) == 1) {
        if (c == CTRL_D && e.pos == 0) {
            printf("\nExit.\n");
            break;
        }
        
        if (c == ERASE) {
            if (e.pos > 0) {
                e.pos--;
                e.len--;
                memmove(e.text + e.pos, e.text + e.pos + 1, e.len - e.pos + 1);
                redraw(&e);
            } else {
                putchar(BELL);
                fflush(stdout);
            }
            continue;
        }
        
        if (c == KILL) {
            if (e.pos > 0) {
                memmove(e.text, e.text + e.pos, e.len - e.pos + 1);
                e.len -= e.pos;
                e.pos = 0;
                redraw(&e);
            } else {
                putchar(BELL);
                fflush(stdout);
            }
            continue;
        }
        
        if (c == CTRL_W) {
            erase_word(&e);
            continue;
        }
        
        if (c < 32 || c > 126) {
            putchar(BELL);
            fflush(stdout);
            continue;
        }
        
        if (e.len >= MAX_TEXT_LENGTH - 1) {
            putchar(BELL);
            fflush(stdout);
            continue;
        }
        
        if (e.pos < e.len) {
            memmove(e.text + e.pos + 1, e.text + e.pos, e.len - e.pos);
        }
        e.text[e.pos] = c;
        e.pos++;
        e.len++;
        e.text[e.len] = '\0';
        redraw(&e);
    }
    
    return 0;
}
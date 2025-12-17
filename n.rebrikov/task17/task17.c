#include <unistd.h>
#include <sys/termios.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>


#define MY_CERASE   0x7F  // Backspace (обычно Ctrl+H или Backspace)
#define MY_CKILL    0x15  // Ctrl+U (удалить всю строку)
#define MY_CWERASE  0x17  // Ctrl+W (удалить слово)
#define MY_CEOF     0x04  // Ctrl+D (завершить программу)


#define LINE_LENGTH 40
struct termios orig_termios;

void disableRawMode() 
{
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

void enableRawMode() 
{
    // 1. Сохраняем оригинальные настройки терминала
    tcgetattr(STDIN_FILENO, &orig_termios);

    // 2. Регистрируем функцию восстановления при выходе
    atexit(disableRawMode);

    // 3. Создаем копию настроек для модификации
    struct termios raw = orig_termios;

    // 4. Отключаем флаги:
    raw.c_lflag &= ~(ECHO | ICANON);
    /*
    ECHO - отображать вводимые символы на экране
    ICANON - построчный (канонический) режим ввода
    */
    raw.c_cc[VMIN] = 1; // Ж�дать минимум 1 символ
    raw.c_cc[VTIME] = 0; // �,tp nfqvfenf

    // 5. Применяем новые настройки
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
    //TCSAFLUSH - ждет пока все выходные данные отправятся, сбрасывает входную очередь
}

int main() 
{
    enableRawMode();

    unsigned char c;
    static char line[LINE_LENGTH + 1];

    while (read(STDIN_FILENO, &c, 1) == 1) 
    {
        // Теперь мы получаем каждый символ сразу
        // И сами решаем что с ним делать
        int len = strlen(line);
        if (iscntrl(c) || !isprint(c)) 
        {
            switch (c) 
            {
                case MY_CERASE: 
                {
                    // Когда вводится символ ERASE, стирается
                    // последний символ в текущей строке.

                    line[len - 1] = 0;

                    // [D - Move cursor left one char
                    // [K - Clear line from cursor right
                    printf("\33[D\33[K");

                    break;
                }

                case MY_CKILL: 
                {
                    // Когда вводится символ KILL, стираются
                    // все символы в текущей строке.

                    line[0] = 0;

                    // [2K - Clear entire line
                    printf("\33[2K\r");

                    break;
                }

                case MY_CWERASE: 
                {
                    // Когда вводится CTRL+W, стирается последнее слово в текущей
                    // строке, вместе со всеми следующими за ним пробелами.

                    int word_start = 0;
                    char prev = ' ';
		    int i;
                    for(i = 0; i < len; i++) 
			{
                        if (line[i] != ' ' && prev == ' ') 
                        {
                            word_start = i;
                        }
                        prev = line[i];
                    }

                    line[word_start] = 0;

                    // [<n>D - Move cursor left n chars
                    // [K - Clear line from cursor right
                    printf("\33[%dD\33[K", len - word_start);

                    break;
                }

                case MY_CEOF: 
                {
                    // Программа завершается, когда введен CTRL+D
                    // и курсор находится в начале строки.

                    if (line[0] == 0) { exit(0); }
                    break;
                }

                default: 
                {
                    // Если символ управляющий или непечатаемый
                    putchar('\a'); // Звуковой сигналs
                    break;
                }
            }
        } 
        else 
        {
            if (len == LINE_LENGTH) 
            {
                putchar('\n');
                len = 0;
            }

            line[len++] = c;
            line[len] = 0;

            putchar(c);
        }

        fflush(NULL);
    }

    return 0;
}

#include <unistd.h>
#include <termios.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>

#define LINE_LENGTH 40

static struct termios orig_termios;

/* Восстановить исходные настройки терминала */
void disableRawMode(void)
{
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

/* Включить "сырой" режим (без эхо, без каноники) */
void enableRawMode(void)
{
    if (tcgetattr(STDIN_FILENO, &orig_termios) == -1)
    {
        perror("Ошибка при получении настроек терминала (tcgetattr)");
        exit(1);
    }

    atexit(disableRawMode);

    struct termios raw = orig_termios;

    // Выключаем эхо и канонический режим
    raw.c_lflag &= ~(ECHO | ICANON);
    // Оставляем ISIG, чтобы Ctrl+C и Ctrl+Z работали
    // raw.c_lflag &= ~ISIG; // если наоборот надо отключить сигналы

    // Настраиваем поведение read() в неканоническом режиме:
    // ждать минимум 1 байт, без таймаута
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1)
    {
        perror("Ошибка при применении настроек терминала (tcsetattr)");
        exit(1);
    }
}

int main(void)
{
    enableRawMode();

    // Управляющие символы, как они были в исходном каноническом режиме
    unsigned char erase_char = orig_termios.c_cc[VERASE]; // обычно Backspace или DEL
    unsigned char kill_char = orig_termios.c_cc[VKILL];   // обычно ^U
#ifdef VWERASE
    unsigned char werase_char = orig_termios.c_cc[VWERASE]; // обычно ^W
#else
    unsigned char werase_char = 23; // ^W
#endif
    unsigned char eof_char = orig_termios.c_cc[VEOF]; // обычно ^D

    char c;
    static char line[LINE_LENGTH + 1] = {0};

    while (read(STDIN_FILENO, &c, 1) == 1)
    {
        int len = (int)strlen(line);

        if (iscntrl((unsigned char)c) || !isprint((unsigned char)c))
        {
            // Управляющие / непечатные символы

            if ((unsigned char)c == erase_char)
            {
                // ERASE — стереть последний символ текущей строки
                if (len > 0)
                {
                    line[len - 1] = '\0';
                    // Сдвиг курсора влево на 1 и очистка справа
                    printf("\33[D\33[K");
                }
                else
                {
                    // Нечего стирать — издаём звуковой сигнал
                    putchar('\a');
                }
            }
            else if ((unsigned char)c == kill_char)
            {
                // KILL — стереть всю текущую строку
                if (len > 0)
                {
                    line[0] = '\0';
                    // Очистить всю строку и перейти в её начало
                    printf("\33[2K\r");
                }
                else
                {
                    putchar('\a');
                }
            }
            else if ((unsigned char)c == werase_char)
            {
                // CTRL-W — стереть последнее слово и следующие пробелы
                if (len > 0)
                {
                    int i = len - 1;

                    // Сначала убираем пробелы в конце
                    while (i >= 0 && line[i] == ' ')
                    {
                        i--;
                    }
                    // Потом идём назад до начала слова
                    while (i >= 0 && line[i] != ' ')
                    {
                        i--;
                    }

                    int new_len = i + 1;       // позиция после пробела
                    int delta = len - new_len; // сколько символов убираем

                    line[new_len] = '\0';

                    if (delta > 0)
                    {
                        // Сдвигаем курсор влево на delta и очищаем справа
                        printf("\33[%dD\33[K", delta);
                    }
                }
                else
                {
                    putchar('\a');
                }
            }
            else if ((unsigned char)c == eof_char)
            {
                // CTRL-D — завершить, если строка пустая
                if (len == 0)
                {
                    putchar('\n');
                    fflush(stdout);
                    exit(0);
                }
                else
                {
                    // Если есть текст — игнорируем как обычный непечатный
                    putchar('\a');
                }
            }
            else if (c == '\r' || c == '\n')
            {
                // Enter — просто новая строка, сброс текущей логической строки
                putchar('\n');
                line[0] = '\0';
            }
            else
            {
                // Остальные непечатные символы — звуковой сигнал
                putchar('\a');
            }
        }
        else
        {
            // Печатаемый символ

            if (len < LINE_LENGTH)
            {
                // Строка ещё не заполнена
                line[len] = c;
                line[len + 1] = '\0';
                putchar(c);
            }
            else
            {
                // len == LINE_LENGTH, добавление этого символа пересекает 40-й столбец

                if (c == ' ')
                {
                    // Если это пробел — переносим на новую строку
                    putchar('\n');
                    line[0] = '\0';
                }
                else
                {
                    // Переносим последнее слово целиком на следующую строку

                    // Ищем начало последнего слова
                    int word_start = len - 1; // последний символ текущей строки
                    while (word_start > 0 && line[word_start - 1] != ' ')
                    {
                        word_start--;
                    }

                    int word_len = len - word_start; // длина последнего слова

                    // Случай, когда слово занимает всю строку
                    if (word_start == 0 && word_len >= LINE_LENGTH)
                    {
                        // Переносим по символам
                        putchar('\n');
                        line[0] = c;
                        line[1] = '\0';
                        putchar(c);
                    }
                    else
                    {
                        // Убираем слово с текущей строки визуально
                        printf("\33[%dD\33[K", word_len);

                        // Печатаем перевод строки
                        putchar('\n');

                        // Новое содержимое строки:
                        // последнее слово + только что введённый символ
                        char new_line[LINE_LENGTH + 1];
                        int i;
                        for (i = 0; i < word_len; ++i)
                        {
                            new_line[i] = line[word_start + i];
                        }
                        new_line[i++] = c;
                        new_line[i] = '\0';

                        // Копируем в основной буфер
                        strcpy(line, new_line);
                        len = i;

                        // Печатаем новую строку
                        fputs(line, stdout);
                    }
                }
            }
        }

        fflush(stdout);
    }

    return 0;
}

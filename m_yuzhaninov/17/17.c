#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <string.h>
#include <ctype.h>

#define MAX_LINE 40

int main() 
{
    // Стуктуры для записи настроек терминала
    struct termios old_tio, new_tio;
    // Переменная для строки
    char line[MAX_LINE+1] = {0}; 
    // Позиция курсора
    int pos = 0;        
    int escape_state=0;
    int rus_symb=0;

    // Получаем текущие настройки терминала
    if (tcgetattr(STDIN_FILENO, &old_tio) == -1) 
    {
        perror("tcgetattr");
        return 1;
    }

    // Создаем новые настройки
    new_tio = old_tio;
    // Отключаем эхо и каноническую обработку
    new_tio.c_lflag &= ~(ICANON | ECHO);
    // Делаем так, чтобы read возращал каждый символ сразу после нажатия
    new_tio.c_cc[VMIN] = 1;
    new_tio.c_cc[VTIME] = 0;

    // Применяем к терминалу новые настройки
    if (tcsetattr(STDIN_FILENO, TCSANOW, &new_tio) == -1)
    {
        perror("tcsetattr");
        return 1;
    }

    printf("Ctrl-D в начале строки — выход\n");

    // Редактируем строку
    while (1) 
    {
        // Читаем по одному символу
        char c;
        if (read(STDIN_FILENO, &c, 1) != 1)
        {
            tcsetattr(STDIN_FILENO, TCSANOW, &old_tio);
            perror("read");
            return 1;
        }

        // Обработка ESC
        if (escape_state == 0 && c == 0x1b) 
        {
            printf("\x07");
            printf("\nВведен некорректный символ\n");
            fflush(stdout);
            escape_state = 2; 
            pos = 0;
            continue;
        } 
        else if (escape_state > 0) 
        {
            escape_state--;
            continue;
        }

        // Обработка русских букв
        if (rus_symb == 0 && (unsigned char)c & 0x80) 
        {
            printf("\x07");
            printf("\nВведен некорректный символ\n");
            fflush(stdout);
            rus_symb = 1;
            pos = 0;
            continue;
        } 
        else if (rus_symb == 1)
        {
            rus_symb = 0;
            continue;
        }

        // Ставим курсор на прошлый символ
        int i = pos - 1;

        // Определяем что за символ поступил
        switch (c) 
        {
            // Ctrl-D: выход, только если курсор в начале строки
            case 4:
                // Если курсор в начале, то мы возвращаем старые настройки и завершаем
                if (pos == 0) 
                {
                    tcsetattr(STDIN_FILENO, TCSANOW, &old_tio);
                    printf("\nВыход.\n");
                    fflush(stdout);
                    return 0;
                } 
                // Иначе звуковой сигнал
                else 
                {
                    printf("\x07");
                    fflush(stdout);
                    break;
                }

            // Ctrl-W: удалить последнее слово + последующие пробелы
            case 23: 
                // Идем назад и пропускаем пробелы в конце, если они есть
                while (i >= 0 && line[i] == ' ') 
                {
                    i--;
                }
                // Идем назад и пропускаем последнее слово
                while (i >= 0 && line[i] != ' ') 
                {
                    i--;
                }
                // Сделан один лишний -1, нужно вернуть
                i++;

                // Стираем символы на экране
                for (int j = i; j < pos; j++) 
                {
                    printf("\b \b");
                    fflush(stdout);
                    line[j] = '\0';
                }

                // Удаляем затертые символы из буфера
                pos = i;
                line[pos] = '\0';
                break;

            // ERASE (Backspace/Delete): удалить последний символ
            // Backspace
            case 8:
            // Delete
            case 127:
                // Если есть символы, то стираем последний
                if (pos > 0) 
                {
                    printf("\b \b"); 
                    fflush(stdout);
                    pos--;
                    line[pos] = '\0';
                } 
                // Иначе звуковой сигнал
                else 
                {
                    printf("\x07"); 
                    fflush(stdout); 
                }
                break;

            // KILL (Ctrl-U): очистить всю строку
            case 21:
                // Затираем все символы
                while (pos > 0) 
                {
                    printf("\b \b");
                    fflush(stdout);
                    pos--;
                    line[pos] = '\0';
                }
                break;
    

            // Печатаемые символы
            default:
                // Если символ непечатаемый, то звуковой сигнал
                if (!isprint(c)) 
                {
                    printf("\x07");
                    fflush(stdout); 
                } 
                else 
                {
                    if (pos == MAX_LINE) 
                    {
                        int word_start = pos;
                        // Находим начало текущего слова
                        while (word_start > 0 && line[word_start - 1] != ' ') 
                        {
                            word_start--;
                        }

                        // Стираем слово с экрана
                        for (int i = word_start; i < pos; i++) 
                        {
                            printf("\b \b");
                            fflush(stdout);
                        }

                        // Сохраняем слово во временный буфер
                        char temp[MAX_LINE] = {0};
                        int len = 0;
                        for (int i = word_start; i < pos; i++) 
                        {
                            temp[len++] = line[i];
                        }
                        temp[len] = '\0';

                        // Очищаем часть строки от word_start до конца
                        for (int i = word_start; i <= pos; i++) 
                        {
                            line[i] = '\0';
                        }
                        pos = word_start;  // Обновляем позицию курсора

                        // Переходим на новую строку
                        printf("\n");
                        fflush(stdout);

                        // Выводим сохранённое слово в новой строке
                        printf("%s", temp);
                        fflush(stdout);

                        // Обновляем line и pos для новой строки
                        strcpy(line, temp);
                        pos = len;
                    }

                    else
                    {
                        // Добавляем символ в буфер и отображаем
                        line[pos++] = c;
                        line[pos] = '\0';
                        printf("%c", c);
                        fflush(stdout);
                    }
                }
                break;
        }
    }
}
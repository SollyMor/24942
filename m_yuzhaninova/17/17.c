#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <string.h>
#include <ctype.h>

#define MAX_LINE_LENGTH 40

int main() 
{
    struct termios old_termios, new_termios;
    char buffer[MAX_LINE_LENGTH + 1] = {0}; 
    int cursor_position = 0;        

    if (tcgetattr(STDIN_FILENO, &old_termios) == -1) 
    {
        perror("tcgetattr");
        return 1;
    }

    new_termios = old_termios;
    new_termios.c_lflag &= ~(ICANON | ECHO);
    new_termios.c_cc[VMIN] = 1;
    new_termios.c_cc[VTIME] = 0;

    if (tcsetattr(STDIN_FILENO, TCSANOW, &new_termios) == -1)
    {
        perror("tcsetattr");
        return 1;
    }

    printf("Ctrl-D в начале строки завершит программу\n");

    while (1) 
    {
        char symb;
        if (read(STDIN_FILENO, &symb, 1) != 1)
        {
            tcsetattr(STDIN_FILENO, TCSANOW, &old_termios);
            perror("read");
            return 1;
        }

        // Игнорируем непечатаемые символы кроме управляющих
        if (!isprint((unsigned char)symb) && 
            symb != 4 && symb != 23 && symb != 21 && 
            symb != 8 && symb != 127) 
        {
            printf("\x07");
            fflush(stdout);
            continue;
        }

        int search_position = cursor_position - 1;

        switch (symb) 
        {
            case 4: // Ctrl-D
                if (cursor_position == 0) 
                {
                    tcsetattr(STDIN_FILENO, TCSANOW, &old_termios);
                    printf("\nВыход.\n");
                    return 0;
                } 
                else 
                {
                    printf("\x07");
                    fflush(stdout);
                }
                break;

            case 23: // Ctrl-W
                if (cursor_position == 0) 
                {
                    printf("\x07");
                    fflush(stdout);
                    break;
                }
                
                // Удаляем пробелы и последнее слово
                while (search_position >= 0 && buffer[search_position] == ' ') 
                {
                    search_position--;
                }
                while (search_position >= 0 && buffer[search_position] != ' ') 
                {
                    search_position--;
                }
                search_position++;

                // Стираем с экрана
                for (int char_index = search_position; char_index < cursor_position; char_index++) 
                {
                    printf("\b \b");
                }
                fflush(stdout);

                cursor_position = search_position;
                buffer[cursor_position] = '\0';
                break;

            case 8:  // Backspace
            case 127: // Delete
                if (cursor_position > 0) 
                {
                    printf("\b \b");
                    fflush(stdout);
                    cursor_position--;
                    buffer[cursor_position] = '\0';
                } 
                else 
                {
                    printf("\x07");
                    fflush(stdout);
                }
                break;

            case 21: // Ctrl-U
                if (cursor_position == 0) 
                {
                    printf("\x07");
                    fflush(stdout);
                    break;
                }
                
                while (cursor_position > 0) 
                {
                    printf("\b \b");
                    cursor_position--;
                    buffer[cursor_position] = '\0';
                }
                fflush(stdout);
                break;
    
            default: // Обычные символы
                if (cursor_position == MAX_LINE_LENGTH) 
                {
                    int word_start_position = cursor_position;
                    // Ищем начало текущего слова
                    while (word_start_position > 0 && buffer[word_start_position - 1] != ' ') 
                    {
                        word_start_position--;
                    }

                    if (word_start_position == 0) 
                    {
                        printf("\x07");
                        fflush(stdout);
                        break;
                    }

                    // Сохраняем слово для переноса
                    char word_buffer[MAX_LINE_LENGTH] = {0};
                    int word_length = 0;
                    for (int char_index = word_start_position; char_index < cursor_position; char_index++) 
                    {
                        word_buffer[word_length++] = buffer[char_index];
                        printf("\b \b");
                    }
                    fflush(stdout);

                    // Очищаем строку и переносим слово
                    for (int char_index = word_start_position; char_index <= cursor_position; char_index++) 
                    {
                        buffer[char_index] = '\0';
                    }
                    cursor_position = word_start_position;

                    printf("\n%s", word_buffer);
                    fflush(stdout);

                    strcpy(buffer, word_buffer);
                    cursor_position = word_length;
                }

                buffer[cursor_position++] = symb;
                buffer[cursor_position] = '\0';
                printf("%c", symb);
                fflush(stdout);
                break;
        }
    }
}
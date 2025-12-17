#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <sys/wait.h>

int main(void) 
{
    // Массив для файловых дескрипторов канала
    int pipefd[2];  
    // Переменная для хранения id процесса
    pid_t pid;
    char buffer[1024];
    long n;

    // Создаем канал
    if (pipe(pipefd) == -1) 
    {
        perror("pipe");
        return 1;
    }

    // Создаем дочерний процесс
    pid = fork();
    if (pid == -1) 
    {
        perror("fork");
        return 1;
    }

    // Если процесс дочерний
    if (pid == 0) 
    {
        // Дочерний процесс только читает, поэтому закрываем дескриптор для записи
        close(pipefd[1]);  

        // Читаем данные из канала
        while ((n = read(pipefd[0], buffer, sizeof(buffer))) > 0) 
        {
            // Преобразуем в верхний регистр
            for (long i = 0; i < n; i++) 
            {
                buffer[i] = toupper((unsigned char)buffer[i]);
            }
            // Выводим результат
            write(STDOUT_FILENO, buffer, n);
        }
        // Закрываем канал полностью
        close(pipefd[0]);
        return 0;

    } 
    else 
    {
        // Родительский процесс только пишет, закрываем дескриптор для чтения
        close(pipefd[0]);  

        printf("Введите текст (для завершения нажмите Ctrl+D):\n");

        // Получаем ввод
        while (fgets(buffer, sizeof(buffer), stdin) != NULL) 
        {
            n = strlen(buffer);
            int flag=0;
            for (long i = 0; i < n; i++)
            {
                if (!isprint(buffer[i]) && buffer[i] != '\n' && buffer[i] != ' ')
                {
                    flag = 1;
                }
            }

            if (flag == 1)
            {
                printf("Введены некорректные символы\n");
                continue;
            }
            // Записываем данные в канал
            if (write(pipefd[1], buffer, n) == -1) 
            {
                perror("write to pipe");
                break;
            }
        }

        // Закрываем канал полностью
        close(pipefd[1]);
        // После ввода данных ждем, пока выполнится дочерний процесс 
        wait(NULL);
    }

    return 0;
}

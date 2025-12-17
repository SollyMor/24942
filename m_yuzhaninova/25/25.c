#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <sys/wait.h>

int main(void) 
{
    int fd[2];  

    // Открытие канала
    if (pipe(fd) == -1) 
    {
        perror("pipe");
        return 1;
    }

    // Созданием подпроцесс
    pid_t p = fork();
    if (p == -1) 
    {
        perror("fork");
        return 1;
    }

    char buffer[500];
    // Процесс потомок читает из pipe и выводит ответ
    if (p == 0) 
    {
        close(fd[1]);  

        long bytes_read;
        while ((bytes_read = read(fd[0], buffer, sizeof(buffer))) > 0) 
        {
            for (long i = 0; i < bytes_read; i++)
            {
                buffer[i] = toupper((unsigned char)buffer[i]);
            }
            write(STDOUT_FILENO, buffer, bytes_read);
        }
        close(fd[0]);
        return 0;

    } 
    // Процесс родитель читает ввод и пишет в pipe
    else 
    {
        close(fd[0]);  

        printf("Введите текст. Чтобы завершить нажмите Ctrl-D\n");

        long bytes_read;
        while ((bytes_read = read(STDIN_FILENO, buffer, sizeof(buffer))) > 0) 
        {
            if (write(fd[1], buffer, bytes_read) == -1) 
            {
                perror("write");
                break;
            }
        }

        close(fd[1]);
        wait(NULL);
    }

    return 0;
}
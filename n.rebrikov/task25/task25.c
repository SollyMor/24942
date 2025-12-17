#include <unistd.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>

int main() 
{
    int fildes[2];
    // fildes[0] - файловый дескриптор для чтения
    // fildes[1] - файловый дескриптор для записи
    if (pipe(fildes) == -1) // создает однонаправленный канал
    {
        perror("pipe failed");
        return 1;
    }

    pid_t pid = fork();
    if (pid == -1) 
    {
        perror("fork failed");
        close(fildes[0]);
        close(fildes[1]);
        return 1;
    }

    if (pid == 0) 
    {
        // Дочерний процесс - читает из канала и переводит в верхний регистр
        close(fildes[1]);  // Закрываем неиспользуемый конец для записи

        char c;
        ssize_t bytes_read;
        while ((bytes_read = read(fildes[0], &c, 1)) > 0) 
        {
            printf("%c", toupper((unsigned char)c));
        }
        
        if (bytes_read == -1) 
        {
            perror("read failed");
        }
        
        close(fildes[0]);
        exit(0);
    } 
    else 
    {
        // Родительский процесс - пишет в канал
        close(fildes[0]);  // Закрываем неиспользуемый конец для чтения

        char *text = "Hello, World!\n";
        ssize_t bytes_written = write(fildes[1], text, strlen(text));
        
        if (bytes_written == -1) 
        {
            perror("write failed");
        }
        
        close(fildes[1]);  // Закрытие канала отправляет EOF читателю
        
        // Ждем завершения дочернего процесса
        wait(NULL);
        printf("Parent: Child process finished.\n");
    }

    return 0;
}

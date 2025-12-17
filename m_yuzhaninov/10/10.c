#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(int argc, char *argv[])
{
    pid_t pid;
    int status;
    
    // Проверяем наличие аргументов
    if (argc == 1) 
    {
        fprintf(stderr, "Input error\n");
        return 1;
    }
    
    // Создаем подпроцесс
    // Дальше процессы выполняются параллельно
    pid = fork();
    
    // Ошибка создания подпроцесса
    if (pid == -1) 
    {
        perror("fork failed");
        return 1;
    }
    
    if (pid == 0) 
    {
        // Заменяем подпроцесс заданной командой
        execvp(argv[1], argv + 1);
        // Если команда не сработала, то ошибка
        perror("execlp failed");
        return 1;
    } 
    else 
    {
        // Ждем завершения подпроцесса, чтобы выполнить родительский процесс
        if (waitpid(pid, &status, 0) == -1) 
        {
            perror("waitpid failed");
            return 1;
        }
        
        printf("\nЭтот текст выводится после завершения заданной команды\n");
        
        // Проверяем статус, чтобы узнать как завершился подпроцесс
        if (WIFEXITED(status)) 
        {
            printf("Подпроцесс завершился с кодом: %d\n", WEXITSTATUS(status));
        } 
        else if (WIFSIGNALED(status)) 
        {
            printf("Подпроцесс завершен сигналом: %d\n", WTERMSIG(status));
        }
    }
    
    return 0;
}

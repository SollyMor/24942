#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(int argc, char *argv[])
{
    // Проверяем наличие аргументов
    if (argc < 2) 
    {
        fprintf(stderr, "Input error\n");
        return 1;
    }

    pid_t p;
    p = fork();
    
    // Ошибка создания подпроцесса
    if (p == -1) 
    {
        perror("fork");
        return 1;
    }
    
    if (p == 0) 
    {
        execvp(argv[1], argv + 1);
        perror("execvp");
        return 1;
    } 
    else 
    {
        int status;
        if (waitpid(p, &status, 0) == -1) 
        {
            perror("waitpid");
            return 1;
        }

        // Проверка кода завершения
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

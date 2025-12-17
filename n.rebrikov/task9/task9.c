#include <unistd.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>

int main() 
{
    pid_t pid = fork();
    
    if (pid == -1) 
    {
        perror("fork failed");
        return 1;
    }

    if (pid == 0) 
    {
        // Дочерний процесс
        printf("Child process (PID: %d) executing cat...\n", getpid());
        execlp("cat", "cat", "../task1/task1.c", NULL);
        
        // Если execlp вернулся - значит ошибка
        perror("execlp failed");
        exit(1);
    } 
    else 
    {
        // Родительский процесс
        printf("Parent process (PID: %d) created child with PID: %d\n", getpid(), pid);
        printf("Parent is doing some work...\n");
        
        // Ждем завершения дочернего процесса
        int status;
        pid_t waited_pid = wait(&status);
        
        if (waited_pid == -1) 
        {
            perror("wait failed");
        } 
        else 
        {
            if (WIFEXITED(status)) 
            {
                printf("\nChild process (PID: %d) finished with exit code: %d\n", 
                       waited_pid, WEXITSTATUS(status));
            } 
            else if (WIFSIGNALED(status)) 
            {
                printf("\nChild process (PID: %d) killed by signal: %d\n", 
                       waited_pid, WTERMSIG(status));
            }
        }
    }

    return 0;
}

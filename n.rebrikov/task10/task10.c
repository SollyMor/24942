#include <unistd.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) 
{
    if (argc < 2) 
    {
        fprintf(stderr, "Usage: %s <command> [args...]\n", argv[0]);
        return 1;
    }

    pid_t pid = fork();
    if (pid == -1) 
    {
        perror("fork failed");
        return 1;
    }

    if (pid == 0) 
    {
        // Дочерний процесс
        execvp(argv[1], argv + 1);
        
        // Если дошли сюда - execvp не сработал
        perror("execvp failed");
        exit(127);  // Стандартный код для "command not found"
    } 
    else 
    {
        // Родительский процесс
        int stat_loc;
        pid_t waited_pid = wait(&stat_loc);

        if (waited_pid == -1) 
        {
            perror("wait failed");
            return 1;
        }

        printf("\nChild process (PID: %d) ", waited_pid);
        
        if (WIFEXITED(stat_loc)) 
        {
            printf("finished with exit code: %d\n", WEXITSTATUS(stat_loc));
        } 
        else if (WIFSIGNALED(stat_loc)) 
        {
            printf("was killed by signal: %d\n", WTERMSIG(stat_loc));
        } 
        else if (WIFSTOPPED(stat_loc)) 
        {
            printf("was stopped by signal: %d\n", WSTOPSIG(stat_loc));
        } 
        else 
        {
            printf("terminated abnormally\n");
        }
    }

    return 0;
}

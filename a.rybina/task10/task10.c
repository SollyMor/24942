// Напишите программу, которая запускает команду, заданную в качестве первого аргумента, в виде порожденного процесса. Все остальные аргументы программы передаются этой команде. Затем программа должна дождаться завершения порожденного процесса и распечатать его код завершения.

//./task10 cat ./test.txt 
//./task10 ls -la

#include <stdio.h>
#include <stdlib.h> 
#include <unistd.h>
#include <sys/wait.h>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <command> [arguments]\n", argv[0]);
        printf("Example: %s ls -la\n", argv[0]);
        printf("Example: %s cat test.txt\n", argv[0]);
        return 1;
    }

    char *command = argv[1];
    int status;
    pid_t pid = fork();
    
    if (pid == -1) {
        perror("fork failed");
        return 1;
    }
    else if (pid == 0) {
        printf("Child process (PID: %d) executing: %s\n", getpid(), command);
        execvp(command, &argv[1]); //execute vector path
        perror("execvp failed");
        exit(1);
    }
    else {
        printf("\nParent process (PID: %d) created child (PID: %d)\n", getpid(), pid);
        printf("Parent waiting for child to complete...\n");
        
        if (waitpid(pid, &status, 0) == -1) {
            perror("waitpid failed");
            return 1;
        }

        if (WIFEXITED(status)) {
            printf("\nChild process completed with exit code: %d\n", WEXITSTATUS(status));
        } else if (WIFSIGNALED(status)) {
            printf("\nChild process was killed by signal: %d\n", WTERMSIG(status));
        } else {
            printf("\n  Child process terminated abnormally\n");
        }
    }
    
    return 0;
}
// Напишите программу, которая создает подпроцесс. Этот подпроцесс должен исполнить cat(1) длинного файла. Родитель должен вызвать printf(3) и распечатать какой-либо текст. После выполнения первой части задания модифицируйте программу так, чтобы последняя строка, распечатанная родителем, выводилась после завершения порожденного процесса. Используйте wait(2), waitid(2) или waitpid(3).

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <filename>\n", argv[0]);
        printf("Example: %s test.txt\n", argv[0]);
        return 1;
    }
    
    char *filename = argv[1];
    pid_t pid = fork();
    int status;
    
    if (pid == -1) {
        perror("fork failed");
        return 1;
    }
    else if (pid == 0) {
        // Child process
        printf("Child process (PID: %d) executing cat on file: %s\n", getpid(), filename);
        execlp("cat", "cat", filename, NULL);
        perror("execlp failed");
        exit(1);
    }
    else {
        // Parent process
        printf("Parent process (PID: %d) created child (PID: %d)\n", getpid(), pid);

        if (waitpid(pid, &status, 0) == -1) {
            perror("waitpid failed");
            return 1;
        }
        
        if (WIFEXITED(status)) { //wait if exited
            printf("\nChild process completed successfully with exit code: %d\n", WEXITSTATUS(status));
        } else {
            printf("\nChild process terminated abnormally\n");
        }
        
        // This line prints after child process completes
        printf("\n=== PARENT'S FINAL MESSAGE: Child process has finished! ===\n");
    }
    
    return 0;
}
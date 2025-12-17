#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>

int main() {
    pid_t pid;
    int status;
    const char *filename = "/etc/passwd";  // Используем длинный системный файл
    
    printf("Родительский процесс: PID = %d\n", getpid());
    printf("Родительский процесс: создаю подпроцесс для выполнения cat\n");
    
    pid = fork();
    
    if (pid < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        printf("Дочерний процесс: PID = %d\n", getpid());
        sleep(1);
        printf("Дочерний процесс: выполняю cat для файла %s\n", filename);
        
        execlp("cat", "cat", filename, (char *)NULL);
        
        perror("execlp");
        exit(EXIT_FAILURE);
    } else {
        printf("Родительский процесс: дочерний процесс создан с PID = %d\n", pid);
        sleep(1);
        printf("Родительский процесс: печатаю текст во время выполнения дочернего процесса\n");
        printf("Родительский процесс: ожидаю завершения дочернего процесса...\n");
        
        waitpid(pid, &status, 0);
        
        printf("Родительский процесс: дочерний процесс завершился\n");
    }
    
    return EXIT_SUCCESS;
}


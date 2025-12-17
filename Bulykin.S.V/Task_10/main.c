#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>

int main(int argc, char *argv[]) {
    pid_t pid;
    int status;
    
    if (argc < 2) {
        fprintf(stderr, "Использование: %s <команда> [аргументы...]\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    
    printf("Родительский процесс: PID = %d\n", getpid());
    printf("Родительский процесс: запускаю команду '%s'\n", argv[1]);
    
    pid = fork();
    
    if (pid < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        printf("Дочерний процесс: PID = %d\n", getpid());
        printf("Дочерний процесс: выполняю команду '%s'\n", argv[1]);
        
        execvp(argv[1], &argv[1]);
        
        perror("execvp");
        exit(EXIT_FAILURE);
    } else {
        printf("Родительский процесс: дочерний процесс создан с PID = %d\n", pid);
        printf("Родительский процесс: ожидаю завершения дочернего процесса...\n");
        
        waitpid(pid, &status, 0);
        printf("Родительский процесс: Дочерний процесс завершился\n");
        
        
    }
    
    return EXIT_SUCCESS;
}


#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>

int main() {
    pid_t pid = fork();
    
    if (pid == -1) {
        perror("Ошибка при создании процесса");
        exit(1);
    }
    
    if (pid == 0) {
        printf("=== ДОЧЕРНИЙ ПРОЦЕСС (PID: %d) ===\n", getpid());
        

        FILE *fp = fopen("long_file.txt", "w");
        if (fp == NULL) {
            perror("Ошибка создания файла");
            exit(1);
        }
        
        for (int i = 1; i <= 50; i++) {
            fprintf(fp, "Строка %d из длинного файла cat\n", i);
        }
        fclose(fp);
        printf("Файл с 50 строками создан успешно\n");
        
        printf("Дочерний процесс выполняет: cat -n long_file.txt\n");
        
        if (system("cat -n /dev/null > /dev/null 2>&1") == 0) {
            execlp("cat", "cat", "-n", "long_file.txt", NULL);
        } else {
            execlp("cat", "cat", "long_file.txt", NULL);
        }
        
        perror("Ошибка при выполнении cat");
        exit(1);
        
    } else {
        printf("=== РОДИТЕЛЬСКИЙ ПРОЦЕСС (PID: %d) ===\n", getpid());
        printf("PID дочернего процесса: %d\n", pid);
        
        for (int i = 1; i <= 3; i++) {
            printf("Родитель выполняет работу... шаг %d/3\n", i);
            sleep(2);
        }

        printf("Ожидание завершения дочернего процесса...\n");
        
        int status;
        pid_t waited_pid = waitpid(pid, &status, 0);
        
        if (waited_pid == -1) {
            perror("Ошибка в waitpid");
        } else {
            printf("Информация о завершении дочернего процесса:\n");
            printf("  PID: %d\n", waited_pid);
             
            if (WIFEXITED(status)) {
                printf("  Причина: завершился нормально\n");
                printf("  Код возврата: %d\n", WEXITSTATUS(status));
            } else if (WIFSIGNALED(status)) {
                printf("  Причина: убит сигналом\n");
                printf("  Номер сигнала: %d\n", WTERMSIG(status));
            } else if (WIFSTOPPED(status)) {
                printf("  Причина: остановлен\n");
                printf("  Номер сигнала: %d\n", WSTOPSIG(status));
            } else {
                printf("  Причина: неизвестно\n");
            }
        }
        
        printf("ПОСЛЕДНЯЯ СТРОКА РОДИТЕЛЯ: Cat завершен, можно продолжать!\n");
    }
    

    if (access("long_file.txt", F_OK) == 0) {
        system("rm -f long_file.txt");
    }
    
    return 0;
}
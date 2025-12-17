#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>

int main() {
    pid_t pid = fork(); // здаем новый процесс

    if (pid == -1) {
        perror("fork failed");
        exit(1);
    }

    if (pid == 0) {
        // код выполняется в дочернем процессе
        printf("Дочерний процесс: запуск cat");
        execlp("cat", "cat", "/etc/passwd", NULL);
        // если execlp вернул управление, значит произошла ошибка
        perror("execlp failed");
        exit(1);
    } else {
        // код выполняется в родительском процессе
        printf("Родительский процесс: мой PID = %d, PID дочернего = %d\n",
                getpid(), pid);
        // ждем завершения дочернего процесса
        int status;
        waitpid(pid, &status, 0);
        printf("Родительский процесс: дочерний процесс завершился\n");
        printf("Родительский процесс: работа завершена\n");
    }
    return 0;
}
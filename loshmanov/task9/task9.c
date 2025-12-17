#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main(int argc, char *argv[]) {
    // Файл, который будет "длинным" для cat.
    // Можно передать через аргумент командной строки:
    // ./a.out some_long_file.txt
    const char *filename = (argc > 1) ? argv[1] : "test_for9.txt";

    // На всякий случай явно отключим буферизацию stdout,
    // чтобы вывод родителя и cat шёл сразу:
    setvbuf(stdout, NULL, _IONBF, 0);

    printf("Parent: going to create child process...\n");

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return EXIT_FAILURE;
    }

    if (pid == 0) {
        // --- Child process ---
        printf("Child: now executing cat for file: %s\n", filename);

        execlp("cat", "cat", filename, (char *)NULL);

        // Если мы здесь, то execlp не выполнился (ошибка)
        perror("execlp");
        _exit(127); // важно: _exit, а не exit
    }

    // --- Parent process ---
    printf("Parent: child created with pid = %d\n", pid);
    printf("Parent: child is running cat now, parent can do something else...\n");

    int status;
    pid_t w = waitpid(pid, &status, 0);  // ждём КОНКРЕТНО нашего ребёнка
    if (w == -1) {
        perror("waitpid");
        return EXIT_FAILURE;
    }

    if (WIFEXITED(status)) {
        printf("Parent: child exited normally with code %d\n",
               WEXITSTATUS(status));
    } else if (WIFSIGNALED(status)) {
        printf("Parent: child was terminated by signal %d\n",
               WTERMSIG(status));
    } else {
        printf("Parent: child finished in some unusual way\n");
    }

    // ВАЖНО: эта строка печатается уже ПОСЛЕ завершения дочернего процесса
    printf("Parent: this is the LAST line, printed after child finished.\n");

    return EXIT_SUCCESS;
}


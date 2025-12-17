#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s command [args...]\n", argv[0]);
        return EXIT_FAILURE;
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return EXIT_FAILURE;
    }

    if (pid == 0) {
        // --- Child process ---
        // argv[1] — команда, argv[1..] — аргументы для неё, массив уже NULL-терминирован
        execvp(argv[1], &argv[1]);

        // Если мы здесь — execvp не запустился (ошибка)
        perror("execvp");
        _exit(127);  // стандартный код "команда не найдена/не запустилась"
    }

    // --- Parent process ---
    int status = 0;
    pid_t w = waitpid(pid, &status, 0);
    if (w < 0) {
        perror("waitpid");
        return EXIT_FAILURE;
    }

    if (WIFEXITED(status)) {
        int code = WEXITSTATUS(status);
        printf("Child process (pid: %d) exited with code %d\n", pid, code);
    } else if (WIFSIGNALED(status)) {
        int sig = WTERMSIG(status);
        printf("Child process (pid: %d) was terminated by signal %d\n", pid, sig);
    } else {
        printf("Child process (pid: %d) finished in an unusual way\n", pid);
    }

    return EXIT_SUCCESS;
}


#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>

int main(int argc, char *argv[]) {

    if (argc < 2) {
        fprintf(stderr, "Использование: %s <команда> [аргументы...]\n", argv[0]);
        return 1;
    }

    pid_t pid = fork();

    if (pid < 0) {
        perror("fork");
        return 1;
    } else if (pid == 0) {
        // Child process
        execvp(argv[1], &argv[1]);

        perror("execvp");
        _exit(127);
    } else {
        // Parent process
        int status;
        if (wait(&status) == -1) {
            perror("waitpid");
            return 1;
        }

        printf("\nChild process %d finished with status: %d\n", pid, WEXITSTATUS(status));
    }

    return 0;
}

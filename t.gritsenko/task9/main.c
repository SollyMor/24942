#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>

int main(void) {

    printf("Starting child process...\n");
    fflush(stdout);

    pid_t pid = fork();

    if (pid < 0) {
        perror("fork");
        return 1;
    } else if (pid == 0) {
        // Child process
        execlp("cat", "cat", "test.txt", NULL);

        perror("execlp");
        _exit(127);
    } else {
        // Parent process
        if (wait(NULL) == -1) {
            perror("waitpid");
            return 1;
        }

        printf("\nChild process %d finished.\n", pid);
    }

    return 0;
}

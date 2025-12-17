#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <command> [args...]\n", argv[0]);
        exit(1);
    }

    pid_t pid = fork();
    
    if (pid == -1) {
        perror("fork failed");
        exit(1);
    }
    
    if (pid == 0) {
        execvp(argv[1], &argv[1]);
        perror("exec failed");
        exit(127);
    } else {
        int status;
        waitpid(pid, &status, 0);
        
        if (WIFEXITED(status)) {
            printf("Exit code: %d\n", WEXITSTATUS(status));
        } else if (WIFSIGNALED(status)) {
            printf("Terminated by signal: %d\n", WTERMSIG(status));
        }
    }
    
    return 0;
}
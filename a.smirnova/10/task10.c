#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Использование: %s <команда> [аргументы...]\n", argv[0]);
        exit(1);
    }

    char *command;
    char **cmd_args;
    int args_count;

    // Определяем команду и аргументы
    if (argc == 2) {
        command = "cat";
        args_count = 3;
        cmd_args = malloc(args_count * sizeof(char*));
        cmd_args[0] = "cat";
        cmd_args[1] = argv[1];
        cmd_args[2] = NULL;
    } else {
        command = argv[1];
        args_count = argc;
        cmd_args = malloc(args_count * sizeof(char*));
        cmd_args[0] = argv[1];
        for (int i = 2; i < argc; i++) {
            cmd_args[i-1] = argv[i];
        }
    }
    
    cmd_args[args_count-1] = NULL;

    printf("Родительский процесс: PID = %d\n", getpid());
    printf("Запускаю команду: %s", command);

    for (int i = 1; i < args_count-1; i++) {
        printf(" %s", cmd_args[i]);
    }
    printf("\n");

    pid_t pid = fork();

    if (pid == -1) {
        perror("fork failed");
        free(cmd_args);
        exit(1);
    }

    if (pid == 0) {
        printf("Дочерний процесс: PID = %d\n", getpid());
        execvp(command, cmd_args);
        perror("execvp failed");
        free(cmd_args);
        exit(1);
    } else {
        printf("Родительский процесс: жду завершения дочернего процесса\n");
        int status;
        waitpid(pid, &status, 0);
        if (WIFEXITED(status)) {
            int exit_code = WEXITSTATUS(status);
            printf("Родительский процесс: дочерний процесс завершился с кодом = %d\n", exit_code);
        } else if (WIFSIGNALED(status)) {
            int signal_num = WTERMSIG(status);
            printf("Родительский процесс: дочерний процесс убит сигналом = %d\n", signal_num);
        } else {
            printf("Родительский процесс: дочерний процесс завершился ненормально\n");
        }
    }

    free(cmd_args);
    return 0;
}
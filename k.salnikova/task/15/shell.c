#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include "shell.h"

char *infile, *outfile, *appfile;
struct command cmds[MAXCMDS];
char bkgrnd;

int handle_builtin(struct command *cmd) {
    if (strcmp(cmd->cmdargs[0], "cd") == 0) {
        char *path = cmd->cmdargs[1];
        if (path == NULL) {
            path = getenv("HOME");
            if (path == NULL) {
                fprintf(stderr, "cd: HOME not set\n");
                return 1;
            }
        }
        if (chdir(path) != 0) {
            perror("cd");
            return 1;
        }
        return 0;
    }
    else if (strcmp(cmd->cmdargs[0], "exit") == 0) {
        exit(0);
    }
    else if (strcmp(cmd->cmdargs[0], "echo") == 0) {
        for (int i = 1; cmd->cmdargs[i] != NULL; i++) {
            printf("%s", cmd->cmdargs[i]);
            if (cmd->cmdargs[i + 1] != NULL) {
                printf(" ");
            }
        }
        printf("\n");
        return 0;
    }
    return -1;
}

int main(int argc, char *argv[])
{
    char line[1024];
    char prompt[50];
    int status;

    sprintf(prompt,"[%s] ", argv[0]);

    while (promptline(prompt, line, sizeof(line)) > 0) {
        int ncmds = parseline(line);
        
        if (ncmds <= 0) {
            continue;
        }

        for (int i = 0; i < ncmds; i++) {
            if (cmds[i].cmdargs[0] == NULL) {
                continue;
            }

            // Встроенные команды выполняем без fork
            if (handle_builtin(&cmds[i]) != -1) {
                continue;
            }

            pid_t pid = fork();
            
            if (pid == 0) {
                // ПЕРЕНАПРАВЛЕНИЕ ВВОДА для первой команды
                if (i == 0 && infile != NULL) {
                    int fd_in = open(infile, O_RDONLY);
                    if (fd_in < 0) {
                        perror("open input file");
                        exit(1);
                    }
                    dup2(fd_in, STDIN_FILENO);
                    close(fd_in);
                }

                // ПЕРЕНАПРАВЛЕНИЕ ВЫВОДА для последней команды
                if (i == ncmds - 1) {
                    if (outfile != NULL) {
                        int fd_out = open(outfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                        if (fd_out < 0) {
                            perror("open output file");
                            exit(1);
                        }
                        dup2(fd_out, STDOUT_FILENO);
                        close(fd_out);
                    } else if (appfile != NULL) {
                        int fd_app = open(appfile, O_WRONLY | O_CREAT | O_APPEND, 0644);
                        if (fd_app < 0) {
                            perror("open append file");
                            exit(1);
                        }
                        dup2(fd_app, STDOUT_FILENO);
                        close(fd_app);
                    }
                }

                execvp(cmds[i].cmdargs[0], cmds[i].cmdargs);
                perror("execvp failed");
                exit(1);
            }
            else if (pid > 0) {
                if (!bkgrnd) {
                    waitpid(pid, &status, 0);
                } else {
                    printf("Background process PID: %d\n", pid);
                }
            }
            else {
                perror("fork failed");
            }
        }

        // Сбрасываем глобальные переменные для следующей команды
        infile = outfile = appfile = NULL;
        bkgrnd = 0;

        // Очищаем структуры команд
        for (int i = 0; i < MAXCMDS; i++) {
            for (int j = 0; j < MAXARGS; j++) {
                cmds[i].cmdargs[j] = NULL;
            }
        }
    }
    return 0;
}
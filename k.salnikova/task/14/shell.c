#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
#include <wait.h>
#include <stdlib.h>
#include <string.h>
#include "shell.h"

char *infile, *outfile, *appfile;
struct command cmds[MAXCMDS];
char bkgrnd;

int main(int argc, char *argv[])
{
    char line[1024];
    int ncmds;
    char prompt[50];
    pid_t pid;
    int status;

    sprintf(prompt,"[%s] ", argv[0]);

    while (promptline(prompt, line, sizeof(line)) > 0) {
        if ((ncmds = parseline(line)) <= 0)
            continue;

        // Проверяем что только одна команда (без пайпов)
        if (ncmds > 1) {
            fprintf(stderr, "Error: Only one command per line allowed\n");
            continue;
        }

        // Проверяем что нет перенаправлений (только & разрешено)
        if (infile != NULL || outfile != NULL || appfile != NULL) {
            fprintf(stderr, "Error: I/O redirection not allowed\n");
            continue;
        }
        if (cmds[0].cmdflag & (OUTPIP | INPIP)) {
            fprintf(stderr, "Error: Pipes not allowed\n");
            continue;
        }

        // FORK AND EXECUTE
        pid = fork();
        
        if (pid == 0) {  /* child process */
            execvp(cmds[0].cmdargs[0], cmds[0].cmdargs);
            fprintf(stderr, "Error: Command not found: %s\n", cmds[0].cmdargs[0]);
            exit(1);
        }
        else if (pid < 0) {
            perror("fork failed");
            continue;
        }
        else {  /* parent process */
            if (bkgrnd) {
                // Фоновый режим - выводим PID и не ждем завершения
                printf("Background process PID: %d\n", pid);
            } else {
                // Форграунд режим - ждем завершения
                wait(&status);
            }
        }

        // Сбрасываем флаг фонового режима для следующей команды
        bkgrnd = 0;

    }  /* close while */
    return 0;
}
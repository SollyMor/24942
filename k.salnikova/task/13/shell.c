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

main(int argc, char *argv[])
{
    register int i;
    char line[1024];
    int ncmds;
    char prompt[50];
    pid_t pid;
    int status;

    sprintf(prompt,"[%s] ", argv[0]);

    while (promptline(prompt, line, sizeof(line)) > 0) {
        if ((ncmds = parseline(line)) <= 0)
            continue;

        // Для задания №2: обрабатываем только первую команду
        // и игнорируем все метасимволы
        if (ncmds > 1) {
            fprintf(stderr, "Error: Only one command per line allowed\n");
            continue;
        }

        // Игнорируем фоновый режим, перенаправления и пайпы для задания №2
        if (bkgrnd) {
            fprintf(stderr, "Error: Background execution not allowed\n");
            continue;
        }
        if (infile != NULL || outfile != NULL || appfile != NULL) {
            fprintf(stderr, "Error: I/O redirection not allowed\n");
            continue;
        }
        if (cmds[0].cmdflag & (OUTPIP | INPIP)) {
            fprintf(stderr, "Error: Pipes not allowed\n");
            continue;
        }

        // FORK AND EXECUTE - только одна команда
        pid = fork();
        
        if (pid == 0) {  /* child process */
            // Execute the command
            execvp(cmds[0].cmdargs[0], cmds[0].cmdargs);
            // Если execvp возвращает управление, значит ошибка
            fprintf(stderr, "Error: Command not found: %s\n", cmds[0].cmdargs[0]);
            exit(1);
        }
        else if (pid < 0) {  /* fork failed */
            perror("fork failed");
            continue;
        }
        else {  /* parent process */
            // Ждем завершения подпроцесса
            wait(&status);
        }

    }  /* close while */
}
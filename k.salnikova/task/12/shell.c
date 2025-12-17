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

        // ДЛЯ ЗАДАНИЯ №1: Запрещаем все метасимволы
        if (ncmds > 1) {
            fprintf(stderr, "Error: Multiple commands not allowed\n");
            continue;
        }
        if (bkgrnd) {
            fprintf(stderr, "Error: Background execution not allowed\n");
            continue;
        }
        if (infile != NULL) {
            fprintf(stderr, "Error: Input redirection not allowed\n");
            continue;
        }
        if (outfile != NULL || appfile != NULL) {
            fprintf(stderr, "Error: Output redirection not allowed\n");
            continue;
        }
        if (cmds[0].cmdflag & (OUTPIP | INPIP)) {
            fprintf(stderr, "Error: Pipes not allowed\n");
            continue;
        }

        /* FORK AND EXECUTE */
        pid = fork();
        
        if (pid == 0) {  /* child process */
            execvp(cmds[0].cmdargs[0], cmds[0].cmdargs);
            fprintf(stderr, "Error: Command not found: %s\n", cmds[0].cmdargs[0]);
            exit(1);
        }
        else if (pid < 0) {  /* fork failed */
            perror("fork failed");
            continue;
        }
        else {  /* parent process */
            wait(&status);
        }

    }  /* close while */
    return 0;
}
/*
gcc -o shell shell.c parseline.c promptline.c
./shell
*/
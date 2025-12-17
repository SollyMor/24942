#include <unistd.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>

int main(void) {
    int fildes[2]; // ма�аcив дл�из 2 �^mлеменЭлеменТов длЯ п� пРогРамного ка�а
    if (pipe(fildes) == -1) {
        perror("pipe");
        return 1;
    }

    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        return 1;
    }

    if (pid == 0) {
        // Child process
        close(fildes[1]);

        char c;
        ssize_t n;
        while ((n = read(fildes[0], &c, 1)) > 0) {
            printf("%c", toupper((unsigned char)c));
        }

        if (n == -1) {
            perror("read");
        }

        printf("\n");
        close(fildes[0]);
        exit(0);
    } else {
        // Parent process
        close(fildes[0]);

        char *text = "Hello, World!";
        write(fildes[1], text, strlen(text));

        close(fildes[1]);
        wait(NULL);
    }
    return 0;
}
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int count = 0;

void handle_sigint(int signo) {
    printf("\a");
    fflush(stdout);
    count++;
}

void handle_sigquit(int signo) {
    printf("\nThe signal sounded %d times.\n", count);
    fflush(stdout);
    exit(0);
}

int main(void) {
    struct sigaction sa_int, sa_quit;

    //SIGINT
    sa_int.sa_handler = handle_sigint;
    sigemptyset(&sa_int.sa_mask);
    sa_int.sa_flags = 0;
    sigaction(SIGINT, &sa_int, NULL);

    //SIGQUIT
    sa_quit.sa_handler = handle_sigquit;
    sigemptyset(&sa_quit.sa_mask);
    sa_quit.sa_flags = 0;
    sigaction(SIGQUIT, &sa_quit, NULL);

    while (1)
        pause();

    return 0;
}

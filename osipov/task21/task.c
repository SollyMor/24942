#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int count = 0;

void handleSIGINT(int signum) {
    write(STDOUT_FILENO, "\a", 1);  // \a — звуковой сигнал
    count++;
}

void handleSIGQUIT(int signum) {
    printf("\nThe signal sounded %d times.", count);
    exit(0);
}

int main() {
    signal(SIGINT, handleSIGINT);
    signal(SIGQUIT, handleSIGQUIT);

    while (1);

    return 0;
}
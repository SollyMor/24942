#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

// Глобальный счетчик количества SIGINT
volatile sig_atomic_t sigint_count = 0;

// Обработчик SIGINT (Ctrl+C)
void handle_sigint(int signum) {
    (void)signum;
    sigint_count++;
    write(1, "\a", 1);  // звуковой сигнал
}

// Обработчик SIGQUIT (Ctrl+\)
void handle_sigquit(int signum) {
    (void)signum;
    printf("\nПолучено SIGQUIT.\n");
    printf("Сигнал SIGINT (Ctrl+C) был подан %d раз(а).\n", sigint_count);
    exit(0);
}

int main(void) {
    struct sigaction sa_int, sa_quit;

    // Настраиваем обработчик SIGINT
    sa_int.sa_handler = handle_sigint;
    sigemptyset(&sa_int.sa_mask);
    sa_int.sa_flags = 0;

    // Настраиваем обработчик SIGQUIT
    sa_quit.sa_handler = handle_sigquit;
    sigemptyset(&sa_quit.sa_mask);
    sa_quit.sa_flags = 0;

    // Устанавливаем обработчики
    sigaction(SIGINT, &sa_int, NULL);
    sigaction(SIGQUIT, &sa_quit, NULL);

    printf("Программа запущена.\n");
    printf("Нажмите Ctrl+C для звукового сигнала, Ctrl+\\ для выхода.\n");

    // Ждем сигналы (pause() приостанавливает процесс, пока не придет сигнал)
    while (1) {
        pause();
    }

    return 0;
}

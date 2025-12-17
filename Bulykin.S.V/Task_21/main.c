#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

volatile int signal_count = 0;

void sigint_handler(int sig) {
    signal_count++;
    // Выводим звуковой сигнал (ASCII символ BEL - \a)
    write(STDOUT_FILENO, "\a", 1);
    // Переустанавливаем обработчик, так как в некоторых системах он сбрасывается
    signal(SIGINT, sigint_handler);
}

void sigquit_handler(int sig) {
    printf("\nПрограмма получила SIGQUIT. Звуковой сигнал прозвучал %d раз(а).\n", signal_count);
    exit(EXIT_SUCCESS);
}

int main() {
    // Устанавливаем обработчики сигналов
    signal(SIGINT, sigint_handler);
    
    signal(SIGQUIT, sigquit_handler);
    
    printf("Программа запущена. Нажмите CTRL-C для звукового сигнала, CTRL-\\ для выхода.\n");
    
    // Бесконечный цикл
    while (1) {
        pause();  // Ожидание сигнала
    }
    
    return EXIT_SUCCESS;
}


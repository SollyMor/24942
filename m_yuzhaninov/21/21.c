#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>

// Используем volatile чтобы компилятор каждый раз перечитывал значение переменных
static volatile sig_atomic_t sigint_count = 0;  
static volatile sig_atomic_t quit_flag = 0;    

// Функция обработки Ctrl-C
void handle_sigint(int signum) 
{
    // Не используем поступивший сигнал
    (void)signum; 
    // Считаем сигналы
    sigint_count++;
    // Издаем звук
    write(STDOUT_FILENO, "\x07", 1); 
}

// Функция обработки SIGQUIT
void handle_sigquit(int signum) 
{
    // Не используем поступивший сигнал
    (void)signum;
    // Меняем флаг
    quit_flag = 1;
}

int main(void) 
{
    // Создаем структуру сигналов
    struct sigaction sa;

    // Настраиваем структуру на сигнал SIGINT
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_sigint;
    sa.sa_flags = 0;
    // Сохраняем настройку
    if (sigaction(SIGINT, &sa, NULL) == -1) 
    {
        perror("sigaction (SIGINT)");
        return 1;
    }

    // Настраиваем структуру на сигнал SIGQUIT
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_sigquit;
    sa.sa_flags = 0;
    // Сохраняем настройку
    if (sigaction(SIGQUIT, &sa, NULL) == -1) 
    {
        perror("sigaction (SIGQUIT)");
        return 1;
    }

    printf("Ожидание сигналов... (Ctrl-C — звук, Ctrl-\\ — выход)\n");

    // Пока не будет получен сигнал SIGQUIT, ждем новых сигналов
    while (!quit_flag) 
    {
        pause(); 
    }

    printf("\nSIGINT получен %d раз. Завершение.\n", sigint_count);
    return 0;
}

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

volatile sig_atomic_t count = 0;  // Используем volatile sig_atomic_t

void handleSIGINT(int sig) 
{      // Добавляем параметр сигнала
    write(STDOUT_FILENO, "\a", 1); // Безопасный вывод
    count++;
}

void handleSIGQUIT(int sig) 
{     // Добавляем параметр сигнала
    printf("\nThe signal sounded %d times.\n", count);
    exit(0);
}

int main() 
{
    // Используем sigaction вместо signal для лучшей переносимости
    struct sigaction sa_int, sa_quit;
    
    sa_int.sa_handler = handleSIGINT;
    sigemptyset(&sa_int.sa_mask);
    sa_int.sa_flags = 0;
    
    sa_quit.sa_handler = handleSIGQUIT;
    sigemptyset(&sa_quit.sa_mask);
    sa_quit.sa_flags = 0;
    
    if (sigaction(SIGINT, &sa_int, NULL) == -1) 
    {
        perror("sigaction SIGINT");
        return 1;
    }
    
    if (sigaction(SIGQUIT, &sa_quit, NULL) == -1) 
    {
        perror("sigaction SIGQUIT");
        return 1;
    }
    
    printf("Program started. Press Ctrl+C for beep, Ctrl+\\ to quit.\n");
    
    while (1) 
    {
        pause();  
    }
    
    return 0;
}

#include <stdio.h>
#include <unistd.h>
#include <signal.h>

static volatile sig_atomic_t count = 0;  
static volatile sig_atomic_t quit = 0;    

void handler_sigquit(int signum) 
{
    (void)signum;
    quit = 1;
}

void handler_sigint(int signum) 
{
    (void)signum; 
    count++;
    write(STDOUT_FILENO, "\x07", 1); 
}

int main(void) 
{
    struct sigaction sigint = {0}, sigquit = {0};

    // Настройка под сигнал SIGINT
    sigint.sa_handler = handler_sigint;
    if (sigaction(SIGINT, &sigint, NULL) == -1) 
    {
        perror("SIGINT");
        return 1;
    }

    // Настройка под сигнал SIGQUIT
    sigquit.sa_handler = handler_sigquit;
    if (sigaction(SIGQUIT, &sigquit, NULL) == -1) 
    {
        perror("SIGQUIT");
        return 1;
    }

    printf("Ctrl-C - издать звук, Ctrl-\\ — завершить программу\n");

    while (!quit) 
    {
        pause(); 
    }

    printf("\nЗвук был произведен %d раз\n", count);
    return 0;
}
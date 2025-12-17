#include <stdio.h>
#include <signal.h>
#include <unistd.h>

static volatile sig_atomic_t cnt = 0, quit = 0;

void h_int(int s) { (void)s; cnt++; printf("\a"); fflush(stdout);}
void h_quit(int s){ (void)s; quit = 1; }

int main(void) {
    sigset(SIGINT,  h_int);
    sigset(SIGQUIT, h_quit);
    for (;;){
        pause();
        if (quit) { printf("написано %d раз(а)\n", cnt); return 0; }
    }
}

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

int beep_count = 0;

int main() {
    printf("Program started. Type text and press Enter to beep. Press 'q' + Enter to exit.\n");
    
    while(1) {
        int c = getchar();
        
        if (c == EOF) {
            printf("\nBeep count: %d\n", beep_count);
            exit(0);
        }
        
        if (c == '\n') {
            write(1, "\a", 1); 
            beep_count++;
            printf("Beep! (line completed)\n");
        }
        
        if (c == 'q') {
            int next_char = getchar();
            if (next_char == '\n') {
                printf("\nBeep count: %d\n", beep_count);
                exit(0);
            }
        }
    }
    
    return 0;
}
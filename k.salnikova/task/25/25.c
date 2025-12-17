#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <sys/wait.h>

#define BUFFER_SIZE 1024

int main() {
    int pipefd[2];
    pid_t pid;
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;
    
    if (pipe(pipefd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }
    
    pid = fork();
    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }
    
    if (pid == 0) {
        close(pipefd[1]); 
        
        while ((bytes_read = read(pipefd[0], buffer, BUFFER_SIZE)) > 0) {
            for (int i = 0; i < bytes_read; i++) {
                buffer[i] = toupper(buffer[i]);
            }
            write(STDOUT_FILENO, buffer, bytes_read);
        }
        
        close(pipefd[0]); 
        exit(EXIT_SUCCESS);
        
    } else {
        close(pipefd[0]); 
        
        const char *text = "Hello World!\n"
                          "This is a Test String\n"
                          "Programming in C is FUN!\n"
                          "End of transmission.\n";
        
        write(pipefd[1], text, strlen(text));
        
        close(pipefd[1]);
        
        wait(NULL);
    }
    
    return 0;
}
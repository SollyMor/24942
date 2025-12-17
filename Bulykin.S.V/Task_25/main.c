#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <ctype.h>
#include <string.h>

#define BUFFER_SIZE 1024

int main() {
    int pipefd[2];
    pid_t pid;
    char buffer[BUFFER_SIZE];
    ssize_t nbytes;
    
    if (pipe(pipefd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }
    
    pid = fork();
    
    if (pid < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        close(pipefd[1]);
        
        while ((nbytes = read(pipefd[0], buffer, BUFFER_SIZE - 1)) > 0) {
            buffer[nbytes] = '\0';
            
            for (int i = 0; i < nbytes; i++) {
                buffer[i] = toupper(buffer[i]);
            }
            
            write(STDOUT_FILENO, buffer, nbytes);
        }
        
        if (nbytes == -1) {
            perror("read");
            exit(EXIT_FAILURE);
        }
        
        close(pipefd[0]);
        exit(EXIT_SUCCESS);
    } else {
        close(pipefd[0]);
        
        const char *text = "Hello World! This is a Test String with Mixed Case.\n"
                          "Programming in C is Fun! *nervously laughing*\n"
                          "SunOS 5.11 is a Unix Operating System.\n";
        
        if (write(pipefd[1], text, strlen(text)) == -1) {
            perror("write");
            exit(EXIT_FAILURE);
        }
        
        close(pipefd[1]);
        
        waitpid(pid, NULL, 0);
    }
    
    return EXIT_SUCCESS;
}


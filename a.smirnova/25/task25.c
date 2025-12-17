#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <sys/wait.h>

int main() {
    int pipefd[2];
    pid_t pid;
    
    if (pipe(pipefd) == -1) {
        perror("pipe");
        exit(1);
    }
    
    pid = fork();
    
    if (pid == -1) {
        perror("fork");
        exit(1);
    }
    
    if (pid == 0) {
        close(pipefd[1]);
        
        char buffer[1024];
        int bytes_read;
        
        while ((bytes_read = read(pipefd[0], buffer, sizeof(buffer))) > 0) {
            for (int i = 0; i < bytes_read; i++) {
                buffer[i] = toupper(buffer[i]);
            }
            write(STDOUT_FILENO, buffer, bytes_read);
        }
        
        close(pipefd[0]);
        exit(0);
    } else {
        close(pipefd[0]);
        
        char *text = "Hello World!\n"
                     "This is TEst\n"
                     "QwErT yUiOpA\n";
        
        write(pipefd[1], text, strlen(text));
        
        close(pipefd[1]);
        
        wait(NULL);
        printf("Родительский процесс завершен.\n");
    }
    
    return 0;
}
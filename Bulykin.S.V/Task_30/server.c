#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <ctype.h>
#include <string.h>

#define SOCKET_PATH "/tmp/task30_socket"
#define BUFFER_SIZE 1024

int main() {
    int server_fd, client_fd;
    struct sockaddr_un server_addr, client_addr;
    socklen_t client_len;
    char buffer[BUFFER_SIZE];
    ssize_t nbytes;
    
    server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }
    
    unlink(SOCKET_PATH);
    
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, SOCKET_PATH, sizeof(server_addr.sun_path) - 1);
    
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    
    if (listen(server_fd, 1) == -1) {
        perror("listen");
        close(server_fd);
        unlink(SOCKET_PATH);
        exit(EXIT_FAILURE);
    }
    
    client_len = sizeof(client_addr);
    client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
    if (client_fd == -1) {
        perror("accept");
        close(server_fd);
        unlink(SOCKET_PATH);
        exit(EXIT_FAILURE);
    }
    
    while ((nbytes = read(client_fd, buffer, BUFFER_SIZE - 1)) > 0) {
        buffer[nbytes] = '\0';
        
        for (int i = 0; i < nbytes; i++) {
            buffer[i] = toupper(buffer[i]);
        }
        
        write(STDOUT_FILENO, buffer, nbytes);
    }
    
    if (nbytes == -1) {
        perror("read");
    }
    
    close(client_fd);
    close(server_fd);
    unlink(SOCKET_PATH);
    
    return EXIT_SUCCESS;
}


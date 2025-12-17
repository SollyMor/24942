#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <string.h>

#define SOCKET_PATH "/tmp/task30_socket"
#define BUFFER_SIZE 1024

int main() {
    int client_fd;
    struct sockaddr_un server_addr;
    char buffer[BUFFER_SIZE];
    ssize_t nbytes;
    
    client_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (client_fd == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }
    
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, SOCKET_PATH, sizeof(server_addr.sun_path) - 1);
    
    if (connect(client_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("connect");
        close(client_fd);
        exit(EXIT_FAILURE);
    }
    
    while ((nbytes = read(STDIN_FILENO, buffer, BUFFER_SIZE)) > 0) {
        if (write(client_fd, buffer, nbytes) == -1) {
            perror("write");
            close(client_fd);
            exit(EXIT_FAILURE);
        }
    }
    
    if (nbytes == -1) {
        perror("read");
    }
    
    close(client_fd);
    
    return EXIT_SUCCESS;
}


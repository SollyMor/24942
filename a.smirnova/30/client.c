#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

#define SOCKET_PATH "/tmp/30_socket"
#define BUFFER_SIZE 1024

int main() {
    int sockfd;
    struct sockaddr_un server_addr;
    char buffer[BUFFER_SIZE];
    ssize_t bytes_written;
    
    sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sockfd == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }
    
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, SOCKET_PATH, sizeof(server_addr.sun_path) - 1);
    
    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("connect");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    
    printf("Подключено к серверу. Введите текст (Ctrl+D для завершения):\n");
    
    while (fgets(buffer, BUFFER_SIZE, stdin) != NULL) {
        bytes_written = write(sockfd, buffer, strlen(buffer));
        if (bytes_written == -1) {
            perror("write");
            break;
        }
    }
    
    printf("Соединение закрыто\n");
    close(sockfd);
    return 0;
}
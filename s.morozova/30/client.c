#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <string.h>

#define SOCKET_PATH "/tmp/simple_socket"
#define BUFFER_SIZE 1024

int main(int argc, char *argv[]) {
    int sockfd;
    struct sockaddr_un server_addr;
    
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <text>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    
    sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sockfd == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }
    
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, SOCKET_PATH, sizeof(server_addr.sun_path) - 1);
    
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("connect");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    
    // Отправляем текст из аргументов командной строки
    write(sockfd, argv[1], strlen(argv[1]));
    
    close(sockfd);
    return EXIT_SUCCESS;
}
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <string.h>

#define SOCKET_PATH "/tmp/task31_socket"

int main(int argc, char *argv[]) {
    int sockfd;
    struct sockaddr_un server_addr;
    
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <text> [count]\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    
    int count = 50;  // По умолчанию 50 сообщений
    if (argc > 2) {
        count = atoi(argv[2]);
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
    
    // Отправляем N сообщений
    for (int i = 0; i < count; i++) {
        char message[256];
        snprintf(message, sizeof(message), "%s%d\n", argv[1], i+1);
        write(sockfd, message, strlen(message));
        usleep(10000);  // 10ms задержка между сообщениями
    }
    
    close(sockfd);
    return EXIT_SUCCESS;
}
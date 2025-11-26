#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

#define SOCKET_PATH "/tmp/async_socket"
#define BUFFER_SIZE 1024

int main() {
    int sockfd;
    struct sockaddr_un addr;
    char buffer[BUFFER_SIZE];
    
    // Создаем сокет
    sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sockfd == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }
    
    // Настраиваем адрес сервера
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);
    
    // Подключаемся к серверу
    if (connect(sockfd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        perror("connect");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    
    printf("Подключен к серверу\n");
    
    // Отправляем несколько сообщений
    for (int i = 1; i <= 3; i++) {
        snprintf(buffer, BUFFER_SIZE, "Сообщение %d от клиента\n", i);
        write(sockfd, buffer, strlen(buffer));
        printf("Отправлено: %s", buffer);
        sleep(1);
    }
    
    close(sockfd);
    return 0;
}
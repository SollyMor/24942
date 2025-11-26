#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

#define SOCKET_PATH "/tmp/case_converter_epoll_socket"
#define BUFFER_SIZE 1024

int main(int argc, char *argv[]) {
    int sockfd;
    struct sockaddr_un server_addr;
    char buffer[BUFFER_SIZE];
    
    if (argc < 2) {
        printf("Использование: %s <client_name>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    
    char *client_name = argv[1];
    
    // Создаем сокет
    sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sockfd == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }
    
    // Настраиваем адрес сервера
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, SOCKET_PATH, sizeof(server_addr.sun_path) - 1);
    
    // Подключаемся к серверу
    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("connect");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    
    printf("Клиент '%s' подключен к серверу\n", client_name);
    
    // Читаем приветственное сообщение
    ssize_t bytes_read = read(sockfd, buffer, BUFFER_SIZE - 1);
    if (bytes_read > 0) {
        buffer[bytes_read] = '\0';
        printf("Сервер: %s", buffer);
    }
    
    // Отправляем несколько сообщений
    for (int i = 1; i <= 3; i++) {
        snprintf(buffer, BUFFER_SIZE, "Сообщение %d от клиента %s\n", i, client_name);
        
        printf("Отправляю: %s", buffer);
        write(sockfd, buffer, strlen(buffer));
        
        // Ждем ответа
        sleep(1);
    }
    
    printf("Клиент '%s' завершает работу\n", client_name);
    close(sockfd);
    return 0;
}
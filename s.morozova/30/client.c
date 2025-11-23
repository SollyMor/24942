#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>

#define SOCKET_PATH "/tmp/case_converter_socket"
#define BUFFER_SIZE 1024

int main() {
    int client_fd;
    struct sockaddr_un server_addr;
    char buffer[BUFFER_SIZE];
    
    // Создаем сокет
    client_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (client_fd == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }
    
    // Устанавливаем адрес сервера
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, SOCKET_PATH, sizeof(server_addr.sun_path) - 1);
    
    // Подключаемся к серверу
    if (connect(client_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        if (errno == ECONNREFUSED) {
            printf("Сервер занят. Попробуйте позже.\n");
        } else {
            perror("connect");
        }
        close(client_fd);
        exit(EXIT_FAILURE);
    }
    
    printf("Подключено к серверу. Введите текст (Ctrl+D для завершения):\n");
    
    // Читаем текст с stdin и отправляем на сервер
    while (fgets(buffer, BUFFER_SIZE, stdin) != NULL) {
        if (write(client_fd, buffer, strlen(buffer)) == -1) {
            perror("write");
            break;
        }
    }
    
    printf("Клиент завершает работу.\n");
    close(client_fd);
    
    return 0;
}
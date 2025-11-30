#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <signal.h>

#define SOCKET_PATH "/tmp/case_converter_socket"
#define BUFFER_SIZE 1024

int client_fd = -1;

void cleanup(int sig) {
    if (client_fd != -1) {
        close(client_fd);
    }
    exit(0);
}

int main() {
    struct sockaddr_un server_addr;
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;
    
    // Обработчик сигналов для корректного завершения
    signal(SIGINT, cleanup);
    signal(SIGTERM, cleanup);
    
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
        perror("connect");
        close(client_fd);
        exit(EXIT_FAILURE);
    }
    
    printf("Connected to server. Enter text (Ctrl+C to exit):\n");
    
    // Читаем текст из стандартного ввода и отправляем серверу
    while ((bytes_read = read(STDIN_FILENO, buffer, BUFFER_SIZE)) > 0) {
        if (write(client_fd, buffer, bytes_read) == -1) {
            perror("write");
            break;
        }
    }
    
    if (bytes_read == -1) {
        perror("read");
    }
    
    printf("Disconnecting from server\n");
    close(client_fd);
    
    return 0;
}
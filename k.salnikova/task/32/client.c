#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

#define SOCKET_PATH "/tmp/async_socket"
#define BUFFER_SIZE 1024

int main() {
    int client_fd;
    struct sockaddr_un server_addr;
    char buffer[BUFFER_SIZE];
    int bytes_read;
    
    client_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (client_fd == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }
    
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, SOCKET_PATH, sizeof(server_addr.sun_path) - 1);
    
    if (connect(client_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("connect");
        close(client_fd);
        exit(EXIT_FAILURE);
    }
    
    printf("Подключено к асинхронному серверу. Введите текст (Ctrl+D для завершения):\n");
    
    while ((bytes_read = read(0, buffer, BUFFER_SIZE)) > 0) {
        if (write(client_fd, buffer, bytes_read) == -1) {
            perror("write");
            break;
        }
    }
    
    if (bytes_read == -1) {
        perror("read");
    }
    
    printf("Соединение закрыто\n");
    close(client_fd);
    
    return 0;
}
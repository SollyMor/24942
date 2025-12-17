#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

#define SOCKET "./mysocket"

int main() 
{
    int client_fd;
    
    // Создаем сокет
    client_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (client_fd == -1) 
    {
        perror("socket");
        close(client_fd);
        exit(EXIT_FAILURE);
    }
    
    struct sockaddr_un server_addr = {0};
    
    // Заполняем структуру
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, SOCKET, sizeof(server_addr.sun_path) - 1);
    
    // Подключаемся к серверу
    if (connect(client_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) 
    {
        perror("connect");
        close(client_fd);
        exit(EXIT_FAILURE);
    }
    printf("Подключено к серверу. Введите текст (Ctrl+D для завершения):\n");

    char buffer[500];
    long bytes_read;
    
    // Читаем ввод пользователя и отправляем серверу
    while ((bytes_read = read(STDIN_FILENO, buffer, 500)) > 0) 
    {
        if (write(client_fd, buffer, bytes_read) != bytes_read) 
        {
            perror("write");
            break;
        }
    }
    
    if (bytes_read == -1) 
    {
        perror("read");
    }
    
    printf("Соединение закрыто\n");
    
    // Закрываем соединение
    close(client_fd);
    
    return 0;
}
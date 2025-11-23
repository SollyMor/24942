#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <ctype.h>
#include <errno.h>

#define SOCKET_PATH "/tmp/case_converter_socket"
#define BUFFER_SIZE 1024

int main() {
    int server_fd, client_fd;
    struct sockaddr_un server_addr, client_addr;
    socklen_t client_len;
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;
    
    // Создаем сокет
    server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }
    
    // Устанавливаем адрес сервера
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, SOCKET_PATH, sizeof(server_addr.sun_path) - 1);
    
    // Удаляем старый сокет если он существует
    unlink(SOCKET_PATH);
    
    // Привязываем сокет к адресу
    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    
    // Начинаем прослушивание
    if (listen(server_fd, 5) == -1) {
        perror("listen");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    
    printf("Сервер запущен и ожидает подключения...\n");
    
    while (1) {
        // Принимаем подключение
        client_len = sizeof(client_addr);
        client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
        if (client_fd == -1) {
            perror("accept");
            continue; // Продолжаем ожидать подключения даже при ошибке
        }
        
        printf("Клиент подключен!\n");
        
        // Читаем данные от клиента и преобразуем в верхний регистр
        while ((bytes_read = read(client_fd, buffer, BUFFER_SIZE - 1)) > 0) {
            buffer[bytes_read] = '\0';
            
            // Преобразуем каждый символ в верхний регистр
            for (int i = 0; i < bytes_read; i++) {
                buffer[i] = toupper(buffer[i]);
            }
            
            // Выводим результат
            printf("Преобразованный текст: %s", buffer);
            fflush(stdout);
        }
        
        if (bytes_read == -1) {
            perror("read");
        }
        
        printf("Соединение разорвано. Ожидаем новое подключение...\n");
        
        // Закрываем клиентский сокет
        close(client_fd);
    }
    
    // Закрываем серверный сокет и удаляем файл сокета (этот код никогда не выполнится в текущей реализации)
    close(server_fd);
    unlink(SOCKET_PATH);
    
    return 0;
}
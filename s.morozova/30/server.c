#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <time.h>
#include <ctype.h>

#define SOCKET_PATH "/tmp/uppercase_socket"
#define BUFFER_SIZE 1024

int main() {
    int server_fd, client_fd;
    struct sockaddr_un server_addr, client_addr;
    socklen_t client_len;
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;
    time_t start_time, end_time;
    
    // Засекаем время начала работы сервера
    time(&start_time);
    printf("Сервер запущен в: %s", ctime(&start_time));
    
    // Создаем сокет
    server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }
    
    // Удаляем старый сокет файл если существует
    unlink(SOCKET_PATH);
    
    // Настраиваем адрес сервера
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, SOCKET_PATH, sizeof(server_addr.sun_path) - 1);
    
    // Привязываем сокет к адресу
    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    
    // Начинаем слушать соединения
    if (listen(server_fd, 5) == -1) {
        perror("listen");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    
    printf("Сервер слушает на сокете: %s\n", SOCKET_PATH);
    
    // Принимаем соединение от клиента
    client_len = sizeof(client_addr);
    client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
    if (client_fd == -1) {
        perror("accept");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    
    printf("Клиент подключен\n");
    
    // Читаем данные от клиента и преобразуем в верхний регистр
    while ((bytes_read = read(client_fd, buffer, BUFFER_SIZE - 1)) > 0) {
        buffer[bytes_read] = '\0';
        
        // Преобразуем каждый символ в верхний регистр
        for (int i = 0; i < bytes_read; i++) {
            buffer[i] = toupper(buffer[i]);
        }
        
        // Выводим преобразованный текст
        printf("СЕРВЕР: %s", buffer);
        fflush(stdout);
    }
    
    if (bytes_read == -1) {
        perror("read");
    }
    
    // Закрываем соединения
    close(client_fd);
    close(server_fd);
    
    // Удаляем файл сокета
    unlink(SOCKET_PATH);
    
    // Засекаем время окончания и выводим общее время работы
    time(&end_time);
    printf("\nСервер завершил работу в: %s", ctime(&end_time));
    printf("Общее время работы сервера: %.2f секунд\n", difftime(end_time, start_time));
    
    return 0;
}
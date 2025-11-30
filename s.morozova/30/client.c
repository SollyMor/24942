#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <time.h>

#define SOCKET_PATH "/tmp/uppercase_socket"
#define BUFFER_SIZE 1024

int main() {
    int client_fd;
    struct sockaddr_un server_addr;
    char buffer[BUFFER_SIZE];
    time_t start_time, message_time;
    int message_count = 3;
    
    // Засекаем общее время начала работы клиента
    time(&start_time);
    printf("Клиент запущен в: %s", ctime(&start_time));
    
    // Создаем сокет
    client_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (client_fd == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }
    
    // Настраиваем адрес сервера
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, SOCKET_PATH, sizeof(server_addr.sun_path) - 1);
    
    // Подключаемся к серверу
    if (connect(client_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("connect");
        close(client_fd);
        exit(EXIT_FAILURE);
    }
    
    printf("Подключено к серверу\n");
    
    // Отправляем 3 сообщения
    for (int i = 0; i < message_count; i++) {
        time(&message_time);
        
        // Формируем сообщение с временем отправки
        snprintf(buffer, BUFFER_SIZE, 
                 "Сообщение %d от клиента. Время отправки: %s", 
                 i + 1, ctime(&message_time));
        
        // Отправляем сообщение серверу
        if (write(client_fd, buffer, strlen(buffer)) == -1) {
            perror("write");
            break;
        }
        
        printf("Клиент отправил сообщение %d\n", i + 1);
        
        // Небольшая пауза между сообщениями
        sleep(1);
    }
    
    // Закрываем соединение
    close(client_fd);
    
    time_t end_time;
    time(&end_time);
    printf("Клиент завершил работу в: %s", ctime(&end_time));
    printf("Общее время работы клиента: %.2f секунд\n", difftime(end_time, start_time));
    
    return 0;
}
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <poll.h>
#include <ctype.h>
#include <errno.h>

#define SOCKET_PATH "/tmp/case_converter_socket"
#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024

typedef struct {
    int fd;
    char buffer[BUFFER_SIZE];
} client_t;

int main() {
    int server_fd, new_client_fd;
    struct sockaddr_un server_addr, client_addr;
    socklen_t client_len;
    struct pollfd fds[MAX_CLIENTS + 1];
    client_t clients[MAX_CLIENTS];
    int nfds = 1;
    int i, rc;
    
    // Инициализация массива клиентов
    for (i = 0; i < MAX_CLIENTS; i++) {
        clients[i].fd = -1;
    }
    
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
    
    // Удаляем старый сокет файл если существует
    unlink(SOCKET_PATH);
    
    // Привязываем сокет к адресу
    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    
    // Начинаем слушать соединения
    if (listen(server_fd, MAX_CLIENTS) == -1) {
        perror("listen");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    
    printf("Server listening on socket: %s\n", SOCKET_PATH);
    
    // Настраиваем poll для серверного сокета
    fds[0].fd = server_fd;
    fds[0].events = POLLIN;
    fds[0].revents = 0;
    
    while (1) {
        rc = poll(fds, nfds, -1);
        if (rc == -1) {
            perror("poll");
            break;
        }
        
        // Проверяем серверный сокет на новые подключения
        if (fds[0].revents & POLLIN) {
            client_len = sizeof(client_addr);
            new_client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
            if (new_client_fd == -1) {
                perror("accept");
                continue;
            }
            
            int client_index = -1;
            for (i = 0; i < MAX_CLIENTS; i++) {
                if (clients[i].fd == -1) {
                    client_index = i;
                    break;
                }
            }
            
            if (client_index != -1) {
                clients[client_index].fd = new_client_fd;
                
                if (nfds < MAX_CLIENTS + 1) {
                    fds[nfds].fd = new_client_fd;
                    fds[nfds].events = POLLIN;
                    fds[nfds].revents = 0;
                    nfds++;
                    printf("Client %d connected (total clients: %d)\n", 
                           client_index, nfds - 1);
                } else {
                    printf("Too many clients, rejecting connection\n");
                    close(new_client_fd);
                }
            } else {
                printf("No free slots for new client\n");
                close(new_client_fd);
            }
        }
        
        // Проверяем клиентские сокеты на данные
        for (i = 1; i < nfds; i++) {
            if (fds[i].revents & POLLIN) {
                char temp_buffer[BUFFER_SIZE];
                ssize_t bytes_read = read(fds[i].fd, temp_buffer, BUFFER_SIZE - 1);
                
                if (bytes_read > 0) {
                    temp_buffer[bytes_read] = '\0';
                    
                    // Сохраняем оригинальный текст для вывода
                    char original[BUFFER_SIZE];
                    strncpy(original, temp_buffer, BUFFER_SIZE);
                    
                    // Преобразуем в верхний регистр
                    for (int j = 0; j < bytes_read; j++) {
                        temp_buffer[j] = toupper(temp_buffer[j]);
                    }
                    
                    // Выводим на сервере
                    printf("[Client %d original]: %s", i, original);
                    printf("[Client %d converted]: %s", i, temp_buffer);
                    fflush(stdout);
                    
                    // Отправляем преобразованный текст обратно клиенту
                    if (write(fds[i].fd, temp_buffer, bytes_read) == -1) {
                        perror("write back to client");
                    }
                    
                } else if (bytes_read == 0) {
                    printf("Client %d disconnected\n", i);
                    close(fds[i].fd);
                    
                    for (int j = 0; j < MAX_CLIENTS; j++) {
                        if (clients[j].fd == fds[i].fd) {
                            clients[j].fd = -1;
                            break;
                        }
                    }
                    
                    fds[i].fd = -1;
                } else {
                    perror("read");
                }
            }
        }
        
        // Убираем закрытые дескрипторы из массива poll
        for (i = 1; i < nfds; i++) {
            if (fds[i].fd == -1) {
                for (int j = i; j < nfds - 1; j++) {
                    fds[j] = fds[j + 1];
                }
                nfds--;
                i--;
            }
        }
    }
    
    // Очистка
    for (i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].fd != -1) {
            close(clients[i].fd);
        }
    }
    close(server_fd);
    unlink(SOCKET_PATH);
    
    return 0;
}
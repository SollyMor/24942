#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <poll.h>
#include <ctype.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>

// ./server & server_pid=$! && sleep 1 && (./client 1 & ./client 2 & ./client 3 & wait) && kill "$server_pid" && wait "$server_pid" 2>/dev/null

#define SOCKET_PATH "/tmp/case_converter_socket"
#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024

typedef struct {
    int fd;
    char buffer[BUFFER_SIZE];
    int message_count;
} client_t;

void print_current_time() {
    struct timeval tv;
    struct tm *tm_info;
    char time_buffer[26];
    
    gettimeofday(&tv, NULL);
    tm_info = localtime(&tv.tv_sec);
    
    strftime(time_buffer, 26, "%Y-%m-%d %H:%M:%S", tm_info);
    printf("[%s.%03ld] ", time_buffer, tv.tv_usec / 1000);
}

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
        clients[i].message_count = 0;
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
                clients[client_index].message_count = 0;
                
                if (nfds < MAX_CLIENTS + 1) {
                    fds[nfds].fd = new_client_fd;
                    fds[nfds].events = POLLIN;
                    fds[nfds].revents = 0;
                    nfds++;
                    print_current_time();
                    printf("Client %d connected (total clients: %d)\n", 
                           client_index, nfds - 1);
                } else {
                    print_current_time();
                    printf("Too many clients, rejecting connection\n");
                    close(new_client_fd);
                }
            } else {
                print_current_time();
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
                    
                    // Находим индекс клиента в массиве
                    int client_index = -1;
                    for (int j = 0; j < MAX_CLIENTS; j++) {
                        if (clients[j].fd == fds[i].fd) {
                            client_index = j;
                            clients[j].message_count++;
                            break;
                        }
                    }
                    
                    // Сохраняем оригинальный текст для вывода
                    char original[BUFFER_SIZE];
                    strncpy(original, temp_buffer, BUFFER_SIZE);
                    original[strlen(original) - 1] = '\0'; // убираем \n для красивого вывода
                    
                    // Преобразуем в верхний регистр
                    for (int j = 0; j < bytes_read; j++) {
                        temp_buffer[j] = toupper(temp_buffer[j]);
                    }
                    
                    // Выводим на сервере с временем и номером сообщения
                    print_current_time();
                    if (client_index != -1) {
                        printf("Client %d, Message #%d: '%s' -> '%s'", 
                               client_index, 
                               clients[client_index].message_count,
                               original, 
                               temp_buffer);
                    } else {
                        printf("Unknown client, Message: '%s' -> '%s'", 
                               original, 
                               temp_buffer);
                    }
                    
                    // Отправляем преобразованный текст обратно клиенту
                    if (write(fds[i].fd, temp_buffer, bytes_read) == -1) {
                        perror("write back to client");
                    }
                    
                } else if (bytes_read == 0) {
                    // Находим индекс клиента для вывода информации
                    int client_index = -1;
                    for (int j = 0; j < MAX_CLIENTS; j++) {
                        if (clients[j].fd == fds[i].fd) {
                            client_index = j;
                            break;
                        }
                    }
                    
                    print_current_time();
                    if (client_index != -1) {
                        printf("Client %d disconnected (sent %d messages)\n", 
                               client_index, clients[client_index].message_count);
                    } else {
                        printf("Unknown client disconnected\n");
                    }
                    
                    close(fds[i].fd);
                    
                    for (int j = 0; j < MAX_CLIENTS; j++) {
                        if (clients[j].fd == fds[i].fd) {
                            clients[j].fd = -1;
                            clients[j].message_count = 0;
                            break;
                        }
                    }
                    
                    fds[i].fd = -1;
                } else {
                    perror("read");
                }
            }
        }
        
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
    
    for (i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].fd != -1) {
            close(clients[i].fd);
        }
    }
    close(server_fd);
    unlink(SOCKET_PATH);
    
    return 0;
}
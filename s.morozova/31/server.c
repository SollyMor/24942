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
#include <signal.h>

#define SOCKET_PATH "/tmp/case_converter_socket"
#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024
#define TOTAL_MESSAGES 30  // Ожидаем 30 сообщений от каждого клиента

typedef struct {
    int fd;
    int client_id;
    int message_count;
} client_t;

volatile sig_atomic_t running = 1;
struct timeval start_time;
int total_received_messages = 0;
int clients_connected = 0;
int clients_completed = 0;

void print_current_time() {
    struct timeval tv;
    struct tm *tm_info;
    char time_buffer[26];
    
    gettimeofday(&tv, NULL);
    tm_info = localtime(&tv.tv_sec);
    
    strftime(time_buffer, 26, "%Y-%m-%d %H:%M:%S", tm_info);
    printf("[%s.%06ld] ", time_buffer, tv.tv_usec);
}

void print_uptime() {
    struct timeval end_time;
    gettimeofday(&end_time, NULL);
    
    long seconds = end_time.tv_sec - start_time.tv_sec;
    long microseconds = end_time.tv_usec - start_time.tv_usec;
    
    if (microseconds < 0) {
        seconds--;
        microseconds += 1000000;
    }
    
    printf("\n=== Результаты работы сервера ===\n");
    printf("Всего подключений: %d\n", clients_connected);
    printf("Всего сообщений получено: %d (ожидалось: %d)\n", 
           total_received_messages, clients_connected * TOTAL_MESSAGES);
    printf("Время работы: %ld.%06ld секунд\n", seconds, microseconds);
    printf("Средняя скорость: %.2f сообщений/сек\n", 
           total_received_messages / (seconds + microseconds / 1000000.0));
}

void signal_handler(int sig) {
    running = 0;
}

int main() {
    int server_fd, new_client_fd;
    struct sockaddr_un server_addr, client_addr;
    socklen_t client_len;
    struct pollfd fds[MAX_CLIENTS + 1];
    client_t clients[MAX_CLIENTS];
    int nfds = 1;
    int i, rc;
    int next_client_id = 0;
    
    // Засекаем время начала работы
    gettimeofday(&start_time, NULL);
    
    // Настраиваем обработчик сигналов
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // Инициализация массива клиентов
    for (i = 0; i < MAX_CLIENTS; i++) {
        clients[i].fd = -1;
        clients[i].client_id = -1;
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
    
    printf("Сервер запущен");
    
    fds[0].fd = server_fd;
    fds[0].events = POLLIN;
    fds[0].revents = 0;
    
    while (running) {
        rc = poll(fds, nfds, 10000); // Таймаут 1 секунда
        
        if (rc == -1) {
            if (errno == EINTR) continue;
            perror("poll");
            break;
        }
        
        // Проверяем новые подключения
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
                clients[client_index].client_id = next_client_id++;
                clients[client_index].message_count = 0;
                
                if (nfds < MAX_CLIENTS + 1) {
                    fds[nfds].fd = new_client_fd;
                    fds[nfds].events = POLLIN;
                    fds[nfds].revents = 0;
                    nfds++;
                }
                
                clients_connected++;
                print_current_time();
                printf("Клиент %d подключился\n", clients[client_index].client_id);
            } else {
                close(new_client_fd);
            }
        }
        
        // Обрабатываем данные от клиентов
        for (i = 1; i < nfds; i++) {
            if (fds[i].revents & POLLIN) {
                char temp_buffer[BUFFER_SIZE];
                ssize_t bytes_read = read(fds[i].fd, temp_buffer, BUFFER_SIZE - 1);
                
                if (bytes_read > 0) {
                    temp_buffer[bytes_read] = '\0';
                    
                    // Находим клиента
                    int found_client_index = -1;
                    int found_client_id = -1;
                    for (int j = 0; j < MAX_CLIENTS; j++) {
                        if (clients[j].fd == fds[i].fd) {
                            found_client_index = j;
                            found_client_id = clients[j].client_id;
                            clients[j].message_count++;
                            break;
                        }
                    }
                    
                    // Преобразуем в верхний регистр
                    for (int j = 0; j < bytes_read; j++) {
                        temp_buffer[j] = toupper(temp_buffer[j]);
                    }
                    
                    total_received_messages++;
                    
                    // Выводим информацию о полученном сообщении
                    print_current_time();
                    if (found_client_index != -1) {
                        printf("Клиент %d, - %d/%d: %s", 
                               found_client_id, 
                               clients[found_client_index].message_count,
                               TOTAL_MESSAGES,
                               temp_buffer);
                    } else {
                        printf("Неизвестный клиент, сообщение: %s", temp_buffer);
                    }
                    
                    // Отправляем ответ обратно клиенту
                    write(fds[i].fd, temp_buffer, bytes_read);
                    
                    // Проверяем, завершил ли клиент свою работу
                    if (found_client_index != -1 && 
                        clients[found_client_index].message_count >= TOTAL_MESSAGES) {
                        print_current_time();
                        printf("Клиент %d завершил отправку %d сообщений\n", 
                               found_client_id, TOTAL_MESSAGES);
                        close(fds[i].fd);
                        fds[i].fd = -1;
                        clients[found_client_index].fd = -1;
                        clients_completed++;
                    }
                    
                } else if (bytes_read == 0) {
                    // Клиент отключился
                    int disconnected_client_id = -1;
                    int disconnected_client_index = -1;
                    
                    for (int j = 0; j < MAX_CLIENTS; j++) {
                        if (clients[j].fd == fds[i].fd) {
                            disconnected_client_id = clients[j].client_id;
                            disconnected_client_index = j;
                            clients[j].fd = -1;
                            break;
                        }
                    }
                    
                    print_current_time();
                    if (disconnected_client_id != -1) {
                               disconnected_client_id, 
                               disconnected_client_index != -1 ? 
                               clients[disconnected_client_index].message_count : 0);
                    }
                    
                    close(fds[i].fd);
                    fds[i].fd = -1;
                }
            }
        }
        
        // Убираем закрытые дескрипторы
        for (i = 1; i < nfds; i++) {
            if (fds[i].fd == -1) {
                for (int j = i; j < nfds - 1; j++) {
                    fds[j] = fds[j + 1];
                }
                nfds--;
                i--;
            }
        }
        
        // Проверяем условие завершения
        if (clients_connected > 0 && clients_completed >= clients_connected) {
            printf("\nВсе клиенты завершили работу\n");
            break;
        }
    }
    
    // Выводим итоговую статистику
    print_uptime();
    
    // Закрываем все соединения
    for (i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].fd != -1) {
            close(clients[i].fd);
        }
    }
    close(server_fd);
    unlink(SOCKET_PATH);
    
    return 0;
}
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/select.h>
#include <ctype.h>
#include <signal.h>
#include <string.h>
#include <sys/time.h>
#include <errno.h>

#define SOCKET_PATH "/tmp/task31_socket"
#define BUFFER_SIZE 1024
#define MAX_CLIENTS 64

typedef struct {
    int fd;
    char buffer[BUFFER_SIZE];
    int active;
} client_info_t;

// Глобальные переменные для времени
struct timeval program_start_time;
volatile sig_atomic_t keep_running = 1;

void signal_handler(int sig) {
    struct timeval end_time;
    gettimeofday(&end_time, NULL);
    
    long sec = end_time.tv_sec - program_start_time.tv_sec;
    long usec = end_time.tv_usec - program_start_time.tv_usec;
    double total_time = (double)sec * 1000.0 + (double)usec / 1000.0;
    
    printf("\nServer ran for %.3f ms\n", total_time);
    keep_running = 0;
    exit(EXIT_SUCCESS);
}

static double elapsed_ms(const struct timeval *start, const struct timeval *end) {
    long sec = end->tv_sec - start->tv_sec;
    long usec = end->tv_usec - start->tv_usec;
    return (double)sec * 1000.0 + (double)usec / 1000.0;
}

int main() {
    int server_fd, client_fd;
    struct sockaddr_un server_addr, client_addr;
    socklen_t client_len;
    ssize_t nbytes;
    
    client_info_t clients[MAX_CLIENTS];
    int max_fd;
    int i, n_clients = 0;
    fd_set read_fds;
    
    struct timeval start_time, end_time;
    struct timeval select_timeout;
    
    // Устанавливаем обработчик сигнала
    signal(SIGINT, signal_handler);
    
    // Запоминаем время старта программы ДО любого вывода
    gettimeofday(&program_start_time, NULL);
    
    server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }
    
    unlink(SOCKET_PATH);
    
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, SOCKET_PATH, sizeof(server_addr.sun_path) - 1);
    
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    
    if (listen(server_fd, 5) == -1) {
        perror("listen");
        close(server_fd);
        unlink(SOCKET_PATH);
        exit(EXIT_FAILURE);
    }
    
    // Инициализируем массив клиентов
    for (i = 0; i < MAX_CLIENTS; i++) {
        clients[i].fd = -1;
        clients[i].active = 0;
    }
    
    printf("Server started. Listening on socket %s\n", SOCKET_PATH);
    printf("Press Ctrl+C to stop the server\n");
    
    // Главный цикл сервера
    while (keep_running) {
        FD_ZERO(&read_fds);
        FD_SET(server_fd, &read_fds);
        max_fd = server_fd;
        
        // Добавляем дескрипторы активных клиентов в set
        for (i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i].fd != -1) {
                FD_SET(clients[i].fd, &read_fds);
                if (clients[i].fd > max_fd) {
                    max_fd = clients[i].fd;
                }
            }
        }
        
        // Устанавливаем таймаут для select (0.1 секунды)
        select_timeout.tv_sec = 0;
        select_timeout.tv_usec = 100000;  // 100 мс
        
        // Ждем активности на любом из дескрипторов
        int select_result = select(max_fd + 1, &read_fds, NULL, NULL, &select_timeout);
        
        if (select_result == -1) {
            // Проверяем, был ли select прерван сигналом
            if (errno == EINTR) {
                // Сигнал прервал select, продолжаем (сигнал обработается в handler)
                continue;
            }
            perror("select");
            break;
        }
        
        if (select_result == 0) {
            // Таймаут, ничего не произошло
            continue;
        }
        
        // Проверяем новое соединение
        if (FD_ISSET(server_fd, &read_fds)) {
            client_len = sizeof(client_addr);
            client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
            if (client_fd == -1) {
                perror("accept");
                continue;
            }
            
            // Добавляем нового клиента
            for (i = 0; i < MAX_CLIENTS; i++) {
                if (clients[i].fd == -1) {
                    clients[i].fd = client_fd;
                    clients[i].active = 1;
                    n_clients++;
                    printf("Client %d connected (total clients: %d)\n", i, n_clients);
                    break;
                }
            }
            
            if (i == MAX_CLIENTS) {
                printf("Too many clients, connection rejected\n");
                close(client_fd);
            }
        }
        
        // Проверяем данные от существующих клиентов
        for (i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i].fd != -1 && FD_ISSET(clients[i].fd, &read_fds)) {
                // Засекаем время начала чтения (для времени обработки запроса)
                gettimeofday(&start_time, NULL);
                
                nbytes = read(clients[i].fd, clients[i].buffer, BUFFER_SIZE - 1);
                
                if (nbytes <= 0) {
                    // Соединение закрыто или ошибка
                    if (nbytes == 0) {
                        printf("Client %d disconnected (total clients: %d)\n", 
                               i, n_clients - 1);
                    } else {
                        // Проверяем, была ли ошибка из-за неблокирующего режима
                        if (errno == EAGAIN || errno == EWOULDBLOCK) {
                            // Нет данных для чтения, продолжаем
                            continue;
                        } else {
                            perror("read");
                        }
                    }
                    
                    close(clients[i].fd);
                    clients[i].fd = -1;
                    clients[i].active = 0;
                    n_clients--;
                } else {
                    // Вычисляем время обработки запроса
                    gettimeofday(&end_time, NULL);
                    double processing_time = elapsed_ms(&start_time, &end_time);
                    
                    // Обрабатываем полученные данные
                    clients[i].buffer[nbytes] = '\0';
                    
                    // Преобразуем текст в верхний регистр
                    for (int j = 0; j < nbytes; j++) {
                        clients[i].buffer[j] = toupper(clients[i].buffer[j]);
                    }
                    
                    // Создаем форматированный вывод с информацией о времени обработки
                    char output_buffer[BUFFER_SIZE + 128];
                    int output_len = snprintf(output_buffer, sizeof(output_buffer),
                                            "[Client %d, processing time: %.3f ms] ", 
                                            i, processing_time);
                    
                    // Копируем преобразованные данные
                    memcpy(output_buffer + output_len, clients[i].buffer, nbytes);
                    output_len += nbytes;
                    
                    // Добавляем перенос строки если его нет
                    if (nbytes > 0 && clients[i].buffer[nbytes-1] != '\n') {
                        output_buffer[output_len++] = '\n';
                    }
                    
                    // Выводим результат
                    write(STDOUT_FILENO, output_buffer, output_len);
                }
            }
        }
    }
    
    printf("\nShutting down server...\n");
    
    // Закрываем все соединения
    for (i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].fd != -1) {
            close(clients[i].fd);
            printf("Closed connection for client %d\n", i);
        }
    }
    
    close(server_fd);
    unlink(SOCKET_PATH);
    
    return EXIT_SUCCESS;
}
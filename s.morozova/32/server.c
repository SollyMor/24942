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
#include <fcntl.h>

#define SOCKET_PATH "/tmp/task31_socket"
#define BUFFER_SIZE 1024
#define MAX_CLIENTS 64

typedef struct {
    int fd;
    char buffer[BUFFER_SIZE];
    int buf_pos;                // Текущая позиция в буфере
    int active;
    struct timeval last_activity;
} client_info_t;

// Глобальные переменные
volatile sig_atomic_t keep_running = 1;
double total_processing_time = 0.0;
int total_requests_processed = 0;

void signal_handler(int sig) {
    printf("\n=== Server Statistics ===\n");
    printf("Total requests processed: %d\n", total_requests_processed);
    printf("Total processing time: %.3f ms\n", total_processing_time);
    if (total_requests_processed > 0) {
        printf("Average processing time per request: %.3f ms\n", 
               total_processing_time / total_requests_processed);
    }
    printf("Shutting down...\n");
    keep_running = 0;
}

static double elapsed_ms(const struct timeval *start, const struct timeval *end) {
    long sec = end->tv_sec - start->tv_sec;
    long usec = end->tv_usec - start->tv_usec;
    return (double)sec * 1000.0 + (double)usec / 1000.0;
}

// Обработка одного завершенного сообщения от клиента
static void process_client_message(client_info_t *client, int client_id, 
                                   struct timeval *start_time) {
    struct timeval end_time;
    gettimeofday(&end_time, NULL);
    
    // Вычисляем время обработки
    double processing_time = elapsed_ms(start_time, &end_time);
    
    // Добавляем к суммарному времени
    total_processing_time += processing_time;
    total_requests_processed++;
    
    // Преобразуем текст в верхний регистр
    for (int i = 0; i < client->buf_pos; i++) {
        client->buffer[i] = toupper(client->buffer[i]);
    }
    
    // Форматируем вывод
    char output_buffer[BUFFER_SIZE + 128];
    int output_len = snprintf(output_buffer, sizeof(output_buffer),
                            "[Client %d, processing time: %.3f ms] ", 
                            client_id, processing_time);
    
    // Копируем преобразованные данные
    memcpy(output_buffer + output_len, client->buffer, client->buf_pos);
    output_len += client->buf_pos;
    
    // Добавляем перенос строки если его нет
    if (client->buf_pos > 0 && client->buffer[client->buf_pos - 1] != '\n') {
        output_buffer[output_len++] = '\n';
    }
    
    // Выводим результат
    write(STDOUT_FILENO, output_buffer, output_len);
    
    // Сбрасываем буфер для следующего сообщения
    client->buf_pos = 0;
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
    
    struct timeval start_time, select_timeout;
    
    // Устанавливаем обработчик сигнала
    struct sigaction sa;
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);
    
    server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }
    
    // Устанавливаем неблокирующий режим для серверного сокета
    int flags = fcntl(server_fd, F_GETFL, 0);
    fcntl(server_fd, F_SETFL, flags | O_NONBLOCK);
    
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
        clients[i].buf_pos = 0;
    }
    
    printf("Async server started. Listening on socket %s\n", SOCKET_PATH);
    printf("Press Ctrl+C to stop the server and see statistics\n");
    
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
        
        // Устанавливаем таймаут для select (0.05 секунды)
        select_timeout.tv_sec = 0;
        select_timeout.tv_usec = 50000;  // 50 мс
        
        // Ждем активности на любом из дескрипторов
        int select_result = select(max_fd + 1, &read_fds, NULL, NULL, &select_timeout);
        
        if (select_result == -1) {
            if (errno == EINTR) {
                continue;  // Сигнал прервал select
            }
            perror("select");
            break;
        }
        
        if (select_result == 0) {
            // Таймаут, проверяем клиентов на таймаут неактивности
            continue;
        }
        
        // Проверяем новое соединение
        if (FD_ISSET(server_fd, &read_fds)) {
            client_len = sizeof(client_addr);
            client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
            
            while (client_fd != -1) {
                // Устанавливаем неблокирующий режим для нового клиента
                flags = fcntl(client_fd, F_GETFL, 0);
                fcntl(client_fd, F_SETFL, flags | O_NONBLOCK);
                
                // Ищем свободный слот для клиента
                int slot_found = 0;
                for (i = 0; i < MAX_CLIENTS; i++) {
                    if (clients[i].fd == -1) {
                        clients[i].fd = client_fd;
                        clients[i].active = 1;
                        clients[i].buf_pos = 0;
                        gettimeofday(&clients[i].last_activity, NULL);
                        n_clients++;
                        printf("Client %d connected (total clients: %d)\n", i, n_clients);
                        slot_found = 1;
                        break;
                    }
                }
                
                if (!slot_found) {
                    printf("Too many clients, connection rejected\n");
                    close(client_fd);
                }
                
                // Пробуем принять еще соединение (неблокирующий accept)
                client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
            }
        }
        
        // Обрабатываем данные от существующих клиентов
        for (i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i].fd != -1 && FD_ISSET(clients[i].fd, &read_fds)) {
                // Засекаем время начала обработки
                gettimeofday(&start_time, NULL);
                
                // Читаем доступные данные (неблокирующее чтение)
                while (1) {
                    // Проверяем, есть ли место в буфере
                    if (clients[i].buf_pos >= BUFFER_SIZE - 1) {
                        // Буфер полон, обрабатываем что есть
                        process_client_message(&clients[i], i, &start_time);
                    }
                    
                    // Читаем данные
                    nbytes = read(clients[i].fd, 
                                 clients[i].buffer + clients[i].buf_pos,
                                 BUFFER_SIZE - clients[i].buf_pos - 1);
                    
                    if (nbytes > 0) {
                        // Успешно прочитали данные
                        clients[i].buf_pos += nbytes;
                        clients[i].buffer[clients[i].buf_pos] = '\0';
                        gettimeofday(&clients[i].last_activity, NULL);
                        
                        // Проверяем, есть ли завершенное сообщение (по \n)
                        for (int j = 0; j < clients[i].buf_pos; j++) {
                            if (clients[i].buffer[j] == '\n') {
                                // Сохраняем позицию после \n
                                int message_end = j + 1;
                                
                                // Временно сохраняем остаток данных после \n
                                char temp_buf[BUFFER_SIZE];
                                int remaining = clients[i].buf_pos - message_end;
                                if (remaining > 0) {
                                    memcpy(temp_buf, clients[i].buffer + message_end, remaining);
                                }
                                
                                // Обрезаем буфер до конца сообщения
                                clients[i].buffer[message_end] = '\0';
                                int saved_buf_pos = clients[i].buf_pos;
                                clients[i].buf_pos = message_end;
                                
                                // Обрабатываем завершенное сообщение
                                process_client_message(&clients[i], i, &start_time);
                                
                                // Восстанавливаем остаток данных
                                if (remaining > 0) {
                                    memcpy(clients[i].buffer, temp_buf, remaining);
                                    clients[i].buf_pos = remaining;
                                    clients[i].buffer[clients[i].buf_pos] = '\0';
                                } else {
                                    clients[i].buf_pos = 0;
                                }
                                
                                break;
                            }
                        }
                    } else if (nbytes == 0) {
                        // Соединение закрыто клиентом
                        printf("Client %d disconnected (total clients: %d)\n", 
                               i, n_clients - 1);
                        
                        // Обрабатываем оставшиеся данные в буфере
                        if (clients[i].buf_pos > 0) {
                            process_client_message(&clients[i], i, &start_time);
                        }
                        
                        close(clients[i].fd);
                        clients[i].fd = -1;
                        clients[i].active = 0;
                        n_clients--;
                        break;
                    } else if (nbytes == -1) {
                        // Ошибка или нет данных
                        if (errno == EAGAIN || errno == EWOULDBLOCK) {
                            // Нет данных для чтения, выходим из цикла
                            break;
                        } else {
                            // Другая ошибка
                            perror("read");
                            close(clients[i].fd);
                            clients[i].fd = -1;
                            clients[i].active = 0;
                            n_clients--;
                            break;
                        }
                    }
                }
            }
        }
    }
    
    printf("\n=== Final Statistics ===\n");
    printf("Total requests processed: %d\n", total_requests_processed);
    printf("Total processing time: %.3f ms\n", total_processing_time);
    
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
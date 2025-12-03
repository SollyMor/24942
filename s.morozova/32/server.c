#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <aio.h>
#include <fcntl.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <sys/time.h>

#define SOCKET_PATH "/tmp/task32_socket"
#define BUFFER_SIZE 1024
#define MAX_CLIENTS 64

// Структура для хранения информации о клиенте
typedef struct {
    int fd;
    struct aiocb aio_req;
    char buffer[BUFFER_SIZE];
    int active;
    struct timeval receive_time;  // Время получения сообщения
} client_info_t;

static double elapsed_ms(const struct timeval *start, const struct timeval *end) {
    long sec = end->tv_sec - start->tv_sec;
    long usec = end->tv_usec - start->tv_usec;
    return (double)sec * 1000.0 + (double)usec / 1000.0;
}

client_info_t clients[MAX_CLIENTS];
int server_fd;
volatile sig_atomic_t running = 1;

// Обработчик сигнала для корректного завершения
void signal_handler(int sig) {
    printf("\nReceived signal %d, shutting down...\n", sig);
    running = 0;
}

// Инициализация асинхронного чтения для клиента
int init_client_read(int client_idx) {
    client_info_t *client = &clients[client_idx];
    
    // Очищаем структуру aiocb
    memset(&client->aio_req, 0, sizeof(struct aiocb));
    
    // Настраиваем асинхронное чтение
    client->aio_req.aio_fildes = client->fd;
    client->aio_req.aio_buf = client->buffer;
    client->aio_req.aio_nbytes = BUFFER_SIZE - 1;
    client->aio_req.aio_offset = 0;
    client->aio_req.aio_sigevent.sigev_notify = SIGEV_NONE;
    
    // Инициируем асинхронное чтение
    if (aio_read(&client->aio_req) == -1) {
        return -1;
    }
    
    return 0;
}

int main() {
    int client_fd;
    struct sockaddr_un server_addr, client_addr;
    socklen_t client_len;
    int i, n_clients = 0;
    ssize_t nbytes;
    struct aiocb *aiocb_list[MAX_CLIENTS];
    int n_pending;
    
    struct timeval start_time, end_time;
    struct timeval program_start_time;
    
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // Запоминаем время старта программы
    gettimeofday(&program_start_time, NULL);
    
    // Создаем сокет
    server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }
    
    unlink(SOCKET_PATH);
    
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, SOCKET_PATH, sizeof(server_addr.sun_path) - 1);
    
    // Привязываем сокет к адресу
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    
    // Начинаем слушать соединения
    if (listen(server_fd, 5) == -1) {
        perror("listen");
        close(server_fd);
        unlink(SOCKET_PATH);
        exit(EXIT_FAILURE);
    }
    
    // Делаем серверный сокет неблокирующим для асинхронного accept
    int flags = fcntl(server_fd, F_GETFL, 0);
    fcntl(server_fd, F_SETFL, flags | O_NONBLOCK);
    
    // Инициализируем массив клиентов
    for (i = 0; i < MAX_CLIENTS; i++) {
        clients[i].fd = -1;
        clients[i].active = 0;
    }
    
    printf("Server started. Listening on socket %s\n", SOCKET_PATH);
    printf("Press Ctrl+C to stop the server\n");
    
    while (running) {
        // Принимаем новые соединения (неблокирующий accept)
        while (1) {
            client_len = sizeof(client_addr);
            client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
            
            if (client_fd == -1) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    // Нет новых соединений
                    break;
                } else {
                    perror("accept");
                    break;
                }
            }
            
            // Делаем клиентский сокет неблокирующим
            flags = fcntl(client_fd, F_GETFL, 0);
            fcntl(client_fd, F_SETFL, flags | O_NONBLOCK);
            
            // Добавляем нового клиента в массив
            for (i = 0; i < MAX_CLIENTS; i++) {
                if (clients[i].fd == -1) {
                    clients[i].fd = client_fd;
                    clients[i].active = 1;
                    n_clients++;
                    
                    printf("Client %d connected (total clients: %d)\n", i, n_clients);
                    
                    // Инициализируем асинхронное чтение для нового клиента
                    if (init_client_read(i) == -1) {
                        perror("aio_read");
                        close(clients[i].fd);
                        clients[i].fd = -1;
                        clients[i].active = 0;
                        n_clients--;
                    }
                    break;
                }
            }
            
            // Если массив переполнен, закрываем соединение
            if (i == MAX_CLIENTS) {
                printf("Too many clients, connection rejected\n");
                close(client_fd);
            }
        }
        
        // Собираем список активных асинхронных операций
        n_pending = 0;
        for (i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i].active && clients[i].fd != -1) {
                aiocb_list[n_pending++] = &clients[i].aio_req;
            }
        }
        
        if (n_pending > 0) {
            // Ждем завершения хотя бы одной асинхронной операции
            // Используем таймаут 0.1 секунды для периодической проверки новых соединений
            struct timespec timeout = {0, 100000000}; // 0.1 секунды
            int result = aio_suspend((const struct aiocb *const *)aiocb_list, n_pending, &timeout);
            
            if (result == -1 && errno != EINTR) {
                // Ошибка при ожидании
                if (errno != EAGAIN && errno != EINTR) {
                    perror("aio_suspend");
                }
            }
            
            // Проверяем результаты всех операций
            for (i = 0; i < MAX_CLIENTS; i++) {
                if (clients[i].active && clients[i].fd != -1) {
                    // Проверяем статус асинхронной операции
                    int op_status = aio_error(&clients[i].aio_req);
                    
                    if (op_status == 0) {
                        // Операция завершена успешно
                        // Запоминаем время получения данных
                        gettimeofday(&start_time, NULL);
                        clients[i].receive_time = start_time;
                        
                        nbytes = aio_return(&clients[i].aio_req);
                        
                        if (nbytes <= 0) {
                            // Соединение закрыто или ошибка
                            printf("Client %d disconnected (total clients: %d)\n", i, n_clients - 1);
                            close(clients[i].fd);
                            clients[i].fd = -1;
                            clients[i].active = 0;
                            n_clients--;
                        } else {
                            // Обрабатываем полученные данные
                            clients[i].buffer[nbytes] = '\0';
                            
                            // Вычисляем время с момента получения данных
                            gettimeofday(&end_time, NULL);
                            double processing_time = elapsed_ms(&start_time, &end_time);
                            
                            // Преобразуем текст в верхний регистр
                            for (int j = 0; j < nbytes; j++) {
                                clients[i].buffer[j] = toupper(clients[i].buffer[j]);
                            }
                            
                            // Выводим результат с информацией о времени
                            char output_buffer[BUFFER_SIZE + 128];
                            int output_len = snprintf(output_buffer, sizeof(output_buffer),
                                                    "[Client %d, %.3f ms] ", 
                                                    i, processing_time);
                            
                            // Копируем преобразованные данные
                            memcpy(output_buffer + output_len, clients[i].buffer, nbytes);
                            output_len += nbytes;
                            
                            // Добавляем перенос строки если его нет
                            if (nbytes > 0 && clients[i].buffer[nbytes-1] != '\n') {
                                output_buffer[output_len++] = '\n';
                            }
                            
                            // Выводим в стандартный поток вывода
                            write(STDOUT_FILENO, output_buffer, output_len);
                            
                            // Запускаем следующее асинхронное чтение
                            if (init_client_read(i) == -1) {
                                // Ошибка при инициализации чтения
                                if (errno != EAGAIN && errno != EWOULDBLOCK) {
                                    printf("Client %d read error, disconnecting\n", i);
                                    close(clients[i].fd);
                                    clients[i].fd = -1;
                                    clients[i].active = 0;
                                    n_clients--;
                                }
                            }
                        }
                    } else if (op_status != EINPROGRESS) {
                        // Ошибка при выполнении операции
                        printf("Client %d operation error, disconnecting\n", i);
                        close(clients[i].fd);
                        clients[i].fd = -1;
                        clients[i].active = 0;
                        n_clients--;
                    }
                }
            }
        } else {
            // Нет активных клиентов, небольшая пауза
            usleep(100000); // 0.1 секунды
        }
    }
    
    printf("\nShutting down server...\n");
    
    // Закрываем все соединения и очищаем ресурсы
    for (i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].fd != -1) {
            // Отменяем все незавершенные операции
            aio_cancel(clients[i].fd, &clients[i].aio_req);
            close(clients[i].fd);
            printf("Closed connection for client %d\n", i);
        }
    }
    
    close(server_fd);
    unlink(SOCKET_PATH);
    
    // Вычисляем общее время работы сервера
    gettimeofday(&end_time, NULL);
    double total_time = elapsed_ms(&program_start_time, &end_time);
    printf("Server ran for %.3f ms\n", total_time);
    
    return EXIT_SUCCESS;
}
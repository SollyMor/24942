#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <liburing.h>
#include <ctype.h>
#include <time.h>

#define SOCKET_PATH "/tmp/task31_socket"
#define BUFFER_SIZE 1024
#define MAX_CLIENTS 64
#define QUEUE_DEPTH 256
#define MAX_MSG_LEN 128

typedef struct {
    int fd;
    int client_id;
    char buffer[BUFFER_SIZE];
    int buf_pos;
    struct timeval start_time;
} client_ctx_t;

typedef struct {
    int type;  // 0: accept, 1: read, 2: write
    union {
        struct {
            int client_id;
        } accept_data;
        struct {
            int client_id;
            char *data;
            size_t len;
        } write_data;
    };
} request_data_t;

volatile sig_atomic_t keep_running = 1;
int total_requests = 0;
double total_time = 0.0;

void signal_handler(int sig) {
    printf("\n=== Server Statistics ===\n");
    printf("Total requests processed: %d\n", total_requests);
    printf("Total processing time: %.3f ms\n", total_time);
    if (total_requests > 0) {
        printf("Average processing time: %.3f ms\n", total_time / total_requests);
    }
    printf("Shutting down...\n");
    keep_running = 0;
}

static double elapsed_ms(const struct timeval *start, const struct timeval *end) {
    long sec = end->tv_sec - start->tv_sec;
    long usec = end->tv_usec - start->tv_usec;
    return (double)sec * 1000.0 + (double)usec / 1000.0;
}

// Добавление операции accept в очередь
static int add_accept_request(struct io_uring *ring, int server_fd, 
                             client_ctx_t *client, int client_id) {
    struct io_uring_sqe *sqe = io_uring_get_sqe(ring);
    if (!sqe) return -1;
    
    client->fd = -1;
    client->client_id = client_id;
    client->buf_pos = 0;
    
    request_data_t *data = malloc(sizeof(request_data_t));
    data->type = 0;
    data->accept_data.client_id = client_id;
    
    io_uring_prep_accept(sqe, server_fd, NULL, NULL, 0);
    io_uring_sqe_set_data(sqe, data);
    
    return 0;
}

// Добавление операции read в очередь
static int add_read_request(struct io_uring *ring, client_ctx_t *client) {
    struct io_uring_sqe *sqe = io_uring_get_sqe(ring);
    if (!sqe) return -1;
    
    // Используем часть буфера для чтения
    int space = BUFFER_SIZE - client->buf_pos;
    if (space <= 0) {
        fprintf(stderr, "Buffer full for client %d\n", client->client_id);
        return -1;
    }
    
    gettimeofday(&client->start_time, NULL);
    
    io_uring_prep_read(sqe, client->fd, 
                      client->buffer + client->buf_pos, 
                      space, 0);
    
    request_data_t *data = malloc(sizeof(request_data_t));
    data->type = 1;
    data->accept_data.client_id = client->client_id;
    io_uring_sqe_set_data(sqe, data);
    
    return 0;
}

// Обработка прочитанных данных
static void process_data(client_ctx_t *client, int bytes_read) {
    if (bytes_read <= 0) {
        printf("Client %d disconnected\n", client->client_id);
        close(client->fd);
        client->fd = -1;
        return;
    }
    
    client->buf_pos += bytes_read;
    client->buffer[client->buf_pos] = '\0';
    
    // Ищем завершенные сообщения (по \n)
    int msg_start = 0;
    for (int i = 0; i < client->buf_pos; i++) {
        if (client->buffer[i] == '\n') {
            // Нашли конец сообщения
            struct timeval end_time;
            gettimeofday(&end_time, NULL);
            
            double proc_time = elapsed_ms(&client->start_time, &end_time);
            total_time += proc_time;
            total_requests++;
            
            // Преобразуем в верхний регистр
            for (int j = msg_start; j <= i; j++) {
                client->buffer[j] = toupper(client->buffer[j]);
            }
            
            // Выводим результат
            char output[MAX_MSG_LEN];
            int len = snprintf(output, sizeof(output),
                             "[Client %d, time: %.3f ms] ",
                             client->client_id, proc_time);
            
            write(STDOUT_FILENO, output, len);
            write(STDOUT_FILENO, client->buffer + msg_start, i - msg_start + 1);
            
            msg_start = i + 1;
        }
    }
    
    // Сдвигаем оставшиеся данные в начало буфера
    if (msg_start > 0) {
        int remaining = client->buf_pos - msg_start;
        if (remaining > 0) {
            memmove(client->buffer, client->buffer + msg_start, remaining);
        }
        client->buf_pos = remaining;
    }
}

int main() {
    struct io_uring ring;
    int server_fd;
    struct sockaddr_un server_addr;
    client_ctx_t clients[MAX_CLIENTS];
    int next_client_id = 0;
    
    // Инициализация io_uring
    if (io_uring_queue_init(QUEUE_DEPTH, &ring, 0) < 0) {
        perror("io_uring_queue_init");
        exit(EXIT_FAILURE);
    }
    
    // Обработчик сигналов
    struct sigaction sa;
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);
    
    // Создание серверного сокета
    server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }
    
    // Установка неблокирующего режима
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
    
    if (listen(server_fd, SOMAXCONN) == -1) {
        perror("listen");
        close(server_fd);
        unlink(SOCKET_PATH);
        exit(EXIT_FAILURE);
    }
    
    // Инициализация клиентов
    for (int i = 0; i < MAX_CLIENTS; i++) {
        clients[i].fd = -1;
        clients[i].client_id = -1;
        clients[i].buf_pos = 0;
    }
    
    printf("Async I/O server started with io_uring\n");
    printf("Listening on %s\n", SOCKET_PATH);
    printf("Press Ctrl+C to stop\n");
    
    // Добавляем начальный accept запрос
    int client_id = next_client_id++;
    add_accept_request(&ring, server_fd, &clients[client_id], client_id);
    
    // Главный цикл
    while (keep_running) {
        struct io_uring_cqe *cqe;
        int ret;
        
        // Отправляем запросы в ядро
        ret = io_uring_submit(&ring);
        if (ret < 0) {
            fprintf(stderr, "io_uring_submit: %s\n", strerror(-ret));
            break;
        }
        
        // Ждем завершения операций
        ret = io_uring_wait_cqe(&ring, &cqe);
        if (ret < 0) {
            fprintf(stderr, "io_uring_wait_cqe: %s\n", strerror(-ret));
            break;
        }
        
        // Обрабатываем завершенные операции
        struct io_uring_cqe *cqes[QUEUE_DEPTH];
        int count = io_uring_peek_batch_cqe(&ring, cqes, QUEUE_DEPTH);
        
        for (int i = 0; i < count; i++) {
            cqe = cqes[i];
            request_data_t *data = (request_data_t *)io_uring_cqe_get_data(cqe);
            int res = cqe->res;
            
            if (data) {
                int cid = data->accept_data.client_id;
                
                if (data->type == 0) {  // accept
                    if (res >= 0) {
                        // Успешное подключение
                        clients[cid].fd = res;
                        
                        // Устанавливаем неблокирующий режим для клиента
                        flags = fcntl(res, F_GETFL, 0);
                        fcntl(res, F_SETFL, flags | O_NONBLOCK);
                        
                        printf("Client %d connected\n", cid);
                        
                        // Добавляем запрос на чтение для этого клиента
                        add_read_request(&ring, &clients[cid]);
                        
                        // Добавляем новый accept запрос для следующего клиента
                        if (next_client_id < MAX_CLIENTS) {
                            int new_cid = next_client_id++;
                            add_accept_request(&ring, server_fd, &clients[new_cid], new_cid);
                        }
                    } else {
                        fprintf(stderr, "Accept failed: %s\n", strerror(-res));
                    }
                } 
                else if (data->type == 1) {  // read
                    if (clients[cid].fd != -1) {
                        if (res > 0) {
                            // Успешно прочитали данные
                            process_data(&clients[cid], res);
                            
                            // Снова добавляем запрос на чтение
                            if (clients[cid].fd != -1) {
                                add_read_request(&ring, &clients[cid]);
                            }
                        } else if (res == 0 || res == -ECONNRESET) {
                            // Соединение закрыто
                            printf("Client %d disconnected\n", cid);
                            close(clients[cid].fd);
                            clients[cid].fd = -1;
                        } else if (res == -EAGAIN) {
                            // Попробовать снова
                            if (clients[cid].fd != -1) {
                                add_read_request(&ring, &clients[cid]);
                            }
                        } else {
                            fprintf(stderr, "Read failed for client %d: %s\n", 
                                    cid, strerror(-res));
                        }
                    }
                }
                
                free(data);
            }
            
            io_uring_cqe_seen(&ring, cqe);
        }
    }
    
    // Очистка
    printf("\nCleaning up...\n");
    
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].fd != -1) {
            close(clients[i].fd);
        }
    }
    
    io_uring_queue_exit(&ring);
    close(server_fd);
    unlink(SOCKET_PATH);
    
    return EXIT_SUCCESS;
}
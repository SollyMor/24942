#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <aio.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <sys/time.h>

#define SOCKET_PATH "/tmp/task32_socket"
#define BUFFER_SIZE 1024
#define MAX_CLIENTS 64

typedef struct {
    int fd;
    struct aiocb aio_req;
    char buffer[BUFFER_SIZE];
    int active;
} client_info_t;

static double elapsed_ms(const struct timeval *start, const struct timeval *end) {
    long sec = end->tv_sec - start->tv_sec;
    long usec = end->tv_usec - start->tv_usec;
    return (double)sec * 1000.0 + (double)usec / 1000.0;
}

client_info_t clients[MAX_CLIENTS];
int server_fd;
volatile sig_atomic_t running = 1;
struct timeval program_start_time;

// Обработчик сигнала для корректного завершения
void signal_handler(int sig) {
    running = 0;
}

// Вывод времени работы
void print_uptime() {
    struct timeval end_time;
    gettimeofday(&end_time, NULL);
    double total_time_ms = elapsed_ms(&program_start_time, &end_time);
    printf("Время работы: %.3f ms\n", total_time_ms);
}

// Инициализация асинхронного чтения для клиента
int init_client_read(int client_idx) {
    client_info_t *client = &clients[client_idx];
    
    memset(&client->aio_req, 0, sizeof(struct aiocb));
    
    client->aio_req.aio_fildes = client->fd;
    client->aio_req.aio_buf = client->buffer;
    client->aio_req.aio_nbytes = BUFFER_SIZE - 1;
    client->aio_req.aio_offset = 0;
    client->aio_req.aio_sigevent.sigev_notify = SIGEV_NONE;
    
    return aio_read(&client->aio_req);
}

int main() {
    int client_fd;
    struct sockaddr_un server_addr, client_addr;
    socklen_t client_len;
    int i;
    ssize_t nbytes;
    
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // Засекаем время начала работы
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
    
    printf("Сервер запущен\n");
    
    while (running) {
        // Принимаем новые соединения
        client_len = sizeof(client_addr);
        client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
        
        if (client_fd != -1) {
            // Добавляем нового клиента
            for (i = 0; i < MAX_CLIENTS; i++) {
                if (!clients[i].active) {
                    clients[i].fd = client_fd;
                    clients[i].active = 1;
                    
                    if (init_client_read(i) == -1) {
                        perror("aio_read");
                        close(client_fd);
                        clients[i].active = 0;
                    }
                    break;
                }
            }
            
            if (i == MAX_CLIENTS) {
                close(client_fd);
            }
        } else if (errno != EAGAIN && errno != EWOULDBLOCK) {
            if (errno != EINTR) {
                perror("accept");
            }
        }
        
        // Проверяем завершение асинхронных операций
        for (i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i].active) {
                int status = aio_error(&clients[i].aio_req);
                
                if (status == 0) {
                    // Операция завершена
                    nbytes = aio_return(&clients[i].aio_req);
                    
                    if (nbytes > 0) {
                        // Обрабатываем данные
                        clients[i].buffer[nbytes] = '\0';
                        
                        for (int j = 0; j < nbytes; j++) {
                            clients[i].buffer[j] = toupper(clients[i].buffer[j]);
                        }
                        
                        write(STDOUT_FILENO, clients[i].buffer, nbytes);
                        
                        // Запускаем следующее чтение
                        if (init_client_read(i) == -1) {
                            perror("aio_read");
                            close(clients[i].fd);
                            clients[i].active = 0;
                        }
                    } else if (nbytes == 0) {
                        // Клиент отключился
                        close(clients[i].fd);
                        clients[i].active = 0;
                    } else {
                        // Ошибка чтения
                        perror("read error");
                        close(clients[i].fd);
                        clients[i].active = 0;
                    }
                } else if (status != EINPROGRESS) {
                    // Ошибка AIO
                    if (status != ECANCELED) {
                        perror("aio_error");
                    }
                    close(clients[i].fd);
                    clients[i].active = 0;
                }
            }
        }
        
        // Небольшая задержка чтобы не грузить CPU
        usleep(10000); // 10 мс
    }
    
    // Выводим время работы
    print_uptime();
    
    // Закрываем все соединения
    for (i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].active) {
            aio_cancel(clients[i].fd, NULL); // Отменяем все операции для этого fd
            close(clients[i].fd);
        }
    }
    
    close(server_fd);
    unlink(SOCKET_PATH);
    
    return EXIT_SUCCESS;
}
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <fcntl.h>
#include <time.h>
#include <aio.h>
#include <signal.h>

#define NUM_CLIENTS 10
#define SOCKET_PATH "socket32"
#define BUFFER_SIZE 256
#define MAX_TIMEOUT_SECONDS 30

typedef struct {
    int fd;
    int active;
    int client_id;
    struct aiocb aio;
    char buf[BUFFER_SIZE + 1];
    time_t connect_time;
} client_t;

client_t clients[NUM_CLIENTS];
int server_fd;
time_t start_time;
volatile sig_atomic_t running = 1;

void cleanup() {
    unlink(SOCKET_PATH);
}

void signal_handler(int sig) {
    running = 0;
}

void print_time_event(int client_id, const char *event) {
    time_t current = time(NULL);
    double elapsed = difftime(current, start_time);
    printf("[Time: %.0fs] Client %d: %s\n", elapsed, client_id, event);
    fflush(stdout);
}

// Альтернативная реализация с использованием fcntl неблокирующего режима
// и ручной проверкой вместо AIO на Solaris
void process_clients_with_select() {
    fd_set readfds;
    struct timeval tv;
    int max_fd = server_fd;
    
    FD_ZERO(&readfds);
    FD_SET(server_fd, &readfds);
    
    for (int i = 0; i < NUM_CLIENTS; i++) {
        if (clients[i].active && clients[i].fd >= 0) {
            FD_SET(clients[i].fd, &readfds);
            if (clients[i].fd > max_fd) {
                max_fd = clients[i].fd;
            }
        }
    }
    
    tv.tv_sec = 0;
    tv.tv_usec = 50000; // 50ms timeout
    
    int activity = select(max_fd + 1, &readfds, NULL, NULL, &tv);
    
    if (activity > 0) {
        // Проверяем новые подключения
        if (FD_ISSET(server_fd, &readfds)) {
            int new_fd = accept(server_fd, NULL, NULL);
            if (new_fd >= 0) {
                // Находим свободный слот
                for (int i = 0; i < NUM_CLIENTS; i++) {
                    if (!clients[i].active) {
                        clients[i].fd = new_fd;
                        clients[i].active = 1;
                        clients[i].client_id = i + 1;
                        clients[i].connect_time = time(NULL);
                        
                        // Устанавливаем неблокирующий режим
                        fcntl(new_fd, F_SETFL, O_NONBLOCK);
                        
                        print_time_event(clients[i].client_id, "CONNECTED");
                        break;
                    }
                }
            }
        }
        
        // Проверяем данные от клиентов
        for (int i = 0; i < NUM_CLIENTS; i++) {
            if (clients[i].active && clients[i].fd >= 0 && 
                FD_ISSET(clients[i].fd, &readfds)) {
                
                char buffer[BUFFER_SIZE];
                int n = read(clients[i].fd, buffer, sizeof(buffer) - 1);
                
                if (n > 0) {
                    buffer[n] = '\0';
                    
                    // Преобразуем в верхний регистр
                    for (int j = 0; j < n; j++) {
                        buffer[j] = toupper((unsigned char)buffer[j]);
                    }
                    
                    // Выводим
                    printf("[Client %d]: ", clients[i].client_id);
                    write(STDOUT_FILENO, buffer, n);
                    
                } else if (n == 0) {
                    // Клиент отключился
                    print_time_event(clients[i].client_id, "DISCONNECTED");
                    close(clients[i].fd);
                    clients[i].active = 0;
                    
                } else if (errno != EWOULDBLOCK && errno != EAGAIN) {
                    // Ошибка чтения
                    fprintf(stderr, "Read error for client %d\n", clients[i].client_id);
                    close(clients[i].fd);
                    clients[i].active = 0;
                }
            }
        }
    }
}

int main() {
    // ФИКСИРУЕМ ВРЕМЯ СТАРТА
    start_time = time(NULL);
    printf("=== ASYNC SERVER STARTED ===\n");
    printf("Start time: %s", ctime(&start_time));
    
    // Инициализация клиентов
    for (int i = 0; i < NUM_CLIENTS; i++) {
        clients[i].active = 0;
        clients[i].client_id = 0;
        clients[i].fd = -1;
    }
    
    atexit(cleanup);
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    // Создание серверного сокета
    server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
        return 1;
    }

    // Настройка адреса
    unlink(SOCKET_PATH);
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

    // Устанавливаем опцию REUSEADDR
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(server_fd);
        return 1;
    }

    if (listen(server_fd, NUM_CLIENTS) < 0) {
        perror("listen");
        close(server_fd);
        return 1;
    }

    // Делаем сервер неблокирующим
    fcntl(server_fd, F_SETFL, O_NONBLOCK);

    printf("Server listening on: %s\n", SOCKET_PATH);
    printf("Using non-blocking I/O with select()\n");
    printf("Maximum clients: %d\n", NUM_CLIENTS);
    printf("Press Ctrl+C to stop server\n\n");

    int active_clients = 0;
    time_t last_activity = time(NULL);

    // Главный цикл сервера
    while (running) {
        // Подсчет активных клиентов
        active_clients = 0;
        for (int i = 0; i < NUM_CLIENTS; i++) {
            if (clients[i].active) {
                active_clients++;
            }
        }
        
        // Обработка клиентов
        process_clients_with_select();
        
        // Обновляем время последней активности
        if (active_clients > 0) {
            last_activity = time(NULL);
        }
        
        // Проверка таймаута
        time_t now = time(NULL);
        if (difftime(now, last_activity) > MAX_TIMEOUT_SECONDS) {
            printf("\nNo activity for %d seconds. Shutting down...\n", MAX_TIMEOUT_SECONDS);
            running = 0;
            break;
        }
        
        // Небольшая пауза для снижения нагрузки на CPU
//        usleep(10000); // 10ms
    }

    // ЗАВЕРШЕНИЕ - ВЫВОД ВРЕМЕНИ РАБОТЫ
    time_t end_time = time(NULL);
    double elapsed = difftime(end_time, start_time);
    
    printf("\n=== SERVER FINISHED ===\n");
    printf("Start time: %s", ctime(&start_time));
    printf("End time:   %s", ctime(&end_time));
    printf("Total execution time: %.0f seconds\n", elapsed);
    printf("Active clients at exit: %d\n", active_clients);

    // Закрытие всех соединений
    for (int i = 0; i < NUM_CLIENTS; i++) {
        if (clients[i].active && clients[i].fd >= 0) {
            close(clients[i].fd);
        }
    }
    
    close(server_fd);
    return 0;
}

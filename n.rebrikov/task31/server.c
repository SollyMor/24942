#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <poll.h>
#include <signal.h>
#include <errno.h>
#include <time.h>

#define MAX_CLIENTS 100
#define BUFFER_SIZE 1024

char *socket_path = "./socket31";
time_t start_time = 0;
volatile sig_atomic_t running = 1;

void cleanup() {
    unlink(socket_path);
}

void signal_handler(int sig) {
    running = 0;
}

int main() {
    int server_fd, rc;
    struct pollfd poll_fds[MAX_CLIENTS + 1];
    int client_fds[MAX_CLIENTS];  // Объявление массива клиентов
    
    // ФИКСИРУЕМ ВРЕМЯ СТАРТА
    start_time = time(NULL);
    printf("Server started at: %s", ctime(&start_time));
    
    // Инициализация массива клиентов
    for (int i = 0; i < MAX_CLIENTS; i++) {
        client_fds[i] = -1;
    }
    
    atexit(cleanup);
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    // Создание серверного сокета
    if ((server_fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        perror("socket error");
        exit(EXIT_FAILURE);
    }

    // Настройка адреса
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path) - 1);

    unlink(socket_path);

    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        perror("bind error");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 10) == -1) {
        perror("listen error");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Server listening on socket: %s\n", socket_path);

    // Инициализация массива для poll
    int nfds = 1;
    poll_fds[0].fd = server_fd;
    poll_fds[0].events = POLLIN;

    // Главный цикл сервера
    while (running) {
        // Обновление массива poll_fds
        nfds = 1;
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (client_fds[i] >= 0) {
                poll_fds[nfds].fd = client_fds[i];
                poll_fds[nfds].events = POLLIN;
                nfds++;
            }
        }

        // Ожидание событий
        rc = poll(poll_fds, nfds, 1000);  // Таймаут 1 секунда
        if (rc == -1) {
            if (errno == EINTR) continue;
            perror("poll error");
            break;
        }

        // Проверка всех файловых дескрипторов
        for (int i = 0; i < nfds; i++) {
            if (poll_fds[i].revents == 0) continue;

            // Новое подключение
            if (poll_fds[i].fd == server_fd) {
                int new_client = accept(server_fd, NULL, NULL);
                if (new_client == -1) {
                    if (errno == EINTR) continue;
                    perror("accept error");
                    continue;
                }

                // Поиск свободного слота
                int slot = -1;
                for (int j = 0; j < MAX_CLIENTS; j++) {
                    if (client_fds[j] < 0) {
                        slot = j;
                        client_fds[j] = new_client;
                        break;
                    }
                }

                if (slot >= 0) {
                    printf("New client connected (slot: %d, fd: %d)\n", slot, new_client);
                } else {
                    printf("Too many clients. Connection rejected.\n");
                    close(new_client);
                }
            }
            // Данные от клиента
            else {
                char buffer[BUFFER_SIZE];
                int client_fd = poll_fds[i].fd;
                
                rc = read(client_fd, buffer, sizeof(buffer) - 1);
                
                if (rc > 0) {
                    // Преобразование в верхний регистр
                    for (int j = 0; j < rc; j++) {
                        buffer[j] = toupper((unsigned char)buffer[j]);
                    }
                    
                    // Вывод преобразованного текста
                    write(STDOUT_FILENO, buffer, rc);
                }
                else if (rc == 0) {
                    // Клиент отключился
                    printf("Client disconnected (fd: %d)\n", client_fd);
                    close(client_fd);
                    
                    // Удаляем из массива
                    for (int j = 0; j < MAX_CLIENTS; j++) {
                        if (client_fds[j] == client_fd) {
                            client_fds[j] = -1;
                            break;
                        }
                    }
                }
                else {
                    if (errno == EINTR) continue;
                    perror("read error");
                    close(client_fd);
                    
                    // Удаляем из массива
                    for (int j = 0; j < MAX_CLIENTS; j++) {
                        if (client_fds[j] == client_fd) {
                            client_fds[j] = -1;
                            break;
                        }
                    }
                }
            }
        }
    }

    // ВЫВОДИТСЯ ВРЕМЯ РАБОТЫ ПРОГРАММЫ
    time_t end_time = time(NULL);
    double elapsed = difftime(end_time, start_time);
    printf("\n=== SERVER FINISHED ===\n");
    printf("Start time: %s", ctime(&start_time));
    printf("End time:   %s", ctime(&end_time));
    printf("Total execution time: %.0f seconds\n", elapsed);

    // Закрытие всех соединений
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (client_fds[i] >= 0) {
            close(client_fds[i]);
        }
    }
    close(server_fd);
    
    return 0;
}
